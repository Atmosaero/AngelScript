#pragma once

#include <AngelScript/AngelScriptBus.h>

#include <AzCore/Component/EntityBus.h>
#include <AzCore/std/containers/unordered_set.h>

namespace AngelScript
{
    class AngelScriptRuntime;

    class EntityBusBinding
        : private AZ::EntityBus::MultiHandler
    {
    public:
        EntityBusBinding(AngelScriptRuntime& runtime, ScriptInstanceId instanceId);
        ~EntityBusBinding() override;

        ScriptCompileResult Connect(AZ::EntityId entityId);
        ScriptCompileResult Disconnect(AZ::EntityId entityId);
        void DisconnectAll();

    private:
        void OnEntityActivated(const AZ::EntityId& entityId) override;

        AngelScriptRuntime& m_runtime;
        ScriptInstanceId m_instanceId = InvalidScriptInstanceId;
        AZStd::unordered_set<AZ::EntityId> m_connectedEntityIds;
    };
} // namespace AngelScript
