class TransformBusProbe
{
    Entity owner;

    [Visible(Tooltip="Entity moved through BehaviorContext TransformBus")]
    Entity target;

    [Visible(Tooltip="Entity with AttachmentComponent used to verify GetOffset")]
    Entity attachmentTarget;

    [Visible(Tooltip="Expected AttachmentComponentBus::GetOffset translation")]
    Vec3 expectedAttachmentOffset;

    [Visible(Tooltip="Enable AttachmentComponentBus::GetOffset runtime check")]
    bool verifyAttachmentOffset = false;

    [Visible(Tooltip="Entity with HeightfieldProviderRequestsBus used to verify GetHeightfieldTransform")]
    Entity heightfieldTarget;

    [Visible(Tooltip="Expected HeightfieldProviderRequestsBus::GetHeightfieldTransform translation")]
    Vec3 expectedHeightfieldTranslation;

    [Visible(Tooltip="Enable HeightfieldProviderRequestsBus::GetHeightfieldTransform runtime check")]
    bool verifyHeightfieldTransform = false;

    [Visible(Tooltip="Viewport id used to verify ViewportBus::GetCameraTransform")]
    int viewportId = 0;

    [Visible(Tooltip="Enable ViewportBus::GetCameraTransform runtime check")]
    bool verifyViewportCameraTransform = true;

    Vec3 startPosition;
    Quat startRotation;
    Transform startLocalTransform;
    Transform startWorldTransform;
    float elapsed = 0.0f;
    float roundTripRequestTime = 0.0f;
    float rotationRoundTripRequestTime = 0.0f;
    float localTransformRoundTripRequestTime = 0.0f;
    float worldTransformRoundTripRequestTime = 0.0f;
    float localTranslationRoundTripRequestTime = 0.0f;
    bool hasTarget = false;
    bool reportedRoundTrip = false;
    bool requestedRoundTrip = false;
    bool reportedRotationRoundTrip = false;
    bool requestedRotationRoundTrip = false;
    bool reportedTransformAccessRoundTrip = false;
    bool reportedHierarchyArrayRoundTrip = false;
    bool reportedLocalTransformRoundTrip = false;
    bool requestedLocalTransformRoundTrip = false;
    bool reportedWorldTransformRoundTrip = false;
    bool requestedWorldTransformRoundTrip = false;
    bool reportedLocalTranslationRoundTrip = false;
    bool requestedLocalTranslationRoundTrip = false;
    bool reportedCameraReady = false;
    bool reportedCameraFailure = false;
    bool reportedViewportTransformRoundTrip = false;
    bool reportedViewportTransformFailure = false;
    bool reportedAttachmentOffsetRoundTrip = false;
    bool reportedHeightfieldTransformRoundTrip = false;
    bool reportedConsoleCommandRoundTrip = false;
    bool reportedEntityNameRoundTrip = false;
    bool reportedTickDeltaRoundTrip = false;
    bool reportedScalarWorldPositionRoundTrip = false;
    bool reportedQuatConstructorRoundTrip = false;
    bool reportedLocalTransformFailureDetails = false;

    void SetEntity(Entity entity)
    {
        owner = entity;
    }

    void OnActivate()
    {
        Debug::Log("TransformBusProbe.OnActivate");

        if (!reportedConsoleCommandRoundTrip)
        {
            ConsoleBus::ExecuteConsoleCommand("");
            Debug::Log("TransformBusProbe ConsoleBus::ExecuteConsoleCommand passed");
            reportedConsoleCommandRoundTrip = true;
        }

        if (owner.IsValid())
        {
            string behaviorContextName = ComponentApplicationBus::GetEntityName(owner);
            string facadeName = owner.GetName();
            if (behaviorContextName == facadeName)
            {
                Debug::Log("TransformBusProbe ComponentApplicationBus::GetEntityName passed");
                reportedEntityNameRoundTrip = true;
            }
            else
            {
                Debug::Warning("TransformBusProbe ComponentApplicationBus::GetEntityName mismatch");
            }
        }

        if (!target.IsValid())
        {
            Debug::Warning("TransformBusProbe target is invalid");
            return;
        }

        hasTarget = true;
        startPosition = TransformBus::GetWorldTranslation(target);
        startRotation = TransformBus::GetWorldRotationQuaternion(target);
        startLocalTransform = TransformBus::GetLocalTM(target);

        Transform localOut;
        Transform worldOut;
        TransformBus::GetLocalAndWorld(target, localOut, worldOut);
        startWorldTransform = worldOut;

        Vec3 facadePosition = target.GetWorldPosition();
        Vec3 delta = facadePosition - startPosition;
        float error = Abs(delta.x) + Abs(delta.y) + Abs(delta.z);

        if (error > 0.001f)
        {
            Debug::Warning("TransformBusProbe facade and BehaviorContext positions differ on activate");
        }
        else
        {
            Debug::Log("TransformBusProbe initial position check passed");
        }

        if (!reportedTransformAccessRoundTrip)
        {
            Transform localTm = TransformBus::GetLocalTM(target);
            Transform localOutCheck;
            Transform worldOutCheck;
            TransformBus::GetLocalAndWorld(target, localOutCheck, worldOutCheck);

            float localTransformError =
                Abs(localTm.basisX.x - localOutCheck.basisX.x) +
                Abs(localTm.basisX.y - localOutCheck.basisX.y) +
                Abs(localTm.basisX.z - localOutCheck.basisX.z) +
                Abs(localTm.basisY.x - localOutCheck.basisY.x) +
                Abs(localTm.basisY.y - localOutCheck.basisY.y) +
                Abs(localTm.basisY.z - localOutCheck.basisY.z) +
                Abs(localTm.basisZ.x - localOutCheck.basisZ.x) +
                Abs(localTm.basisZ.y - localOutCheck.basisZ.y) +
                Abs(localTm.basisZ.z - localOutCheck.basisZ.z) +
                Abs(localTm.translation.x - localOutCheck.translation.x) +
                Abs(localTm.translation.y - localOutCheck.translation.y) +
                Abs(localTm.translation.z - localOutCheck.translation.z);

            if (localTransformError <= 0.001f)
            {
                Debug::Log("TransformBusProbe Transform GetLocalTM/GetLocalAndWorld passed");
                reportedTransformAccessRoundTrip = true;
            }
            else
            {
                Debug::Error("TransformBusProbe Transform GetLocalTM/GetLocalAndWorld mismatch");
            }
        }

        if (!reportedHierarchyArrayRoundTrip)
        {
            array<Entity>@ children = TransformBus::GetChildren(target);
            array<Entity>@ descendants = TransformBus::GetAllDescendants(target);
            array<Entity>@ hierarchy = TransformBus::GetEntityAndAllDescendants(target);

            if (children is null || descendants is null || hierarchy is null)
            {
                Debug::Error("TransformBusProbe Transform hierarchy array access failed");
            }
            else if (hierarchy.length() < descendants.length())
            {
                Debug::Error("TransformBusProbe Transform hierarchy array lengths are inconsistent");
            }
            else
            {
                Debug::Log("TransformBusProbe Transform hierarchy array access passed");
                reportedHierarchyArrayRoundTrip = true;
            }
        }

        if (!reportedQuatConstructorRoundTrip)
        {
            Quat identity;
            Quat explicitQuat(1.0f, 2.0f, 3.0f, 4.0f);

            if (identity.x == 0.0f &&
                identity.y == 0.0f &&
                identity.z == 0.0f &&
                identity.w == 1.0f &&
                explicitQuat.x == 1.0f &&
                explicitQuat.y == 2.0f &&
                explicitQuat.z == 3.0f &&
                explicitQuat.w == 4.0f)
            {
                Debug::Log("TransformBusProbe Quat constructors and properties passed");
                reportedQuatConstructorRoundTrip = true;
            }
            else
            {
                Debug::Error("TransformBusProbe Quat constructors or properties failed");
            }
        }

        if (verifyAttachmentOffset && !reportedAttachmentOffsetRoundTrip)
        {
            if (!attachmentTarget.IsValid())
            {
                Debug::Warning("TransformBusProbe attachmentTarget is invalid; skipping AttachmentComponentBus::GetOffset check");
                reportedAttachmentOffsetRoundTrip = true;
            }
            else
            {
                Transform attachmentOffset = AttachmentComponentBus::GetOffset(attachmentTarget);
                float attachmentError = Vec3Error(attachmentOffset.translation, expectedAttachmentOffset);
                if (attachmentError <= 0.001f)
                {
                    Debug::Log("TransformBusProbe AttachmentComponentBus::GetOffset passed");
                }
                else
                {
                    Debug::Error(
                        "TransformBusProbe AttachmentComponentBus::GetOffset failed expected=" +
                        FormatVec3(expectedAttachmentOffset) +
                        " observed=" +
                        FormatVec3(attachmentOffset.translation));
                }
                reportedAttachmentOffsetRoundTrip = true;
            }
        }

        if (verifyHeightfieldTransform && !reportedHeightfieldTransformRoundTrip)
        {
            if (!heightfieldTarget.IsValid())
            {
                Debug::Warning("TransformBusProbe heightfieldTarget is invalid; skipping HeightfieldProviderRequestsBus::GetHeightfieldTransform check");
                reportedHeightfieldTransformRoundTrip = true;
            }
            else
            {
                Transform heightfieldTransform = HeightfieldProviderRequestsBus::GetHeightfieldTransform(heightfieldTarget);
                float heightfieldError = Vec3Error(heightfieldTransform.translation, expectedHeightfieldTranslation);
                if (heightfieldError <= 0.001f)
                {
                    Debug::Log("TransformBusProbe HeightfieldProviderRequestsBus::GetHeightfieldTransform passed");
                }
                else
                {
                    Debug::Error(
                        "TransformBusProbe HeightfieldProviderRequestsBus::GetHeightfieldTransform failed expected=" +
                        FormatVec3(expectedHeightfieldTranslation) +
                        " observed=" +
                        FormatVec3(heightfieldTransform.translation));
                }
                reportedHeightfieldTransformRoundTrip = true;
            }
        }
    }

    void OnTick(float deltaTime)
    {
        elapsed = elapsed + deltaTime;

        if (!reportedTickDeltaRoundTrip)
        {
            float busDeltaTime = TickBus::GetTickDeltaTime();
            float deltaError = Abs(busDeltaTime - deltaTime);
            if (deltaError < 0.0001f)
            {
                Debug::Log("TransformBusProbe TickBus::GetTickDeltaTime passed");
                reportedTickDeltaRoundTrip = true;
            }
            else
            {
                Debug::Warning("TransformBusProbe TickBus::GetTickDeltaTime mismatch");
            }
        }

        if (!reportedCameraReady)
        {
            Entity activeCamera = CameraSystemBus::GetActiveCamera();
            if (activeCamera.IsValid())
            {
                Debug::Log("TransformBusProbe CameraSystemBus::GetActiveCamera passed");
                reportedCameraReady = true;
            }
            else if (!reportedCameraFailure && elapsed > 1.0f)
            {
                Debug::Error("TransformBusProbe CameraSystemBus::GetActiveCamera still returned invalid entity after startup delay");
                reportedCameraFailure = true;
            }
        }

        if (verifyViewportCameraTransform && reportedCameraReady && !reportedViewportTransformRoundTrip)
        {
            Entity activeCamera = CameraSystemBus::GetActiveCamera();
            if (activeCamera.IsValid())
            {
                Transform viewportCameraTransform = ViewportBus::GetCameraTransform(viewportId);
                Transform activeCameraTransform = TransformBus::GetWorldTM(activeCamera);
                float viewportTransformError = TransformError(viewportCameraTransform, activeCameraTransform);

                if (viewportTransformError <= 0.001f)
                {
                    Debug::Log("TransformBusProbe ViewportBus::GetCameraTransform passed");
                    reportedViewportTransformRoundTrip = true;
                }
                else if (!reportedViewportTransformFailure && elapsed > 0.5f)
                {
                    Debug::Error("TransformBusProbe ViewportBus::GetCameraTransform failed");
                    Debug::Warning("TransformBusProbe viewport camera translation=" + FormatVec3(viewportCameraTransform.translation));
                    Debug::Warning("TransformBusProbe active camera world translation=" + FormatVec3(activeCameraTransform.translation));
                    reportedViewportTransformFailure = true;
                    reportedViewportTransformRoundTrip = true;
                }
            }
        }

        if (!hasTarget)
        {
            return;
        }

        Vec3 busPosition = TransformBus::GetWorldTranslation(target);
        Vec3 facadePosition = target.GetWorldPosition();
        Vec3 delta = facadePosition - busPosition;
        float error = Abs(delta.x) + Abs(delta.y) + Abs(delta.z);

        if (error > 0.001f)
        {
            Debug::Warning("TransformBusProbe facade and BehaviorContext positions differ on tick");
        }

        if (!reportedScalarWorldPositionRoundTrip)
        {
            float worldX = TransformBus::GetWorldX(target);
            float worldY = TransformBus::GetWorldY(target);
            float worldZ = TransformBus::GetWorldZ(target);
            float scalarError = Abs(worldX - busPosition.x) + Abs(worldY - busPosition.y) + Abs(worldZ - busPosition.z);
            if (scalarError < 0.001f)
            {
                Debug::Log("TransformBusProbe scalar TransformBus GetWorldX/Y/Z passed");
                reportedScalarWorldPositionRoundTrip = true;
            }
            else
            {
                Debug::Warning("TransformBusProbe scalar TransformBus GetWorldX/Y/Z mismatch");
            }
        }

        if (!requestedRoundTrip && elapsed > 0.05f)
        {
            Vec3 expected = startPosition;
            expected.z = expected.z + 0.25f;
            TransformBus::SetWorldTranslation(target, expected);
            roundTripRequestTime = elapsed;
            requestedRoundTrip = true;
        }

        if (requestedRoundTrip && !reportedRoundTrip)
        {
            Vec3 expected = startPosition;
            expected.z = expected.z + 0.25f;

            Vec3 afterSet = TransformBus::GetWorldTranslation(target);
            Vec3 setDelta = afterSet - expected;
            float setError = Abs(setDelta.x) + Abs(setDelta.y) + Abs(setDelta.z);

            if (setError <= 0.001f)
            {
                Debug::Log("TransformBusProbe BehaviorContext Set/Get round-trip passed");
                TransformBus::SetWorldTranslation(target, startPosition);
                reportedRoundTrip = true;
            }
            else if ((elapsed - roundTripRequestTime) > 0.25f)
            {
                Debug::Error("TransformBusProbe BehaviorContext Set/Get round-trip failed");
                TransformBus::SetWorldTranslation(target, startPosition);
                reportedRoundTrip = true;
            }
        }

        if (reportedRoundTrip && !requestedRotationRoundTrip)
        {
            Quat expectedRotation = Quat(0.0f, 0.0f, 0.3826834f, 0.9238795f);
            TransformBus::SetWorldRotationQuaternion(target, expectedRotation);
            rotationRoundTripRequestTime = elapsed;
            requestedRotationRoundTrip = true;
        }

        if (requestedRotationRoundTrip && !reportedRotationRoundTrip)
        {
            Quat expectedRotation = Quat(0.0f, 0.0f, 0.3826834f, 0.9238795f);
            Quat afterSetRotation = TransformBus::GetWorldRotationQuaternion(target);
            float rotationError =
                Abs(afterSetRotation.x - expectedRotation.x) +
                Abs(afterSetRotation.y - expectedRotation.y) +
                Abs(afterSetRotation.z - expectedRotation.z) +
                Abs(afterSetRotation.w - expectedRotation.w);

            if (rotationError <= 0.001f)
            {
                Debug::Log("TransformBusProbe BehaviorContext Quat Set/Get round-trip passed");
                TransformBus::SetWorldRotationQuaternion(target, startRotation);
                reportedRotationRoundTrip = true;
            }
            else if ((elapsed - rotationRoundTripRequestTime) > 0.25f)
            {
                Debug::Error("TransformBusProbe BehaviorContext Quat Set/Get round-trip failed");
                TransformBus::SetWorldRotationQuaternion(target, startRotation);
                reportedRotationRoundTrip = true;
            }
        }

        if (reportedRotationRoundTrip && !requestedLocalTransformRoundTrip)
        {
            Transform expectedLocal = startLocalTransform;
            expectedLocal.translation.z = expectedLocal.translation.z + 0.25f;
            TransformBus::SetLocalTM(target, expectedLocal);
            localTransformRoundTripRequestTime = elapsed;
            requestedLocalTransformRoundTrip = true;
        }

        if (requestedLocalTransformRoundTrip && !reportedLocalTransformRoundTrip)
        {
            Transform expectedLocal = startLocalTransform;
            expectedLocal.translation.z = expectedLocal.translation.z + 0.25f;
            Transform afterLocal = TransformBus::GetLocalTM(target);
            Vec3 afterLocalTranslation = TransformBus::GetLocalTranslation(target);
            Vec3 afterWorldTranslation = TransformBus::GetWorldTranslation(target);
            float localError =
                Abs(afterLocal.translation.x - expectedLocal.translation.x) +
                Abs(afterLocal.translation.y - expectedLocal.translation.y) +
                Abs(afterLocal.translation.z - expectedLocal.translation.z);

            if (localError <= 0.001f)
            {
                Debug::Log("TransformBusProbe Transform SetLocalTM/GetLocalTM round-trip passed");
                TransformBus::SetLocalTM(target, startLocalTransform);
                reportedLocalTransformRoundTrip = true;
            }
            else if ((elapsed - localTransformRoundTripRequestTime) > 0.25f)
            {
                Debug::Error("TransformBusProbe Transform SetLocalTM/GetLocalTM round-trip failed");
                if (!reportedLocalTransformFailureDetails)
                {
                    Entity parent = TransformBus::GetParentId(target);
                    Transform localOutAfterSet;
                    Transform worldOutAfterSet;
                    TransformBus::GetLocalAndWorld(target, localOutAfterSet, worldOutAfterSet);
                    string parentState = parent.IsValid() ? "valid" : "invalid";
                    string staticState = TransformBus::IsStaticTransform(target) ? "true" : "false";
                    Debug::Warning("TransformBusProbe SetLocalTM expectedLocal.translation=" + FormatVec3(expectedLocal.translation));
                    Debug::Warning("TransformBusProbe SetLocalTM observed GetLocalTM.translation=" + FormatVec3(afterLocal.translation));
                    Debug::Warning("TransformBusProbe SetLocalTM observed GetLocalAndWorld.local.translation=" + FormatVec3(localOutAfterSet.translation));
                    Debug::Warning("TransformBusProbe SetLocalTM observed GetLocalAndWorld.world.translation=" + FormatVec3(worldOutAfterSet.translation));
                    Debug::Warning("TransformBusProbe SetLocalTM observed GetLocalTranslation=" + FormatVec3(afterLocalTranslation));
                    Debug::Warning("TransformBusProbe SetLocalTM observed GetWorldTranslation=" + FormatVec3(afterWorldTranslation));
                    Debug::Warning("TransformBusProbe SetLocalTM startLocal.translation=" + FormatVec3(startLocalTransform.translation));
                    Debug::Warning("TransformBusProbe SetLocalTM startWorldTranslation=" + FormatVec3(startPosition));
                    Debug::Warning("TransformBusProbe SetLocalTM parent=" + parentState + " isStatic=" + staticState);
                    reportedLocalTransformFailureDetails = true;
                }
                TransformBus::SetLocalTM(target, startLocalTransform);
                reportedLocalTransformRoundTrip = true;
            }
        }

        if (reportedLocalTransformRoundTrip && !requestedWorldTransformRoundTrip)
        {
            Transform expectedWorld = startWorldTransform;
            expectedWorld.translation.z = expectedWorld.translation.z + 0.35f;
            TransformBus::SetWorldTM(target, expectedWorld);
            worldTransformRoundTripRequestTime = elapsed;
            requestedWorldTransformRoundTrip = true;
        }

        if (requestedWorldTransformRoundTrip && !reportedWorldTransformRoundTrip)
        {
            Transform expectedWorld = startWorldTransform;
            expectedWorld.translation.z = expectedWorld.translation.z + 0.35f;
            Vec3 afterWorldTranslation = TransformBus::GetWorldTranslation(target);
            float worldError =
                Abs(afterWorldTranslation.x - expectedWorld.translation.x) +
                Abs(afterWorldTranslation.y - expectedWorld.translation.y) +
                Abs(afterWorldTranslation.z - expectedWorld.translation.z);

            if (worldError <= 0.001f)
            {
                Debug::Log("TransformBusProbe Transform SetWorldTM round-trip passed");
                TransformBus::SetWorldTM(target, startWorldTransform);
                reportedWorldTransformRoundTrip = true;
            }
            else if ((elapsed - worldTransformRoundTripRequestTime) > 0.25f)
            {
                Debug::Error("TransformBusProbe Transform SetWorldTM round-trip failed");
                TransformBus::SetWorldTM(target, startWorldTransform);
                reportedWorldTransformRoundTrip = true;
            }
        }

        if (reportedWorldTransformRoundTrip && !requestedLocalTranslationRoundTrip)
        {
            Vec3 expectedLocalTranslation = startLocalTransform.translation;
            expectedLocalTranslation.z = expectedLocalTranslation.z + 0.45f;
            TransformBus::SetLocalTranslation(target, expectedLocalTranslation);
            localTranslationRoundTripRequestTime = elapsed;
            requestedLocalTranslationRoundTrip = true;
        }

        if (requestedLocalTranslationRoundTrip && !reportedLocalTranslationRoundTrip)
        {
            Vec3 expectedLocalTranslation = startLocalTransform.translation;
            expectedLocalTranslation.z = expectedLocalTranslation.z + 0.45f;
            Vec3 afterLocalTranslation = TransformBus::GetLocalTranslation(target);
            Vec3 afterWorldTranslation = TransformBus::GetWorldTranslation(target);
            float localTranslationError =
                Abs(afterLocalTranslation.x - expectedLocalTranslation.x) +
                Abs(afterLocalTranslation.y - expectedLocalTranslation.y) +
                Abs(afterLocalTranslation.z - expectedLocalTranslation.z);

            if (localTranslationError <= 0.001f)
            {
                Debug::Log("TransformBusProbe Transform SetLocalTranslation/GetLocalTranslation round-trip passed");
                TransformBus::SetLocalTranslation(target, startLocalTransform.translation);
                reportedLocalTranslationRoundTrip = true;
            }
            else if ((elapsed - localTranslationRoundTripRequestTime) > 0.25f)
            {
                Debug::Error("TransformBusProbe Transform SetLocalTranslation/GetLocalTranslation round-trip failed");
                Debug::Warning("TransformBusProbe SetLocalTranslation expected=" + FormatVec3(expectedLocalTranslation));
                Debug::Warning("TransformBusProbe SetLocalTranslation observed local=" + FormatVec3(afterLocalTranslation));
                Debug::Warning("TransformBusProbe SetLocalTranslation observed world=" + FormatVec3(afterWorldTranslation));
                TransformBus::SetLocalTranslation(target, startLocalTransform.translation);
                reportedLocalTranslationRoundTrip = true;
            }
        }
    }

    void OnDeactivate()
    {
        if (hasTarget)
        {
            TransformBus::SetLocalTM(target, startLocalTransform);
            TransformBus::SetWorldTM(target, startWorldTransform);
            TransformBus::SetWorldTranslation(target, startPosition);
            TransformBus::SetWorldRotationQuaternion(target, startRotation);
        }

        Debug::Log("TransformBusProbe.OnDeactivate");
    }

    float Abs(float value)
    {
        if (value < 0.0f)
        {
            return -value;
        }

        return value;
    }

    string FormatFloat(float value)
    {
        return "" + value;
    }

    string FormatVec3(const Vec3& in value)
    {
        return "(" + FormatFloat(value.x) + ", " + FormatFloat(value.y) + ", " + FormatFloat(value.z) + ")";
    }

    float Vec3Error(const Vec3& in lhs, const Vec3& in rhs)
    {
        return Abs(lhs.x - rhs.x) + Abs(lhs.y - rhs.y) + Abs(lhs.z - rhs.z);
    }

    float TransformError(const Transform& in lhs, const Transform& in rhs)
    {
        return
            Vec3Error(lhs.basisX, rhs.basisX) +
            Vec3Error(lhs.basisY, rhs.basisY) +
            Vec3Error(lhs.basisZ, rhs.basisZ) +
            Vec3Error(lhs.translation, rhs.translation);
    }
}
