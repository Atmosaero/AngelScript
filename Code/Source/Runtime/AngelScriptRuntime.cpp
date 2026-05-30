#include <Runtime/AngelScriptRuntime.h>

#include <Bindings/Entity.h>
#include <Bindings/Gameplay.h>
#include <Runtime/BehaviorContextInspector.h>
#include <Runtime/EntityBusBinding.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/Utils/Utils.h>

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
#include <angelscript.h>
#endif

namespace AngelScript
{
    thread_local AngelScriptRuntime* AngelScriptRuntime::s_currentExecutionRuntime = nullptr;
    thread_local ScriptInstanceId AngelScriptRuntime::s_currentExecutionInstanceId = InvalidScriptInstanceId;
    thread_local AZStd::string s_lastCompileMessages;

    namespace
    {
        constexpr const char* RuntimeLogWindow = "AngelScript";
        constexpr size_t MaxScriptFileSize = 1024 * 1024;

        bool RuntimeIsIdentifierContinue(char value)
        {
            return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') || (value >= '0' && value <= '9') || value == '_';
        }

        size_t RuntimeFindMetadataEnd(const AZStd::string& source, size_t openBracketIndex)
        {
            bool inString = false;
            char stringDelimiter = '\0';
            for (size_t index = openBracketIndex + 1; index < source.size(); ++index)
            {
                const char value = source[index];
                if (inString)
                {
                    if (value == '\\')
                    {
                        ++index;
                    }
                    else if (value == stringDelimiter)
                    {
                        inString = false;
                    }
                    continue;
                }

                if (value == '"' || value == '\'')
                {
                    inString = true;
                    stringDelimiter = value;
                    continue;
                }

                if (value == ']')
                {
                    return index;
                }
            }

            return AZStd::string::npos;
        }

        AZStd::string StripVisibleMetadata(const char* source)
        {
            AZStd::string strippedSource = source;
            size_t index = 0;
            while ((index = strippedSource.find("[Visible", index)) != AZStd::string::npos)
            {
                if (index + 8 < strippedSource.size() && RuntimeIsIdentifierContinue(strippedSource[index + 8]))
                {
                    ++index;
                    continue;
                }

                const size_t metadataEndIndex = RuntimeFindMetadataEnd(strippedSource, index);
                if (metadataEndIndex == AZStd::string::npos)
                {
                    break;
                }

                for (size_t replaceIndex = index; replaceIndex <= metadataEndIndex; ++replaceIndex)
                {
                    strippedSource[replaceIndex] = ' ';
                }

                index = metadataEndIndex + 1;
            }

            return strippedSource;
        }
    }

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
    static void MessageCallback(const asSMessageInfo* message, void*)
    {
        const char* type = "Info";
        if (message->type == asMSGTYPE_WARNING)
        {
            type = "Warning";
        }
        else if (message->type == asMSGTYPE_ERROR)
        {
            type = "Error";
        }

        s_lastCompileMessages += AZStd::string::format(
            "%s (%d, %d): %s: %s\n",
            message->section,
            message->row,
            message->col,
            type,
            message->message);

        AZ_TracePrintf(
            RuntimeLogWindow,
            "%s (%d, %d): %s: %s\n",
            message->section,
            message->row,
            message->col,
            type,
            message->message);
    }

#endif

    AngelScriptRuntime::~AngelScriptRuntime()
    {
        Stop();
    }

    bool AngelScriptRuntime::Start()
    {
#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        if (m_engine)
        {
            return true;
        }

        if (!m_multithreadPrepared)
        {
            const int prepareResult = asPrepareMultithread();
            if (prepareResult < 0)
            {
                AZ_Error(RuntimeLogWindow, false, "Failed to prepare AngelScript multithread support. Result: %d", prepareResult);
                return false;
            }
            m_multithreadPrepared = true;
        }

        m_engine = asCreateScriptEngine();
        if (!m_engine)
        {
            AZ_Error(RuntimeLogWindow, false, "Failed to create AngelScript engine.");
            return false;
        }

        const int result = m_engine->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL);
        if (result < 0)
        {
            AZ_Warning(RuntimeLogWindow, false, "Failed to set AngelScript message callback. Result: %d", result);
        }

        if (!RegisterGameplay(m_engine))
        {
            Stop();
            return false;
        }

        return true;
#else
        m_started = true;
        return false;
#endif
    }

    void AngelScriptRuntime::Stop()
    {
        ReleaseScriptInstances();
#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        if (m_engine)
        {
            if (m_executionContext)
            {
                m_executionContext->Release();
                m_executionContext = nullptr;
                m_executionContextInUse = false;
            }

            m_engine->ShutDownAndRelease();
            m_engine = nullptr;
        }
        if (m_multithreadPrepared)
        {
            asUnprepareMultithread();
            m_multithreadPrepared = false;
        }
        m_behaviorContextInspected = false;
#else
        m_started = false;
        m_behaviorContextInspected = false;
#endif
    }

    bool AngelScriptRuntime::IsStarted() const
    {
#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        return m_engine != nullptr;
#else
        return m_started;
#endif
    }

    bool AngelScriptRuntime::IsSdkAvailable() const
    {
#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        return true;
#else
        return false;
#endif
    }

    AngelScriptRuntime* AngelScriptRuntime::GetCurrentExecutionRuntime()
    {
        return s_currentExecutionRuntime;
    }

    ScriptInstanceId AngelScriptRuntime::GetCurrentExecutionInstanceId()
    {
        return s_currentExecutionInstanceId;
    }

    ScriptCompileResult AngelScriptRuntime::CompileScriptString(const char* moduleName, const char* source)
    {
        if (!moduleName || !moduleName[0])
        {
            return { false, "Module name is empty." };
        }

        if (!source || !source[0])
        {
            return { false, "Script source is empty." };
        }

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        if (!m_engine && !Start())
        {
            return { false, "AngelScript engine is not available." };
        }

        InspectBehaviorContextOnce();

        const AZStd::string strippedSource = StripVisibleMetadata(source);
        s_lastCompileMessages.clear();

        asIScriptModule* module = m_engine->GetModule(moduleName, asGM_ALWAYS_CREATE);
        if (!module)
        {
            return { false, "Failed to create AngelScript module." };
        }

        int result = module->AddScriptSection(moduleName, strippedSource.c_str());
        if (result < 0)
        {
            return { false, AZStd::string::format("Failed to add script section. Result: %d", result) };
        }

        result = module->Build();
        if (result < 0)
        {
            AZStd::string error = AZStd::string::format("Failed to build script module. Result: %d", result);
            if (!s_lastCompileMessages.empty())
            {
                error += "\n";
                error += s_lastCompileMessages;
            }
            return { false, AZStd::move(error) };
        }

        return { true, {} };
#else
        return { false, "AngelScript SDK is not linked." };
#endif
    }

    ScriptCompileResult AngelScriptRuntime::CompileScriptFile(const char* moduleName, const char* scriptPath)
    {
        if (!scriptPath || !scriptPath[0])
        {
            return { false, "Script path is empty." };
        }

        AZ::Outcome<AZStd::string, AZStd::string> readResult = AZ::Utils::ReadFile<AZStd::string>(scriptPath, MaxScriptFileSize);
        if (!readResult.IsSuccess())
        {
            return { false, AZStd::string::format("Failed to read script file '%s': %s", scriptPath, readResult.GetError().c_str()) };
        }

        return CompileScriptString(moduleName, readResult.GetValue().c_str());
    }

    ScriptInstanceResult AngelScriptRuntime::CreateScriptInstance(const char* moduleName, const char* className)
    {
        if (!moduleName || !moduleName[0])
        {
            return { false, "Module name is empty." };
        }

        if (!className || !className[0])
        {
            return { false, "Script class name is empty." };
        }

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        if (!m_engine && !Start())
        {
            return { false, "AngelScript engine is not available." };
        }

        InspectBehaviorContextOnce();

        asIScriptModule* module = m_engine->GetModule(moduleName, asGM_ONLY_IF_EXISTS);
        if (!module)
        {
            return { false, AZStd::string::format("AngelScript module '%s' was not found.", moduleName) };
        }

        asITypeInfo* typeInfo = module->GetTypeInfoByName(className);
        if (!typeInfo)
        {
            return { false, AZStd::string::format("AngelScript class '%s' was not found in module '%s'.", className, moduleName) };
        }

        asIScriptObject* object = reinterpret_cast<asIScriptObject*>(m_engine->CreateScriptObject(typeInfo));
        if (!object)
        {
            return { false, AZStd::string::format("Failed to create AngelScript object '%s'. A public default constructor is required.", className) };
        }

        const ScriptInstanceId instanceId = m_nextScriptInstanceId++;
        ScriptInstance& instance = m_scriptInstances[instanceId];
        instance.m_object = object;
        instance.m_setEntity = typeInfo->GetMethodByDecl("void SetEntity(Entity)");
        instance.m_onActivate = typeInfo->GetMethodByDecl("void OnActivate()");
        instance.m_onDeactivate = typeInfo->GetMethodByDecl("void OnDeactivate()");
        instance.m_onTick = typeInfo->GetMethodByDecl("void OnTick(float)");
        instance.m_onEntityActivated = typeInfo->GetMethodByDecl("void OnEntityActivated(Entity)");

        return
        {
            true,
            {},
            instanceId,
            instance.m_setEntity != nullptr,
            instance.m_onActivate != nullptr,
            instance.m_onDeactivate != nullptr,
            instance.m_onTick != nullptr
        };
#else
        return { false, "AngelScript SDK is not linked." };
#endif
    }

    ScriptCompileResult AngelScriptRuntime::SetScriptInstanceEntityProperty(
        ScriptInstanceId instanceId,
        const char* propertyName,
        AZ::EntityId entityId)
    {
        if (!propertyName || !propertyName[0])
        {
            return { false, "AngelScript Entity property name is empty." };
        }

        auto iterator = m_scriptInstances.find(instanceId);
        if (iterator == m_scriptInstances.end())
        {
            return { false, AZStd::string::format("AngelScript instance '%llu' was not found.", instanceId) };
        }

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        if (!m_engine && !Start())
        {
            return { false, "AngelScript engine is not available." };
        }

        asIScriptObject* object = iterator->second.m_object;
        if (!object)
        {
            return { false, AZStd::string::format("AngelScript instance '%llu' has no script object.", instanceId) };
        }

        asITypeInfo* typeInfo = object->GetObjectType();
        if (!typeInfo)
        {
            return { false, AZStd::string::format("AngelScript instance '%llu' has no object type.", instanceId) };
        }

        const int entityTypeId = m_engine->GetTypeIdByDecl("Entity");
        if (entityTypeId < 0)
        {
            return { false, AZStd::string::format("AngelScript Entity type is not registered. Result: %d", entityTypeId) };
        }

        for (asUINT propertyIndex = 0; propertyIndex < typeInfo->GetPropertyCount(); ++propertyIndex)
        {
            const char* scriptPropertyName = nullptr;
            int scriptPropertyTypeId = 0;
            const int propertyResult = typeInfo->GetProperty(propertyIndex, &scriptPropertyName, &scriptPropertyTypeId);
            if (propertyResult < 0)
            {
                return { false, AZStd::string::format("Failed to inspect AngelScript property '%s'. Result: %d", propertyName, propertyResult) };
            }

            if (!scriptPropertyName || AZStd::string_view(scriptPropertyName) != propertyName)
            {
                continue;
            }

            if (scriptPropertyTypeId != entityTypeId)
            {
                return
                {
                    false,
                    AZStd::string::format(
                        "AngelScript property '%s' exists but is not Entity. Expected type id %d, got %d.",
                        propertyName,
                        entityTypeId,
                        scriptPropertyTypeId)
                };
            }

            void* propertyAddress = object->GetAddressOfProperty(propertyIndex);
            if (!propertyAddress)
            {
                return { false, AZStd::string::format("Failed to access AngelScript Entity property '%s'.", propertyName) };
            }

            auto* scriptEntity = reinterpret_cast<ScriptEntity*>(propertyAddress);
            scriptEntity->m_entityId = aznumeric_cast<AZ::u64>(entityId);
            return { true, {} };
        }

        return { false, AZStd::string::format("AngelScript Entity property '%s' was not found.", propertyName) };
#else
        (void)entityId;
        return { false, "AngelScript SDK is not linked." };
#endif
    }

    ScriptCompileResult AngelScriptRuntime::ExecuteScriptInstanceSetEntity(ScriptInstanceId instanceId, AZ::EntityId entityId)
    {
        auto iterator = m_scriptInstances.find(instanceId);
        if (iterator == m_scriptInstances.end())
        {
            return { false, AZStd::string::format("AngelScript instance '%llu' was not found.", instanceId) };
        }

        return ExecuteScriptFunction(instanceId, iterator->second, iterator->second.m_setEntity, "void SetEntity(Entity)", &entityId);
    }

    ScriptCompileResult AngelScriptRuntime::ExecuteScriptInstanceOnActivate(ScriptInstanceId instanceId)
    {
        auto iterator = m_scriptInstances.find(instanceId);
        if (iterator == m_scriptInstances.end())
        {
            return { false, AZStd::string::format("AngelScript instance '%llu' was not found.", instanceId) };
        }

        return ExecuteScriptFunction(instanceId, iterator->second, iterator->second.m_onActivate, "void OnActivate()");
    }

    ScriptCompileResult AngelScriptRuntime::ExecuteScriptInstanceOnDeactivate(ScriptInstanceId instanceId)
    {
        auto iterator = m_scriptInstances.find(instanceId);
        if (iterator == m_scriptInstances.end())
        {
            return { false, AZStd::string::format("AngelScript instance '%llu' was not found.", instanceId) };
        }

        return ExecuteScriptFunction(instanceId, iterator->second, iterator->second.m_onDeactivate, "void OnDeactivate()");
    }

    ScriptCompileResult AngelScriptRuntime::ExecuteScriptInstanceOnTick(ScriptInstanceId instanceId, float deltaTime)
    {
        auto iterator = m_scriptInstances.find(instanceId);
        if (iterator == m_scriptInstances.end())
        {
            return { false, AZStd::string::format("AngelScript instance '%llu' was not found.", instanceId) };
        }

        return ExecuteScriptFunction(instanceId, iterator->second, iterator->second.m_onTick, "void OnTick(float)", nullptr, &deltaTime);
    }

    ScriptCompileResult AngelScriptRuntime::ConnectEntityBus(ScriptInstanceId instanceId, AZ::EntityId entityId)
    {
        auto iterator = m_scriptInstances.find(instanceId);
        if (iterator == m_scriptInstances.end())
        {
            return { false, AZStd::string::format("AngelScript instance '%llu' was not found.", instanceId) };
        }

        ScriptInstance& instance = iterator->second;
        if (!instance.m_onEntityActivated)
        {
            return { false, "EntityBus.Connect requires script method 'void OnEntityActivated(Entity)'." };
        }

        if (!instance.m_entityBusBinding)
        {
            instance.m_entityBusBinding = AZStd::make_unique<EntityBusBinding>(*this, instanceId);
        }

        return instance.m_entityBusBinding->Connect(entityId);
    }

    ScriptCompileResult AngelScriptRuntime::DisconnectEntityBus(ScriptInstanceId instanceId, AZ::EntityId entityId)
    {
        auto iterator = m_scriptInstances.find(instanceId);
        if (iterator == m_scriptInstances.end())
        {
            return { false, AZStd::string::format("AngelScript instance '%llu' was not found.", instanceId) };
        }

        ScriptInstance& instance = iterator->second;
        if (!instance.m_entityBusBinding)
        {
            return { true, {} };
        }

        return instance.m_entityBusBinding->Disconnect(entityId);
    }

    void AngelScriptRuntime::DestroyScriptInstance(ScriptInstanceId instanceId)
    {
        auto iterator = m_scriptInstances.find(instanceId);
        if (iterator == m_scriptInstances.end())
        {
            return;
        }

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        iterator->second.m_entityBusBinding.reset();
        if (iterator->second.m_object)
        {
            iterator->second.m_object->Release();
        }
#endif
        m_scriptInstances.erase(iterator);
    }

    ScriptCompileResult AngelScriptRuntime::ExecuteScriptFunction(
        ScriptInstanceId instanceId,
        ScriptInstance& instance,
        asIScriptFunction* function,
        const char* methodDeclaration,
        AZ::EntityId* entityId,
        float* deltaTime)
    {
        if (!function)
        {
            return { false, AZStd::string::format("AngelScript method '%s' was not found.", methodDeclaration) };
        }

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        if (!m_engine && !Start())
        {
            return { false, "AngelScript engine is not available." };
        }

        bool releaseContextAfterUse = false;
        asIScriptContext* context = AcquireExecutionContext(releaseContextAfterUse);
        if (!context)
        {
            return { false, "Failed to create AngelScript execution context." };
        }

        AngelScriptRuntime* previousRuntime = s_currentExecutionRuntime;
        const ScriptInstanceId previousInstanceId = s_currentExecutionInstanceId;
        s_currentExecutionRuntime = this;
        s_currentExecutionInstanceId = instanceId;

        int result = context->Prepare(function);
        if (result < 0)
        {
            s_currentExecutionRuntime = previousRuntime;
            s_currentExecutionInstanceId = previousInstanceId;
            ReleaseExecutionContext(context, releaseContextAfterUse);
            return { false, AZStd::string::format("Failed to prepare AngelScript method '%s'. Result: %d", methodDeclaration, result) };
        }

        result = context->SetObject(instance.m_object);
        if (result < 0)
        {
            s_currentExecutionRuntime = previousRuntime;
            s_currentExecutionInstanceId = previousInstanceId;
            ReleaseExecutionContext(context, releaseContextAfterUse);
            return { false, AZStd::string::format("Failed to set AngelScript method object for '%s'. Result: %d", methodDeclaration, result) };
        }

        if (entityId)
        {
            ScriptEntity scriptEntity;
            scriptEntity.m_entityId = aznumeric_cast<AZ::u64>(*entityId);

            result = context->SetArgObject(0, &scriptEntity);
            if (result < 0)
            {
                s_currentExecutionRuntime = previousRuntime;
                s_currentExecutionInstanceId = previousInstanceId;
                ReleaseExecutionContext(context, releaseContextAfterUse);
                return { false, AZStd::string::format("Failed to set AngelScript Entity argument. Result: %d", result) };
            }
        }
        else if (deltaTime)
        {
            result = context->SetArgFloat(0, *deltaTime);
            if (result < 0)
            {
                s_currentExecutionRuntime = previousRuntime;
                s_currentExecutionInstanceId = previousInstanceId;
                ReleaseExecutionContext(context, releaseContextAfterUse);
                return { false, AZStd::string::format("Failed to set AngelScript OnTick deltaTime argument. Result: %d", result) };
            }
        }

        result = context->Execute();
        s_currentExecutionRuntime = previousRuntime;
        s_currentExecutionInstanceId = previousInstanceId;
        if (result != asEXECUTION_FINISHED)
        {
            AZStd::string error = AZStd::string::format("AngelScript method '%s' failed. Result: %d", methodDeclaration, result);
            if (result == asEXECUTION_EXCEPTION)
            {
                error = AZStd::string::format("%s Exception: %s", error.c_str(), context->GetExceptionString());
            }

            ReleaseExecutionContext(context, releaseContextAfterUse);
            return { false, error };
        }

        ReleaseExecutionContext(context, releaseContextAfterUse);
        return { true, {} };
#else
        return { false, "AngelScript SDK is not linked." };
#endif
    }

    ScriptCompileResult AngelScriptRuntime::ExecuteScriptInstanceOnEntityActivated(ScriptInstanceId instanceId, AZ::EntityId entityId)
    {
        auto iterator = m_scriptInstances.find(instanceId);
        if (iterator == m_scriptInstances.end())
        {
            return { false, AZStd::string::format("AngelScript instance '%llu' was not found.", instanceId) };
        }

        return ExecuteScriptFunction(
            instanceId,
            iterator->second,
            iterator->second.m_onEntityActivated,
            "void OnEntityActivated(Entity)",
            &entityId);
    }

    asIScriptContext* AngelScriptRuntime::AcquireExecutionContext(bool& releaseAfterUse)
    {
        releaseAfterUse = false;
#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        if (m_executionContextInUse)
        {
            releaseAfterUse = true;
            return m_engine ? m_engine->CreateContext() : nullptr;
        }

        if (!m_executionContext)
        {
            m_executionContext = m_engine ? m_engine->CreateContext() : nullptr;
            if (!m_executionContext)
            {
                return nullptr;
            }
        }

        m_executionContextInUse = true;
        return m_executionContext;
#else
        return nullptr;
#endif
    }

    void AngelScriptRuntime::ReleaseExecutionContext(asIScriptContext* context, bool releaseAfterUse)
    {
#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        if (!context)
        {
            return;
        }

        context->Unprepare();
        if (releaseAfterUse)
        {
            context->Release();
        }
        else
        {
            m_executionContextInUse = false;
        }
#else
        (void)context;
        (void)releaseAfterUse;
#endif
    }

    void AngelScriptRuntime::ReleaseScriptInstances()
    {
#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        for (auto& scriptInstancePair : m_scriptInstances)
        {
            scriptInstancePair.second.m_entityBusBinding.reset();
            if (scriptInstancePair.second.m_object)
            {
                scriptInstancePair.second.m_object->Release();
            }
        }
#endif
        m_scriptInstances.clear();
    }

    void AngelScriptRuntime::InspectBehaviorContextOnce()
    {
        if (m_behaviorContextInspected)
        {
            return;
        }

        m_behaviorContextInspected = true;
        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            behaviorContext,
            &AZ::ComponentApplicationBus::Events::GetBehaviorContext);

        if (behaviorContext == nullptr)
        {
            AZ_Warning(RuntimeLogWindow, false, "BehaviorContextInspector: BehaviorContext is not available.");
            return;
        }

        BehaviorContextInspector inspector;
        inspector.InspectAndLog(*behaviorContext);
    }
} // namespace AngelScript
