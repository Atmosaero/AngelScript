class EntityBusProbe
{
    [Visible(Tooltip="Entity to observe for activation")]
    Entity target;

    bool connected;

    void OnActivate()
    {
        Debug::Log("EntityBusProbe.OnActivate");

        if (!target.IsValid())
        {
            Debug::Warning("EntityBusProbe target is invalid");
            return;
        }

        connected = EntityBus::Connect(target);
        if (connected)
        {
            Debug::Log("EntityBusProbe connected to target");
        }
    }

    void OnDeactivate()
    {
        if (connected && target.IsValid())
        {
            EntityBus::Disconnect(target);
            Debug::Log("EntityBusProbe disconnected from target");
        }

        connected = false;
    }

    void OnEntityActivated(Entity entity)
    {
        string entityName = entity.GetName();
        if (entityName == "")
        {
            Debug::Log("EntityBusProbe.OnEntityActivated: valid entity with empty name");
            return;
        }

        Debug::Log("EntityBusProbe.OnEntityActivated: " + entityName);
    }
}
