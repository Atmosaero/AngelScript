#include <Runtime/EntityBusBinding.h>

#include <Runtime/AngelScriptRuntime.h>

#include <AzCore/Debug/Trace.h>

namespace AngelScript
{
    namespace
    {
        constexpr const char* EntityBusLogWindow = "AngelScript";
    }

    EntityBusBinding::EntityBusBinding(AngelScriptRuntime& runtime, ScriptInstanceId instanceId)
        : m_runtime(runtime)
        , m_instanceId(instanceId)
    {
    }

    EntityBusBinding::~EntityBusBinding()
    {
        DisconnectAll();
    }

    ScriptCompileResult EntityBusBinding::Connect(AZ::EntityId entityId)
    {
        if (!entityId.IsValid())
        {
            return { false, "EntityBus.Connect requires a valid Entity." };
        }

        if (m_connectedEntityIds.contains(entityId))
        {
            return { true, {} };
        }

        AZ::EntityBus::MultiHandler::BusConnect(entityId);
        m_connectedEntityIds.insert(entityId);
        return { true, {} };
    }

    ScriptCompileResult EntityBusBinding::Disconnect(AZ::EntityId entityId)
    {
        if (!entityId.IsValid())
        {
            return { false, "EntityBus.Disconnect requires a valid Entity." };
        }

        if (!m_connectedEntityIds.contains(entityId))
        {
            return { true, {} };
        }

        AZ::EntityBus::MultiHandler::BusDisconnect(entityId);
        m_connectedEntityIds.erase(entityId);
        return { true, {} };
    }

    void EntityBusBinding::DisconnectAll()
    {
        if (!m_connectedEntityIds.empty())
        {
            AZ::EntityBus::MultiHandler::BusDisconnect();
            m_connectedEntityIds.clear();
        }
    }

    void EntityBusBinding::OnEntityActivated(const AZ::EntityId& entityId)
    {
        const ScriptCompileResult result = m_runtime.ExecuteScriptInstanceOnEntityActivated(m_instanceId, entityId);
        if (!result.m_success)
        {
            AZ_Warning(EntityBusLogWindow, false, "Failed to execute OnEntityActivated(Entity): %s", result.m_error.c_str());
        }
    }
} // namespace AngelScript
