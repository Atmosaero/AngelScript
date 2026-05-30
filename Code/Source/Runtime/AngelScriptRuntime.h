#pragma once

#include <AngelScript/AngelScriptBus.h>
#include <Runtime/EntityBusBinding.h>

#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/unordered_map.h>

class asIScriptEngine;
class asIScriptContext;
class asIScriptFunction;
class asIScriptObject;

namespace AngelScript
{
    class AngelScriptRuntime
    {
    public:
        AngelScriptRuntime() = default;
        ~AngelScriptRuntime();

        AngelScriptRuntime(const AngelScriptRuntime&) = delete;
        AngelScriptRuntime& operator=(const AngelScriptRuntime&) = delete;

        bool Start();
        void Stop();

        bool IsStarted() const;
        bool IsSdkAvailable() const;
        static AngelScriptRuntime* GetCurrentExecutionRuntime();
        static ScriptInstanceId GetCurrentExecutionInstanceId();

        ScriptCompileResult CompileScriptString(const char* moduleName, const char* source);
        ScriptCompileResult CompileScriptFile(const char* moduleName, const char* scriptPath);
        ScriptInstanceResult CreateScriptInstance(const char* moduleName, const char* className);
        ScriptCompileResult SetScriptInstanceEntityProperty(ScriptInstanceId instanceId, const char* propertyName, AZ::EntityId entityId);
        ScriptCompileResult ExecuteScriptInstanceSetEntity(ScriptInstanceId instanceId, AZ::EntityId entityId);
        ScriptCompileResult ExecuteScriptInstanceOnActivate(ScriptInstanceId instanceId);
        ScriptCompileResult ExecuteScriptInstanceOnDeactivate(ScriptInstanceId instanceId);
        ScriptCompileResult ExecuteScriptInstanceOnTick(ScriptInstanceId instanceId, float deltaTime);
        ScriptCompileResult ConnectEntityBus(ScriptInstanceId instanceId, AZ::EntityId entityId);
        ScriptCompileResult DisconnectEntityBus(ScriptInstanceId instanceId, AZ::EntityId entityId);
        void DestroyScriptInstance(ScriptInstanceId instanceId);

    private:
        friend class EntityBusBinding;

        struct ScriptInstance
        {
            asIScriptObject* m_object = nullptr;
            asIScriptFunction* m_setEntity = nullptr;
            asIScriptFunction* m_onActivate = nullptr;
            asIScriptFunction* m_onDeactivate = nullptr;
            asIScriptFunction* m_onTick = nullptr;
            asIScriptFunction* m_onEntityActivated = nullptr;
            AZStd::unique_ptr<EntityBusBinding> m_entityBusBinding;
        };

        ScriptCompileResult ExecuteScriptFunction(
            ScriptInstanceId instanceId,
            ScriptInstance& instance,
            asIScriptFunction* function,
            const char* methodDeclaration,
            AZ::EntityId* entityId = nullptr,
            float* deltaTime = nullptr);
        ScriptCompileResult ExecuteScriptInstanceOnEntityActivated(ScriptInstanceId instanceId, AZ::EntityId entityId);
        asIScriptContext* AcquireExecutionContext(bool& releaseAfterUse);
        void ReleaseExecutionContext(asIScriptContext* context, bool releaseAfterUse);
        void ReleaseScriptInstances();
        void InspectBehaviorContextOnce();

        asIScriptEngine* m_engine = nullptr;
        asIScriptContext* m_executionContext = nullptr;
        AZStd::unordered_map<ScriptInstanceId, ScriptInstance> m_scriptInstances;
        ScriptInstanceId m_nextScriptInstanceId = 1;
        bool m_executionContextInUse = false;
        bool m_multithreadPrepared = false;
        bool m_behaviorContextInspected = false;
        bool m_started = false;

        static thread_local AngelScriptRuntime* s_currentExecutionRuntime;
        static thread_local ScriptInstanceId s_currentExecutionInstanceId;
    };
} // namespace AngelScript
