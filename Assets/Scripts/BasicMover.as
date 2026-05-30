class BasicMover
{
    Entity owner;
    [Visible(Tooltip="Entity to follow")]
    Entity target;
    Vec3 velocity;
    Vec3 startPosition;
    Vec3 lastPosition;

    float elapsedTime;
    float accumulatedDistanceZ;
    int tickCount;
    bool warnedAboutInvalidEntity;
    bool loggedLargeDeltaTime;
    bool loggedZeroDeltaTime;
    bool loggedTargetState;
    bool loggedPositionRoundTrip;

    void SetEntity(Entity entity)
    {
        owner = entity;
        elapsedTime = 0.0f;
        accumulatedDistanceZ = 0.0f;
        tickCount = 0;
        warnedAboutInvalidEntity = false;
        loggedLargeDeltaTime = false;
        loggedZeroDeltaTime = false;
        loggedTargetState = false;
        loggedPositionRoundTrip = false;

        velocity.x = 1.0f;
        velocity.y = 0.0f;
        velocity.z = 4.0f;

        Debug::Log("BasicMover.SetEntity");
        if (owner.IsValid())
        {
            Debug::Log("BasicMover entity: " + owner.GetName());
        }
        else
        {
            Debug::Warning("BasicMover received an invalid entity");
        }
    }

    void OnActivate()
    {
        Debug::Log("BasicMover.OnActivate");

        if (!owner.IsValid())
        {
            Debug::Error("BasicMover cannot activate because Entity is invalid");
            return;
        }

        startPosition = owner.GetWorldPosition();
        lastPosition = startPosition;

        if (!loggedTargetState)
        {
            if (target.IsValid())
            {
                Debug::Log("BasicMover serialized target: " + target.GetName());
            }
            else
            {
                Debug::Log("BasicMover has no serialized target assigned");
            }
            loggedTargetState = true;
        }

        Vec3 addCheck = velocity + velocity;
        Vec3 subCheck = addCheck - velocity;
        Vec3 mulCheck = velocity * 0.5f;
        Vec3 reflectedMulCheck = 0.5f * velocity;

        Debug::Log("BasicMover captured start position");

        if (subCheck.z > 0.0f && mulCheck.z > 0.0f && reflectedMulCheck.z > 0.0f)
        {
            Debug::Log("BasicMover Vec3 add/sub/mul checks passed");
        }
        else
        {
            Debug::Warning("BasicMover Vec3 operator check produced an unexpected value");
        }

        owner.SetWorldPosition(startPosition);
        Vec3 roundTripPosition = owner.GetWorldPosition();
        if (!loggedPositionRoundTrip && roundTripPosition.z == startPosition.z)
        {
            Debug::Log("BasicMover position round-trip check passed");
            loggedPositionRoundTrip = true;
        }
    }

    void OnTick(float deltaTime)
    {
        if (!owner.IsValid())
        {
            if (!warnedAboutInvalidEntity)
            {
                Debug::Warning("BasicMover.OnTick skipped because Entity is invalid");
                warnedAboutInvalidEntity = true;
            }
            return;
        }

        if (deltaTime <= 0.0f)
        {
            if (!loggedZeroDeltaTime)
            {
                Debug::Warning("BasicMover received non-positive deltaTime");
                loggedZeroDeltaTime = true;
            }
            return;
        }

        if (deltaTime > 0.5f && !loggedLargeDeltaTime)
        {
            Debug::Warning("BasicMover received unusually large deltaTime");
            loggedLargeDeltaTime = true;
        }

        elapsedTime += deltaTime;
        tickCount += 1;

        Vec3 currentPosition = owner.GetWorldPosition();
        Vec3 movement = velocity * deltaTime;
        Vec3 newPosition = currentPosition + movement;
        owner.SetWorldPosition(newPosition);

        accumulatedDistanceZ += newPosition.z - lastPosition.z;
        lastPosition = newPosition;

        if (target.IsValid() && elapsedTime > 0.25f && elapsedTime < 0.5f)
        {
            Vec3 targetPosition = target.GetWorldPosition();
            Vec3 targetOffset = targetPosition - newPosition;
            if (targetOffset.x != 0.0f || targetOffset.y != 0.0f || targetOffset.z != 0.0f)
            {
                Debug::Log("BasicMover target offset sampled");
            }
        }

        if (elapsedTime > 1.0f)
        {
            Vec3 offset = newPosition - startPosition;
            if (offset.z > 0.0f)
            {
                Debug::Log("BasicMover moved entity upward");
            }

            if (accumulatedDistanceZ <= 0.0f)
            {
                Debug::Warning("BasicMover accumulated Z movement is not positive");
            }

            if (tickCount <= 0)
            {
                Debug::Warning("BasicMover tick counter did not advance");
            }

            elapsedTime = 0.0f;
        }
    }

    void OnDeactivate()
    {
        Debug::Log("BasicMover.OnDeactivate");
    }
}
