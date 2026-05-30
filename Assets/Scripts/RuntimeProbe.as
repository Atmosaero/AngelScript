class RuntimeProbe
{
    Entity owner;
    [Visible(Tooltip="Primary serialized entity reference")]
    Entity target;
    [Visible(Tooltip="Secondary serialized entity reference")]
    Entity alternate;

    bool activationComplete;
    bool loggedFirstTick;

    void LogEntityState(const string &in label, Entity entity)
    {
        if (!entity.IsValid())
        {
            Debug::Warning("RuntimeProbe " + label + " is invalid");
            return;
        }

        string name = entity.GetName();
        if (name == "")
        {
            Debug::Warning("RuntimeProbe " + label + " is valid but GetName() returned an empty string");
        }
        else
        {
            Debug::Log("RuntimeProbe " + label + " name: " + name);
        }
    }

    void SetEntity(Entity entity)
    {
        owner = entity;
        activationComplete = false;
        loggedFirstTick = false;

        Debug::Log("RuntimeProbe.SetEntity");
        LogEntityState("owner", owner);
    }

    void OnActivate()
    {
        Debug::Log("RuntimeProbe.OnActivate");
        activationComplete = true;

        LogEntityState("owner", owner);
        LogEntityState("target", target);
        LogEntityState("alternate", alternate);
    }

    void OnTick(float deltaTime)
    {
        if (!activationComplete)
        {
            Debug::Warning("RuntimeProbe.OnTick before OnActivate");
            return;
        }

        if (!loggedFirstTick)
        {
            Debug::Log("RuntimeProbe first tick deltaTime: " + deltaTime);
            LogEntityState("owner on tick", owner);
            LogEntityState("target on tick", target);
            LogEntityState("alternate on tick", alternate);
            loggedFirstTick = true;
        }
    }

    void OnDeactivate()
    {
        Debug::Log("RuntimeProbe.OnDeactivate");
    }
}
