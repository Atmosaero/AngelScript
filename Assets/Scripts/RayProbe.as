class RayProbe
{
    Entity owner;

    [Visible(Tooltip="Expected entity that should be hit by the ray")]
    Entity expectedHit;

    [Visible(Tooltip="Local offset from owner world position used as ray origin")]
    Vec3 originOffset;

    [Visible(Tooltip="Origin offset velocity in world units per second")]
    Vec3 originVelocity;

    [Visible(Tooltip="Ray direction in world space; native CastRay normalizes it")]
    Vec3 rayDirection = Vec3(1.0f, 0.0f, 0.0f);

    [Visible(Tooltip="Ray length in meters")]
    float rayDistance = 10.0f;

    [Visible(Tooltip="Size of the hit rectangle marker")]
    float hitMarkerSize = 0.25f;

    [Visible(Tooltip="If true, missing hit is treated as an error")]
    bool requireHit = true;

    [Visible(Tooltip="If true, mismatch between expectedHit and actual hit is treated as an error")]
    bool requireExpectedEntity = false;

    [Visible(Tooltip="If true, keep drawing every tick; otherwise draw only on the first probe tick")]
    bool drawEveryTick = true;

    bool activationComplete;
    bool probeFinished;
    bool loggedInvalidOwner;
    Vec3 currentOriginOffset;

    string FormatVec3(const Vec3 &in value)
    {
        return "(" + value.x + ", " + value.y + ", " + value.z + ")";
    }

    string DescribeEntity(Entity entity)
    {
        if (!entity.IsValid())
        {
            return "<invalid>";
        }

        string name = entity.GetName();
        if (name == "")
        {
            return "<unnamed>";
        }

        return name;
    }

    void SetEntity(Entity entity)
    {
        owner = entity;
        activationComplete = false;
        probeFinished = false;
        loggedInvalidOwner = false;
        currentOriginOffset = originOffset;
    }

    void OnActivate()
    {
        Debug::Log("RayProbe.OnActivate");

        if (!owner.IsValid())
        {
            Debug::Warning("RayProbe owner is invalid");
            return;
        }

        currentOriginOffset = originOffset;
        activationComplete = true;
    }

    void RunProbe()
    {
        Vec3 start = owner.GetWorldPosition() + currentOriginOffset;
        Vec3 end = start + rayDirection * rayDistance;
        Ray ray(start, rayDirection, rayDistance);
        TraceHit hit = Physics::LineTrace(
            start,
            end,
            TraceChannel::Visibility,
            TraceQuery()
                .Ignore(owner)
                .DrawDebug(DebugTrace::ForDuration, 0.1f));

        Color rayColor(1.0f, 0.55f, 0.1f, 1.0f);
        Color hitColor(0.1f, 1.0f, 0.3f, 1.0f);
        Color missColor(1.0f, 0.2f, 0.2f, 1.0f);

        if (hit.hit)
        {
            Debug::DrawRay(ray, rayColor);
            Debug::DrawHitRect(hit, hitMarkerSize, hitColor);
        }
        else
        {
            Debug::DrawRay(ray, missColor);
        }

        if (!probeFinished)
        {
            if (requireHit && !hit.hit)
            {
                Debug::Error(
                    "RayProbe expected a hit but got none origin=" +
                    FormatVec3(start) +
                    " direction=" +
                    FormatVec3(rayDirection) +
                    " distance=" +
                    rayDistance);
                probeFinished = true;
                return;
            }

            if (!requireHit && !hit.hit)
            {
                Debug::Log("RayProbe no hit, which is allowed by current settings");
                probeFinished = true;
                return;
            }

            if (requireExpectedEntity)
            {
                if (!expectedHit.IsValid())
                {
                    Debug::Error("RayProbe requireExpectedEntity is true but expectedHit is invalid");
                    probeFinished = true;
                    return;
                }

                if (!hit.entity.IsValid() || hit.entity.GetName() != expectedHit.GetName())
                {
                    Debug::Error(
                        "RayProbe hit entity mismatch expected=" +
                        DescribeEntity(expectedHit) +
                        " actual=" +
                        DescribeEntity(hit.entity));
                    probeFinished = true;
                    return;
                }
            }

            Debug::Log(
                "RayProbe passed hitEntity=" +
                DescribeEntity(hit.entity) +
                " hitPosition=" +
                FormatVec3(hit.position) +
                " hitNormal=" +
                FormatVec3(hit.normal) +
                " hitDistance=" +
                hit.distance);
            probeFinished = true;
        }
    }

    void OnTick(float deltaTime)
    {
        if (!activationComplete)
        {
            return;
        }

        if (!owner.IsValid())
        {
            if (!loggedInvalidOwner)
            {
                Debug::Warning("RayProbe owner became invalid");
                loggedInvalidOwner = true;
            }
            return;
        }

        if (!drawEveryTick && probeFinished)
        {
            return;
        }

        currentOriginOffset = currentOriginOffset + originVelocity * deltaTime;
        RunProbe();
    }

    void OnDeactivate()
    {
        Debug::Log("RayProbe.OnDeactivate");
    }
}
