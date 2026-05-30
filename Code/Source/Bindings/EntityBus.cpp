#include <Bindings/EntityBus.h>

#include <Bindings/Entity.h>
#include <Runtime/AngelScriptRuntime.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
#include <angelscript.h>
#endif

namespace AngelScript
{
    namespace
    {
        constexpr const char* EntityBusBindingLogWindow = "AngelScript";

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        void ScriptEntityBusConnect(asIScriptGeneric* generic)
        {
            const auto* scriptEntity = reinterpret_cast<const ScriptEntity*>(generic->GetArgObject(0));
            const AZ::EntityId entityId = scriptEntity ? AZ::EntityId(aznumeric_cast<AZ::u64>(scriptEntity->m_entityId)) : AZ::EntityId();

            AngelScriptRuntime* runtime = AngelScriptRuntime::GetCurrentExecutionRuntime();
            if (!runtime)
            {
                AZ_Warning(EntityBusBindingLogWindow, false, "EntityBus.Connect must be called from an active AngelScript instance.");
                generic->SetReturnByte(false);
                return;
            }

            const ScriptCompileResult result = runtime->ConnectEntityBus(AngelScriptRuntime::GetCurrentExecutionInstanceId(), entityId);
            if (!result.m_success)
            {
                AZ_Warning(EntityBusBindingLogWindow, false, "EntityBus.Connect failed: %s", result.m_error.c_str());
            }

            generic->SetReturnByte(result.m_success);
        }

        void ScriptEntityBusDisconnect(asIScriptGeneric* generic)
        {
            const auto* scriptEntity = reinterpret_cast<const ScriptEntity*>(generic->GetArgObject(0));
            const AZ::EntityId entityId = scriptEntity ? AZ::EntityId(aznumeric_cast<AZ::u64>(scriptEntity->m_entityId)) : AZ::EntityId();

            AngelScriptRuntime* runtime = AngelScriptRuntime::GetCurrentExecutionRuntime();
            if (!runtime)
            {
                AZ_Warning(EntityBusBindingLogWindow, false, "EntityBus.Disconnect must be called from an active AngelScript instance.");
                generic->SetReturnByte(false);
                return;
            }

            const ScriptCompileResult result = runtime->DisconnectEntityBus(AngelScriptRuntime::GetCurrentExecutionInstanceId(), entityId);
            if (!result.m_success)
            {
                AZ_Warning(EntityBusBindingLogWindow, false, "EntityBus.Disconnect failed: %s", result.m_error.c_str());
            }

            generic->SetReturnByte(result.m_success);
        }
#endif
    } // namespace

    bool RegisterEntityBus(asIScriptEngine* engine)
    {
#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        if (!engine)
        {
            AZ_Error(EntityBusBindingLogWindow, false, "Cannot register AngelScript EntityBus bindings without an engine.");
            return false;
        }

        engine->SetDefaultNamespace("EntityBus");

        int result = engine->RegisterGlobalFunction("bool Connect(const Entity &in)", asFUNCTION(ScriptEntityBusConnect), asCALL_GENERIC);
        if (result < 0)
        {
            engine->SetDefaultNamespace("");
            AZ_Error(EntityBusBindingLogWindow, false, "Failed to register AngelScript EntityBus.Connect. Result: %d", result);
            return false;
        }

        result = engine->RegisterGlobalFunction("bool Disconnect(const Entity &in)", asFUNCTION(ScriptEntityBusDisconnect), asCALL_GENERIC);
        engine->SetDefaultNamespace("");
        if (result < 0)
        {
            AZ_Error(EntityBusBindingLogWindow, false, "Failed to register AngelScript EntityBus.Disconnect. Result: %d", result);
            return false;
        }

        return true;
#else
        (void)engine;
        return false;
#endif
    }
} // namespace AngelScript
