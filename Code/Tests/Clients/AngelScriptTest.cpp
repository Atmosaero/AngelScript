
#include <AzTest/AzTest.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzFramework/UnitTest/TestDebugDisplayRequests.h>
#include <Bindings/Debug.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>
#include <AzFramework/Viewport/ViewportBus.h>
#include <Clients/AngelScriptSystemComponent.h>
#include <Components/AngelScriptComponent.h>
#include "../../../../LmbrCentral/Code/include/LmbrCentral/Animation/AttachmentComponentBus.h"
#include <Runtime/AngelScriptRuntime.h>

namespace AngelScript
{
    namespace
    {
        AZStd::string GetBasicMoverScriptPath()
        {
            AZStd::string path = __FILE__;
            size_t suffixPosition = path.rfind("Code\\Tests\\Clients\\AngelScriptTest.cpp");
            if (suffixPosition == AZStd::string::npos)
            {
                suffixPosition = path.rfind("Code/Tests/Clients/AngelScriptTest.cpp");
            }

            if (suffixPosition == AZStd::string::npos)
            {
                return {};
            }

            path.resize(suffixPosition);
            path += "Assets/Scripts/BasicMover.as";
            return path;
        }

        AZStd::string GetRuntimeProbeScriptPath()
        {
            AZStd::string path = __FILE__;
            size_t suffixPosition = path.rfind("Code\\Tests\\Clients\\AngelScriptTest.cpp");
            if (suffixPosition == AZStd::string::npos)
            {
                suffixPosition = path.rfind("Code/Tests/Clients/AngelScriptTest.cpp");
            }

            if (suffixPosition == AZStd::string::npos)
            {
                return {};
            }

            path.resize(suffixPosition);
            path += "Assets/Scripts/RuntimeProbe.as";
            return path;
        }

        AZStd::string GetEntityBusProbeScriptPath()
        {
            AZStd::string path = __FILE__;
            size_t suffixPosition = path.rfind("Code\\Tests\\Clients\\AngelScriptTest.cpp");
            if (suffixPosition == AZStd::string::npos)
            {
                suffixPosition = path.rfind("Code/Tests/Clients/AngelScriptTest.cpp");
            }

            if (suffixPosition == AZStd::string::npos)
            {
                return {};
            }

            path.resize(suffixPosition);
            path += "Assets/Scripts/EntityBusProbe.as";
            return path;
        }

        AZStd::string GetRayProbeScriptPath()
        {
            AZStd::string path = __FILE__;
            size_t suffixPosition = path.rfind("Code\\Tests\\Clients\\AngelScriptTest.cpp");
            if (suffixPosition == AZStd::string::npos)
            {
                suffixPosition = path.rfind("Code/Tests/Clients/AngelScriptTest.cpp");
            }

            if (suffixPosition == AZStd::string::npos)
            {
                return {};
            }

            path.resize(suffixPosition);
            path += "Assets/Scripts/RayProbe.as";
            return path;
        }

        bool IsBehaviorContextAvailableForTests()
        {
            AZ::BehaviorContext* behaviorContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                behaviorContext,
                &AZ::ComponentApplicationBus::Events::GetBehaviorContext);
            return behaviorContext != nullptr;
        }

        bool HasBehaviorContextEbusEvent(const char* ebusName, const char* eventName)
        {
            AZ::BehaviorContext* behaviorContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                behaviorContext,
                &AZ::ComponentApplicationBus::Events::GetBehaviorContext);
            if (behaviorContext == nullptr)
            {
                return false;
            }

            const AZ::BehaviorEBus* ebus = behaviorContext->FindEBusByReflectedName(ebusName);
            if (ebus == nullptr)
            {
                return false;
            }

            return ebus->m_events.find(eventName) != ebus->m_events.end();
        }

        AZ::Transform CreateCoverageTransform(float translationX, float translationY, float translationZ)
        {
            AZ::Transform transform = AZ::Transform::CreateIdentity();
            transform.SetTranslation(AZ::Vector3(translationX, translationY, translationZ));
            return transform;
        }

        class TestAttachmentComponentRequests
            : public LmbrCentral::AttachmentComponentRequestBus::Handler
        {
        public:
            explicit TestAttachmentComponentRequests(AZ::EntityId entityId)
                : m_offset(CreateCoverageTransform(1.25f, 2.5f, 3.75f))
            {
                BusConnect(entityId);
            }

            ~TestAttachmentComponentRequests() override
            {
                BusDisconnect();
            }

            void Attach(AZ::EntityId, const char*, const AZ::Transform&) override {}
            void Detach() override {}
            void Reattach(bool) override {}
            void SetAttachmentOffset(const AZ::Transform& offset) override
            {
                m_offset = offset;
            }

            const char* GetJointName() override
            {
                return "TestJoint";
            }

            AZ::EntityId GetTargetEntityId() override
            {
                return AZ::EntityId(9001);
            }

            AZ::Transform GetOffset() override
            {
                return m_offset;
            }

        private:
            AZ::Transform m_offset;
        };

        class TestHeightfieldProviderRequests
            : public Physics::HeightfieldProviderRequestsBus::Handler
        {
        public:
            explicit TestHeightfieldProviderRequests(AZ::EntityId entityId)
                : m_transform(CreateCoverageTransform(4.5f, 5.5f, 6.5f))
            {
                BusConnect(entityId);
            }

            ~TestHeightfieldProviderRequests() override
            {
                BusDisconnect();
            }

            AZ::Vector2 GetHeightfieldGridSpacing() const override
            {
                return AZ::Vector2(1.0f, 1.0f);
            }

            void GetHeightfieldGridSize(size_t& numColumns, size_t& numRows) const override
            {
                numColumns = 1;
                numRows = 1;
            }

            AZ::u64 GetHeightfieldGridColumns() const override
            {
                return 1;
            }

            AZ::u64 GetHeightfieldGridRows() const override
            {
                return 1;
            }

            void GetHeightfieldHeightBounds(float& minHeightBounds, float& maxHeightBounds) const override
            {
                minHeightBounds = 0.0f;
                maxHeightBounds = 0.0f;
            }

            float GetHeightfieldMinHeight() const override
            {
                return 0.0f;
            }

            float GetHeightfieldMaxHeight() const override
            {
                return 0.0f;
            }

            AZ::Aabb GetHeightfieldAabb() const override
            {
                return AZ::Aabb::CreateNull();
            }

            AZ::Transform GetHeightfieldTransform() const override
            {
                return m_transform;
            }

            AZStd::vector<AZ::Data::Asset<Physics::MaterialAsset>> GetMaterialList() const override
            {
                return {};
            }

            AZStd::vector<float> GetHeights() const override
            {
                return {};
            }

            AZStd::vector<Physics::HeightMaterialPoint> GetHeightsAndMaterials() const override
            {
                return {};
            }

            void GetHeightfieldIndicesFromRegion(const AZ::Aabb&, size_t& startColumn, size_t& startRow, size_t& numColumns, size_t& numRows) const override
            {
                startColumn = 0;
                startRow = 0;
                numColumns = 0;
                numRows = 0;
            }

            void UpdateHeightsAndMaterials(const Physics::UpdateHeightfieldSampleFunction&, size_t, size_t, size_t, size_t) const override {}

            void UpdateHeightsAndMaterialsAsync(
                const Physics::UpdateHeightfieldSampleFunction&,
                const Physics::UpdateHeightfieldCompleteFunction& updateHeightsCompleteCallback,
                size_t,
                size_t,
                size_t,
                size_t) const override
            {
                if (updateHeightsCompleteCallback)
                {
                    updateHeightsCompleteCallback();
                }
            }

        private:
            AZ::Transform m_transform;
        };

        class TestViewportRequests
            : public AzFramework::ViewportRequestBus::Handler
        {
        public:
            explicit TestViewportRequests(AzFramework::ViewportId viewportId)
                : m_cameraTransform(CreateCoverageTransform(7.25f, 8.25f, 9.25f))
                , m_viewMatrix(AZ::Matrix4x4::CreateIdentity())
                , m_projectionMatrix(AZ::Matrix4x4::CreateIdentity())
            {
                BusConnect(viewportId);
            }

            ~TestViewportRequests() override
            {
                BusDisconnect();
            }

            const AZ::Matrix4x4& GetCameraViewMatrix() const override
            {
                return m_viewMatrix;
            }

            AZ::Matrix3x4 GetCameraViewMatrixAsMatrix3x4() const override
            {
                return AZ::Matrix3x4::CreateIdentity();
            }

            void SetCameraViewMatrix(const AZ::Matrix4x4&) override {}

            const AZ::Matrix4x4& GetCameraProjectionMatrix() const override
            {
                return m_projectionMatrix;
            }

            void SetCameraProjectionMatrix(const AZ::Matrix4x4&) override {}

            AZ::Transform GetCameraTransform() const override
            {
                return m_cameraTransform;
            }

            void SetCameraTransform(const AZ::Transform& transform) override
            {
                m_cameraTransform = transform;
            }

        private:
            AZ::Transform m_cameraTransform;
            AZ::Matrix4x4 m_viewMatrix;
            AZ::Matrix4x4 m_projectionMatrix;
        };
    } // namespace

    class AngelScriptRuntimeTests
        : public ::testing::Test
    {
    };

    class TestableAngelScriptSystemComponent
        : public AngelScriptSystemComponent
    {
    public:
        using AngelScriptSystemComponent::CompileScriptFile;
        using AngelScriptSystemComponent::CompileScriptString;
        using AngelScriptSystemComponent::IsRuntimeSdkAvailable;
        using AngelScriptSystemComponent::IsRuntimeStarted;
    };

    TEST_F(AngelScriptRuntimeTests, Start_WithoutSdk_EntersUnavailableMode)
    {
        AngelScriptRuntime runtime;

        const bool startedWithSdk = runtime.Start();

        EXPECT_EQ(startedWithSdk, runtime.IsSdkAvailable());
        EXPECT_TRUE(runtime.IsStarted());
    }

    TEST_F(AngelScriptRuntimeTests, CompileScriptString_WithEmptyInput_ReturnsError)
    {
        AngelScriptRuntime runtime;

        ScriptCompileResult result = runtime.CompileScriptString("", "");

        EXPECT_FALSE(result.m_success);
        EXPECT_FALSE(result.m_error.empty());
    }

    TEST_F(AngelScriptRuntimeTests, CompileScriptFile_WithMissingPath_ReturnsError)
    {
        AngelScriptRuntime runtime;

        ScriptCompileResult result = runtime.CompileScriptFile("Missing", "missing_script.as");

        EXPECT_FALSE(result.m_success);
        EXPECT_FALSE(result.m_error.empty());
    }

    TEST_F(AngelScriptRuntimeTests, CompileScriptString_WithMinimalScript_ReturnsSdkAvailability)
    {
        AngelScriptRuntime runtime;

        const ScriptCompileResult result = runtime.CompileScriptString("Smoke", "void Main() {}");

        EXPECT_EQ(result.m_success, runtime.IsSdkAvailable());
        if (!runtime.IsSdkAvailable())
        {
            EXPECT_FALSE(result.m_error.empty());
        }
    }

    TEST_F(AngelScriptRuntimeTests, ScriptInstanceLifecycle_WithMinimalClass_ReturnsSdkAvailability)
    {
        AngelScriptRuntime runtime;

        const ScriptCompileResult compileResult = runtime.CompileScriptString(
            "Lifecycle",
            "class LifecycleScript { Entity self; Vec3 up; void SetEntity(Entity entity) { self = entity; up.z = 1.0f; } void OnActivate() { Debug::Log(\"AngelScript activated \" + self.GetName()); self.SetWorldPosition(self.GetWorldPosition() + up); } void OnTick(float deltaTime) { Vec3 step = up * deltaTime; } }");
        EXPECT_EQ(compileResult.m_success, runtime.IsSdkAvailable());

        ScriptInstanceResult instanceResult = runtime.CreateScriptInstance("Lifecycle", "LifecycleScript");
        EXPECT_EQ(instanceResult.m_success, runtime.IsSdkAvailable());
        if (!runtime.IsSdkAvailable())
        {
            EXPECT_FALSE(instanceResult.m_error.empty());
            return;
        }

        EXPECT_NE(instanceResult.m_instanceId, InvalidScriptInstanceId);
        EXPECT_TRUE(instanceResult.m_hasSetEntity);
        EXPECT_TRUE(instanceResult.m_hasOnActivate);
        EXPECT_FALSE(instanceResult.m_hasOnDeactivate);
        EXPECT_TRUE(instanceResult.m_hasOnTick);

        const ScriptCompileResult entityResult = runtime.ExecuteScriptInstanceSetEntity(instanceResult.m_instanceId, AZ::EntityId(123));
        EXPECT_TRUE(entityResult.m_success) << entityResult.m_error.c_str();

        const ScriptCompileResult activateResult = runtime.ExecuteScriptInstanceOnActivate(instanceResult.m_instanceId);
        EXPECT_TRUE(activateResult.m_success) << activateResult.m_error.c_str();

        runtime.DestroyScriptInstance(instanceResult.m_instanceId);
    }

    TEST_F(AngelScriptRuntimeTests, Vec3_ConstructorsAndOperators_ReturnSdkAvailability)
    {
        AngelScriptRuntime runtime;

        const ScriptCompileResult compileResult = runtime.CompileScriptString(
            "Vec3Constructors",
            "class Vec3Constructors { void OnActivate() { Vec3 a; Vec3 b(1.0f, 2.0f, 3.0f); Vec3 c = b + Vec3(4.0f, 5.0f, 6.0f); if (a.x != 0.0f || a.y != 0.0f || a.z != 0.0f) { Debug::Error(\"Vec3 default constructor failed\"); } if (c.x != 5.0f || c.y != 7.0f || c.z != 9.0f) { Debug::Error(\"Vec3 float constructor failed\"); } } }");
        EXPECT_EQ(compileResult.m_success, runtime.IsSdkAvailable()) << compileResult.m_error.c_str();

        ScriptInstanceResult instanceResult = runtime.CreateScriptInstance("Vec3Constructors", "Vec3Constructors");
        EXPECT_EQ(instanceResult.m_success, runtime.IsSdkAvailable());
        if (!runtime.IsSdkAvailable())
        {
            EXPECT_FALSE(instanceResult.m_error.empty());
            return;
        }

        EXPECT_TRUE(instanceResult.m_hasOnActivate);

        const ScriptCompileResult activateResult = runtime.ExecuteScriptInstanceOnActivate(instanceResult.m_instanceId);
        EXPECT_TRUE(activateResult.m_success) << activateResult.m_error.c_str();

        runtime.DestroyScriptInstance(instanceResult.m_instanceId);
    }

    TEST_F(AngelScriptRuntimeTests, Vec2_ConstructorsAndOperators_ReturnSdkAvailability)
    {
        AngelScriptRuntime runtime;

        const ScriptCompileResult compileResult = runtime.CompileScriptString(
            "Vec2Constructors",
            "class Vec2Constructors { void OnActivate() { Vec2 a; Vec2 b(1.0f, 2.0f); Vec2 c = b + Vec2(4.0f, 5.0f); if (a.x != 0.0f || a.y != 0.0f) { Debug::Error(\"Vec2 default constructor failed\"); } if (c.x != 5.0f || c.y != 7.0f) { Debug::Error(\"Vec2 float constructor failed\"); } } }");
        EXPECT_EQ(compileResult.m_success, runtime.IsSdkAvailable()) << compileResult.m_error.c_str();

        ScriptInstanceResult instanceResult = runtime.CreateScriptInstance("Vec2Constructors", "Vec2Constructors");
        EXPECT_EQ(instanceResult.m_success, runtime.IsSdkAvailable());
        if (!runtime.IsSdkAvailable())
        {
            EXPECT_FALSE(instanceResult.m_error.empty());
            return;
        }

        EXPECT_TRUE(instanceResult.m_hasOnActivate);

        const ScriptCompileResult activateResult = runtime.ExecuteScriptInstanceOnActivate(instanceResult.m_instanceId);
        EXPECT_TRUE(activateResult.m_success) << activateResult.m_error.c_str();

        runtime.DestroyScriptInstance(instanceResult.m_instanceId);
    }

    TEST_F(AngelScriptRuntimeTests, Quat_ConstructorsAndProperties_ReturnSdkAvailability)
    {
        AngelScriptRuntime runtime;

        const ScriptCompileResult compileResult = runtime.CompileScriptString(
            "QuatConstructors",
            "class QuatConstructors { void OnActivate() { Quat a; Quat b(1.0f, 2.0f, 3.0f, 4.0f); if (a.x != 0.0f || a.y != 0.0f || a.z != 0.0f || a.w != 1.0f) { Debug::Error(\"Quat default constructor failed\"); } if (b.x != 1.0f || b.y != 2.0f || b.z != 3.0f || b.w != 4.0f) { Debug::Error(\"Quat float constructor failed\"); } } }");
        EXPECT_EQ(compileResult.m_success, runtime.IsSdkAvailable()) << compileResult.m_error.c_str();

        ScriptInstanceResult instanceResult = runtime.CreateScriptInstance("QuatConstructors", "QuatConstructors");
        EXPECT_EQ(instanceResult.m_success, runtime.IsSdkAvailable());
        if (!runtime.IsSdkAvailable())
        {
            EXPECT_FALSE(instanceResult.m_error.empty());
            return;
        }

        EXPECT_TRUE(instanceResult.m_hasOnActivate);

        const ScriptCompileResult activateResult = runtime.ExecuteScriptInstanceOnActivate(instanceResult.m_instanceId);
        EXPECT_TRUE(activateResult.m_success) << activateResult.m_error.c_str();

        runtime.DestroyScriptInstance(instanceResult.m_instanceId);
    }

    TEST_F(AngelScriptRuntimeTests, Transform_ConstructorsAndProperties_ReturnSdkAvailability)
    {
        AngelScriptRuntime runtime;

        const ScriptCompileResult compileResult = runtime.CompileScriptString(
            "TransformConstructors",
            "class TransformConstructors { void OnActivate() { Transform a; a.basisX = Vec3(2.0f, 0.0f, 0.0f); a.basisY = Vec3(0.0f, 2.0f, 0.0f); a.basisZ = Vec3(0.0f, 0.0f, 2.0f); a.translation = Vec3(1.0f, 2.0f, 3.0f); Transform b = a; Transform c; c = b; if (c.basisX.x != 2.0f || c.basisY.y != 2.0f || c.basisZ.z != 2.0f || c.translation.x != 1.0f || c.translation.y != 2.0f || c.translation.z != 3.0f) { Debug::Error(\"Transform copy or assignment failed\"); } } }");
        EXPECT_EQ(compileResult.m_success, runtime.IsSdkAvailable()) << compileResult.m_error.c_str();

        ScriptInstanceResult instanceResult = runtime.CreateScriptInstance("TransformConstructors", "TransformConstructors");
        EXPECT_EQ(instanceResult.m_success, runtime.IsSdkAvailable());
        if (!runtime.IsSdkAvailable())
        {
            EXPECT_FALSE(instanceResult.m_error.empty());
            return;
        }

        EXPECT_TRUE(instanceResult.m_hasOnActivate);

        const ScriptCompileResult activateResult = runtime.ExecuteScriptInstanceOnActivate(instanceResult.m_instanceId);
        EXPECT_TRUE(activateResult.m_success) << activateResult.m_error.c_str();

        runtime.DestroyScriptInstance(instanceResult.m_instanceId);
    }

    TEST_F(AngelScriptRuntimeTests, ColorRayAndDebugApi_ReturnSdkAvailability)
    {
        AngelScriptRuntime runtime;

        const ScriptCompileResult compileResult = runtime.CompileScriptString(
            "ColorRayAndDebugApi",
            "class ColorRayAndDebugApi { void OnActivate() { "
            "Color lineColor(1.0f, 0.25f, 0.0f, 1.0f); "
            "Color hitColor(0.0f, 1.0f, 0.25f, 1.0f); "
            "Ray ray(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), 5.0f); "
            "Debug::DrawLine(ray.origin, ray.origin + ray.direction * ray.distance, lineColor); "
            "Debug::DrawRay(ray, lineColor); "
            "RayHit hit = Physics::CastRay(ray); "
            "Debug::DrawHitRect(hit, 0.25f, hitColor); "
            "Debug::DrawRaycast(ray, hit, lineColor, hitColor, 0.25f); "
            "} }");
        EXPECT_EQ(compileResult.m_success, runtime.IsSdkAvailable()) << compileResult.m_error.c_str();

        ScriptInstanceResult instanceResult = runtime.CreateScriptInstance("ColorRayAndDebugApi", "ColorRayAndDebugApi");
        EXPECT_EQ(instanceResult.m_success, runtime.IsSdkAvailable());
        if (!runtime.IsSdkAvailable())
        {
            EXPECT_FALSE(instanceResult.m_error.empty());
            return;
        }

        const ScriptCompileResult activateResult = runtime.ExecuteScriptInstanceOnActivate(instanceResult.m_instanceId);
        EXPECT_TRUE(activateResult.m_success) << activateResult.m_error.c_str();

        runtime.DestroyScriptInstance(instanceResult.m_instanceId);
    }

    TEST_F(AngelScriptRuntimeTests, UnrealStyleLineTraceApi_ReturnSdkAvailability)
    {
        AngelScriptRuntime runtime;

        const ScriptCompileResult compileResult = runtime.CompileScriptString(
            "UnrealStyleLineTraceApi",
            "class UnrealStyleLineTraceApi { "
            "Entity owner; "
            "void SetEntity(Entity entity) { owner = entity; } "
            "void OnActivate() { "
            "Vec3 start(0.0f, 0.0f, 0.0f); "
            "Vec3 end(10.0f, 0.0f, 0.0f); "
            "TraceHit hit = Physics::LineTrace("
            "start, "
            "end, "
            "TraceChannel::Visibility, "
            "TraceQuery().Ignore(owner).DrawDebug(DebugTrace::ForDuration, 1.0f)); "
            "if (hit.hit) { Debug::DrawHitRect(hit, 0.25f, Color(0.1f, 1.0f, 0.3f, 1.0f)); } "
            "} }");
        EXPECT_EQ(compileResult.m_success, runtime.IsSdkAvailable()) << compileResult.m_error.c_str();

        ScriptInstanceResult instanceResult = runtime.CreateScriptInstance("UnrealStyleLineTraceApi", "UnrealStyleLineTraceApi");
        EXPECT_EQ(instanceResult.m_success, runtime.IsSdkAvailable());
        if (!runtime.IsSdkAvailable())
        {
            EXPECT_FALSE(instanceResult.m_error.empty());
            return;
        }

        const ScriptCompileResult entityResult = runtime.ExecuteScriptInstanceSetEntity(instanceResult.m_instanceId, AZ::EntityId(123));
        EXPECT_TRUE(entityResult.m_success) << entityResult.m_error.c_str();

        const ScriptCompileResult activateResult = runtime.ExecuteScriptInstanceOnActivate(instanceResult.m_instanceId);
        EXPECT_TRUE(activateResult.m_success) << activateResult.m_error.c_str();

        runtime.DestroyScriptInstance(instanceResult.m_instanceId);
    }

    TEST_F(AngelScriptRuntimeTests, RayProbeSample_CompilesWithLineTraceApi_ReturnSdkAvailability)
    {
        AngelScriptRuntime runtime;

        const AZStd::string scriptPath = GetRayProbeScriptPath();
        ASSERT_FALSE(scriptPath.empty());

        const ScriptCompileResult compileResult = runtime.CompileScriptFile("RayProbeSample", scriptPath.c_str());
        EXPECT_EQ(compileResult.m_success, runtime.IsSdkAvailable()) << compileResult.m_error.c_str();

        ScriptInstanceResult instanceResult = runtime.CreateScriptInstance("RayProbeSample", "RayProbe");
        EXPECT_EQ(instanceResult.m_success, runtime.IsSdkAvailable());
        if (!runtime.IsSdkAvailable())
        {
            EXPECT_FALSE(instanceResult.m_error.empty());
            return;
        }

        EXPECT_TRUE(instanceResult.m_hasSetEntity);
        EXPECT_TRUE(instanceResult.m_hasOnActivate);
        EXPECT_TRUE(instanceResult.m_hasOnDeactivate);
        EXPECT_TRUE(instanceResult.m_hasOnTick);

        const ScriptCompileResult entityResult = runtime.ExecuteScriptInstanceSetEntity(instanceResult.m_instanceId, AZ::EntityId(123));
        EXPECT_TRUE(entityResult.m_success) << entityResult.m_error.c_str();

        const ScriptCompileResult activateResult = runtime.ExecuteScriptInstanceOnActivate(instanceResult.m_instanceId);
        EXPECT_TRUE(activateResult.m_success) << activateResult.m_error.c_str();

        runtime.DestroyScriptInstance(instanceResult.m_instanceId);
    }

    TEST_F(AngelScriptRuntimeTests, DebugDrawQueue_FlushesQueuedLineAndHitRect)
    {
        AngelScriptRuntime runtime;

        const ScriptCompileResult compileResult = runtime.CompileScriptString(
            "DebugDrawQueueFlush",
            "class DebugDrawQueueFlush { void OnActivate() { "
            "Color lineColor(1.0f, 0.0f, 0.0f, 1.0f); "
            "Color hitColor(0.0f, 1.0f, 0.0f, 1.0f); "
            "Debug::DrawLine(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), lineColor); "
            "RayHit hit; "
            "hit.hit = true; "
            "hit.position = Vec3(2.0f, 0.0f, 0.0f); "
            "hit.normal = Vec3(0.0f, 0.0f, 1.0f); "
            "Debug::DrawHitRect(hit, 1.0f, hitColor); "
            "} }");
        EXPECT_EQ(compileResult.m_success, runtime.IsSdkAvailable()) << compileResult.m_error.c_str();

        ScriptInstanceResult instanceResult = runtime.CreateScriptInstance("DebugDrawQueueFlush", "DebugDrawQueueFlush");
        EXPECT_EQ(instanceResult.m_success, runtime.IsSdkAvailable());
        if (!runtime.IsSdkAvailable())
        {
            EXPECT_FALSE(instanceResult.m_error.empty());
            return;
        }

        const ScriptCompileResult activateResult = runtime.ExecuteScriptInstanceOnActivate(instanceResult.m_instanceId);
        EXPECT_TRUE(activateResult.m_success) << activateResult.m_error.c_str();

        UnitTest::TestDebugDisplayRequests debugDisplay;
        const AzFramework::ViewportInfo viewportInfo{ 0 };
        FlushQueuedDebugDraw(viewportInfo, debugDisplay);
        EXPECT_GE(debugDisplay.GetPoints().size(), 10);

        runtime.DestroyScriptInstance(instanceResult.m_instanceId);
    }

    TEST_F(AngelScriptRuntimeTests, TransformBus_TransformAndEntityArrayCoverage_ReturnSdkAvailability)
    {
        if (!HasBehaviorContextEbusEvent("TransformBus", "GetWorldX"))
        {
            GTEST_SKIP() << "TransformBus is not reflected in the current unit-test BehaviorContext.";
        }

        AngelScriptRuntime runtime;

        const ScriptCompileResult getLocalTmCompileResult = runtime.CompileScriptString(
            "TransformBusGetLocalTmCoverage",
            "class TransformBusGetLocalTmCoverage { Entity self; void SetEntity(Entity entity) { self = entity; } void OnActivate() { Transform local = TransformBus::GetLocalTM(self); Transform world = TransformBus::GetLocalTM(self); world.basisX = Vec3(1.0f, 0.0f, 0.0f); world.basisY = Vec3(0.0f, 1.0f, 0.0f); world.basisZ = Vec3(0.0f, 0.0f, 1.0f); world.translation = Vec3(1.0f, 2.0f, 3.0f); } }");
        EXPECT_EQ(getLocalTmCompileResult.m_success, runtime.IsSdkAvailable()) << getLocalTmCompileResult.m_error.c_str();

        const ScriptCompileResult setLocalTmCompileResult = runtime.CompileScriptString(
            "TransformBusSetLocalTmCoverage",
            "class TransformBusSetLocalTmCoverage { Entity self; void SetEntity(Entity entity) { self = entity; } void OnActivate() { Transform local; TransformBus::SetLocalTM(self, local); TransformBus::SetWorldTM(self, local); } }");
        EXPECT_EQ(setLocalTmCompileResult.m_success, runtime.IsSdkAvailable()) << setLocalTmCompileResult.m_error.c_str();

        const ScriptCompileResult outRefCompileResult = runtime.CompileScriptString(
            "TransformBusOutRefCoverage",
            "class TransformBusOutRefCoverage { Entity self; void SetEntity(Entity entity) { self = entity; } void OnActivate() { Transform localOut; Transform worldOut; TransformBus::GetLocalAndWorld(self, localOut, worldOut); } }");
        EXPECT_EQ(outRefCompileResult.m_success, runtime.IsSdkAvailable()) << outRefCompileResult.m_error.c_str();

        const ScriptCompileResult childrenCompileResult = runtime.CompileScriptString(
            "TransformBusChildrenCoverage",
            "class TransformBusChildrenCoverage { Entity self; void SetEntity(Entity entity) { self = entity; } void OnActivate() { array<Entity>@ children = TransformBus::GetChildren(self); if (children is null) { Debug::Error(\"TransformBus child array marshalling failed\"); } } }");
        EXPECT_EQ(childrenCompileResult.m_success, runtime.IsSdkAvailable()) << childrenCompileResult.m_error.c_str();

        if (!runtime.IsSdkAvailable())
        {
            return;
        }

        const ScriptCompileResult allDescendantsCompileResult = runtime.CompileScriptString(
            "TransformBusAllDescendantsCoverage",
            "class TransformBusAllDescendantsCoverage { Entity self; void SetEntity(Entity entity) { self = entity; } void OnActivate() { array<Entity>@ descendants = TransformBus::GetAllDescendants(self); if (descendants is null) { Debug::Error(\"TransformBus descendant array marshalling failed\"); } } }");
        EXPECT_EQ(allDescendantsCompileResult.m_success, runtime.IsSdkAvailable()) << allDescendantsCompileResult.m_error.c_str();

        const ScriptCompileResult entityAndDescendantsCompileResult = runtime.CompileScriptString(
            "TransformBusEntityAndDescendantsCoverage",
            "class TransformBusEntityAndDescendantsCoverage { Entity self; void SetEntity(Entity entity) { self = entity; } void OnActivate() { array<Entity>@ hierarchy = TransformBus::GetEntityAndAllDescendants(self); if (hierarchy is null) { Debug::Error(\"TransformBus hierarchy array marshalling failed\"); } } }");
        EXPECT_EQ(entityAndDescendantsCompileResult.m_success, runtime.IsSdkAvailable()) << entityAndDescendantsCompileResult.m_error.c_str();
    }

    TEST_F(AngelScriptRuntimeTests, NonTransformBus_TransformReturnCoverage_RoundTripsThroughBehaviorContext)
    {
        if (!HasBehaviorContextEbusEvent("AttachmentComponentRequestBus", "GetOffset") ||
            !HasBehaviorContextEbusEvent("HeightfieldProviderRequestsBus", "GetHeightfieldTransform") ||
            !HasBehaviorContextEbusEvent("ViewportRequestBus", "GetCameraTransform"))
        {
            GTEST_SKIP() << "One or more non-TransformBus Transform-return methods are not reflected in the current unit-test BehaviorContext.";
        }

        const AZ::EntityId testEntityId(123);
        constexpr AzFramework::ViewportId testViewportId = 7;

        TestAttachmentComponentRequests attachmentRequests(testEntityId);
        TestHeightfieldProviderRequests heightfieldRequests(testEntityId);
        TestViewportRequests viewportRequests(testViewportId);

        AngelScriptRuntime runtime;

        const ScriptCompileResult compileResult = runtime.CompileScriptString(
            "NonTransformBusTransformReturnCoverage",
            "class NonTransformBusTransformReturnCoverage { "
            "Entity self; "
            "void SetEntity(Entity entity) { self = entity; } "
            "void OnActivate() { "
            "Transform attachment = AttachmentComponentRequestBus::GetOffset(self); "
            "if (attachment.translation.x != 1.25f || attachment.translation.y != 2.5f || attachment.translation.z != 3.75f) { Debug::Error(\"AttachmentComponentRequestBus::GetOffset marshalling failed\"); } "
            "Transform heightfield = HeightfieldProviderRequestsBus::GetHeightfieldTransform(self); "
            "if (heightfield.translation.x != 4.5f || heightfield.translation.y != 5.5f || heightfield.translation.z != 6.5f) { Debug::Error(\"HeightfieldProviderRequestsBus::GetHeightfieldTransform marshalling failed\"); } "
            "Transform camera = ViewportRequestBus::GetCameraTransform(7); "
            "if (camera.translation.x != 7.25f || camera.translation.y != 8.25f || camera.translation.z != 9.25f) { Debug::Error(\"ViewportRequestBus::GetCameraTransform marshalling failed\"); } "
            "} }");
        EXPECT_EQ(compileResult.m_success, runtime.IsSdkAvailable()) << compileResult.m_error.c_str();

        ScriptInstanceResult instanceResult =
            runtime.CreateScriptInstance("NonTransformBusTransformReturnCoverage", "NonTransformBusTransformReturnCoverage");
        EXPECT_EQ(instanceResult.m_success, runtime.IsSdkAvailable());
        if (!runtime.IsSdkAvailable())
        {
            EXPECT_FALSE(instanceResult.m_error.empty());
            return;
        }

        const ScriptCompileResult entityResult = runtime.ExecuteScriptInstanceSetEntity(instanceResult.m_instanceId, testEntityId);
        EXPECT_TRUE(entityResult.m_success) << entityResult.m_error.c_str();

        const ScriptCompileResult activateResult = runtime.ExecuteScriptInstanceOnActivate(instanceResult.m_instanceId);
        EXPECT_TRUE(activateResult.m_success) << activateResult.m_error.c_str();

        runtime.DestroyScriptInstance(instanceResult.m_instanceId);
    }

    TEST_F(AngelScriptRuntimeTests, CompileScriptFile_WithBasicMoverSample_ReturnsSdkAvailability)
    {
        AngelScriptRuntime runtime;

        const AZStd::string scriptPath = GetBasicMoverScriptPath();
        ASSERT_FALSE(scriptPath.empty());

        const ScriptCompileResult compileResult = runtime.CompileScriptFile("BasicMoverSample", scriptPath.c_str());
        EXPECT_EQ(compileResult.m_success, runtime.IsSdkAvailable()) << compileResult.m_error.c_str();
    }

    TEST_F(AngelScriptRuntimeTests, BasicMoverSample_InstanceLifecycle_ReturnsSdkAvailability)
    {
        AngelScriptRuntime runtime;

        const AZStd::string scriptPath = GetBasicMoverScriptPath();
        ASSERT_FALSE(scriptPath.empty());

        const ScriptCompileResult compileResult = runtime.CompileScriptFile("BasicMoverRuntime", scriptPath.c_str());
        EXPECT_EQ(compileResult.m_success, runtime.IsSdkAvailable()) << compileResult.m_error.c_str();

        ScriptInstanceResult instanceResult = runtime.CreateScriptInstance("BasicMoverRuntime", "BasicMover");
        EXPECT_EQ(instanceResult.m_success, runtime.IsSdkAvailable());
        if (!runtime.IsSdkAvailable())
        {
            EXPECT_FALSE(instanceResult.m_error.empty());
            return;
        }

        EXPECT_NE(instanceResult.m_instanceId, InvalidScriptInstanceId);
        EXPECT_TRUE(instanceResult.m_hasSetEntity);
        EXPECT_TRUE(instanceResult.m_hasOnActivate);
        EXPECT_TRUE(instanceResult.m_hasOnDeactivate);
        EXPECT_TRUE(instanceResult.m_hasOnTick);

        const ScriptCompileResult targetPropertyResult =
            runtime.SetScriptInstanceEntityProperty(instanceResult.m_instanceId, "target", AZ::EntityId(456));
        EXPECT_TRUE(targetPropertyResult.m_success) << targetPropertyResult.m_error.c_str();

        const ScriptCompileResult entityResult = runtime.ExecuteScriptInstanceSetEntity(instanceResult.m_instanceId, AZ::EntityId(123));
        EXPECT_TRUE(entityResult.m_success) << entityResult.m_error.c_str();

        const ScriptCompileResult activateResult = runtime.ExecuteScriptInstanceOnActivate(instanceResult.m_instanceId);
        EXPECT_TRUE(activateResult.m_success) << activateResult.m_error.c_str();

        const ScriptCompileResult zeroTickResult = runtime.ExecuteScriptInstanceOnTick(instanceResult.m_instanceId, 0.0f);
        EXPECT_TRUE(zeroTickResult.m_success) << zeroTickResult.m_error.c_str();

        const ScriptCompileResult largeTickResult = runtime.ExecuteScriptInstanceOnTick(instanceResult.m_instanceId, 1.0f);
        EXPECT_TRUE(largeTickResult.m_success) << largeTickResult.m_error.c_str();

        const ScriptCompileResult regularTickResult = runtime.ExecuteScriptInstanceOnTick(instanceResult.m_instanceId, 1.0f / 60.0f);
        EXPECT_TRUE(regularTickResult.m_success) << regularTickResult.m_error.c_str();

        const ScriptCompileResult deactivateResult = runtime.ExecuteScriptInstanceOnDeactivate(instanceResult.m_instanceId);
        EXPECT_TRUE(deactivateResult.m_success) << deactivateResult.m_error.c_str();

        runtime.DestroyScriptInstance(instanceResult.m_instanceId);
    }

    TEST_F(AngelScriptRuntimeTests, SetScriptInstanceEntityProperty_WithEntityField_ReturnsSdkAvailability)
    {
        AngelScriptRuntime runtime;

        const ScriptCompileResult compileResult = runtime.CompileScriptString(
            "SerializedEntity",
            "class SerializedEntityScript { [Visible(Tooltip=\"Entity to follow\")] Entity target; void OnActivate() { Debug::Log(\"Serialized target valid\"); } }");
        EXPECT_EQ(compileResult.m_success, runtime.IsSdkAvailable()) << compileResult.m_error.c_str();

        ScriptInstanceResult instanceResult = runtime.CreateScriptInstance("SerializedEntity", "SerializedEntityScript");
        EXPECT_EQ(instanceResult.m_success, runtime.IsSdkAvailable());
        if (!runtime.IsSdkAvailable())
        {
            EXPECT_FALSE(instanceResult.m_error.empty());
            return;
        }

        const ScriptCompileResult propertyResult =
            runtime.SetScriptInstanceEntityProperty(instanceResult.m_instanceId, "target", AZ::EntityId(123));
        EXPECT_TRUE(propertyResult.m_success) << propertyResult.m_error.c_str();

        const ScriptCompileResult missingPropertyResult =
            runtime.SetScriptInstanceEntityProperty(instanceResult.m_instanceId, "missingTarget", AZ::EntityId(123));
        EXPECT_FALSE(missingPropertyResult.m_success);

        runtime.DestroyScriptInstance(instanceResult.m_instanceId);
    }

    TEST_F(AngelScriptRuntimeTests, RuntimeProbeSample_InstanceLifecycle_ReturnsSdkAvailability)
    {
        AngelScriptRuntime runtime;

        const AZStd::string scriptPath = GetRuntimeProbeScriptPath();
        ASSERT_FALSE(scriptPath.empty());

        const ScriptCompileResult compileResult = runtime.CompileScriptFile("RuntimeProbeSample", scriptPath.c_str());
        EXPECT_EQ(compileResult.m_success, runtime.IsSdkAvailable()) << compileResult.m_error.c_str();

        ScriptInstanceResult instanceResult = runtime.CreateScriptInstance("RuntimeProbeSample", "RuntimeProbe");
        EXPECT_EQ(instanceResult.m_success, runtime.IsSdkAvailable());
        if (!runtime.IsSdkAvailable())
        {
            EXPECT_FALSE(instanceResult.m_error.empty());
            return;
        }

        EXPECT_TRUE(instanceResult.m_hasSetEntity);
        EXPECT_TRUE(instanceResult.m_hasOnActivate);
        EXPECT_TRUE(instanceResult.m_hasOnDeactivate);
        EXPECT_TRUE(instanceResult.m_hasOnTick);

        const ScriptCompileResult targetPropertyResult =
            runtime.SetScriptInstanceEntityProperty(instanceResult.m_instanceId, "target", AZ::EntityId(456));
        EXPECT_TRUE(targetPropertyResult.m_success) << targetPropertyResult.m_error.c_str();

        const ScriptCompileResult alternatePropertyResult =
            runtime.SetScriptInstanceEntityProperty(instanceResult.m_instanceId, "alternate", AZ::EntityId(789));
        EXPECT_TRUE(alternatePropertyResult.m_success) << alternatePropertyResult.m_error.c_str();

        const ScriptCompileResult entityResult = runtime.ExecuteScriptInstanceSetEntity(instanceResult.m_instanceId, AZ::EntityId(123));
        EXPECT_TRUE(entityResult.m_success) << entityResult.m_error.c_str();

        const ScriptCompileResult activateResult = runtime.ExecuteScriptInstanceOnActivate(instanceResult.m_instanceId);
        EXPECT_TRUE(activateResult.m_success) << activateResult.m_error.c_str();

        const ScriptCompileResult zeroTickResult = runtime.ExecuteScriptInstanceOnTick(instanceResult.m_instanceId, 0.0f);
        EXPECT_TRUE(zeroTickResult.m_success) << zeroTickResult.m_error.c_str();

        const ScriptCompileResult largeTickResult = runtime.ExecuteScriptInstanceOnTick(instanceResult.m_instanceId, 1.0f);
        EXPECT_TRUE(largeTickResult.m_success) << largeTickResult.m_error.c_str();

        const ScriptCompileResult regularTickResult = runtime.ExecuteScriptInstanceOnTick(instanceResult.m_instanceId, 1.0f / 30.0f);
        EXPECT_TRUE(regularTickResult.m_success) << regularTickResult.m_error.c_str();

        const ScriptCompileResult deactivateResult = runtime.ExecuteScriptInstanceOnDeactivate(instanceResult.m_instanceId);
        EXPECT_TRUE(deactivateResult.m_success) << deactivateResult.m_error.c_str();

        runtime.DestroyScriptInstance(instanceResult.m_instanceId);
    }

    TEST_F(AngelScriptRuntimeTests, EntityBusProbeSample_InstanceLifecycle_ReturnsSdkAvailability)
    {
        AngelScriptRuntime runtime;

        const AZStd::string scriptPath = GetEntityBusProbeScriptPath();
        ASSERT_FALSE(scriptPath.empty());

        const ScriptCompileResult compileResult = runtime.CompileScriptFile("EntityBusProbeSample", scriptPath.c_str());
        EXPECT_EQ(compileResult.m_success, runtime.IsSdkAvailable()) << compileResult.m_error.c_str();

        ScriptInstanceResult instanceResult = runtime.CreateScriptInstance("EntityBusProbeSample", "EntityBusProbe");
        EXPECT_EQ(instanceResult.m_success, runtime.IsSdkAvailable());
        if (!runtime.IsSdkAvailable())
        {
            EXPECT_FALSE(instanceResult.m_error.empty());
            return;
        }

        const AZ::EntityId targetId(456);
        const ScriptCompileResult targetPropertyResult =
            runtime.SetScriptInstanceEntityProperty(instanceResult.m_instanceId, "target", targetId);
        EXPECT_TRUE(targetPropertyResult.m_success) << targetPropertyResult.m_error.c_str();

        const ScriptCompileResult activateResult = runtime.ExecuteScriptInstanceOnActivate(instanceResult.m_instanceId);
        EXPECT_TRUE(activateResult.m_success) << activateResult.m_error.c_str();

        AZ::EntityBus::Event(targetId, &AZ::EntityBus::Events::OnEntityActivated, targetId);

        const ScriptCompileResult deactivateResult = runtime.ExecuteScriptInstanceOnDeactivate(instanceResult.m_instanceId);
        EXPECT_TRUE(deactivateResult.m_success) << deactivateResult.m_error.c_str();

        runtime.DestroyScriptInstance(instanceResult.m_instanceId);
    }

    TEST_F(AngelScriptRuntimeTests, ConsoleBusCStringDispatch_InstanceLifecycle_ReturnsSdkAvailability)
    {
        AngelScriptRuntime runtime;
        const bool hasBehaviorContext = IsBehaviorContextAvailableForTests();

        const ScriptCompileResult compileResult = runtime.CompileScriptString(
            "ConsoleBusCStringDispatch",
            "class ConsoleBusCStringDispatch { void OnActivate() { ConsoleBus::ExecuteConsoleCommand(\"\"); } }");
        EXPECT_EQ(compileResult.m_success, runtime.IsSdkAvailable() && hasBehaviorContext) << compileResult.m_error.c_str();

        ScriptInstanceResult instanceResult = runtime.CreateScriptInstance("ConsoleBusCStringDispatch", "ConsoleBusCStringDispatch");
        EXPECT_EQ(instanceResult.m_success, runtime.IsSdkAvailable() && hasBehaviorContext);
        if (!runtime.IsSdkAvailable() || !hasBehaviorContext)
        {
            EXPECT_FALSE(instanceResult.m_error.empty());
            return;
        }

        EXPECT_TRUE(instanceResult.m_hasOnActivate);

        const ScriptCompileResult activateResult = runtime.ExecuteScriptInstanceOnActivate(instanceResult.m_instanceId);
        EXPECT_TRUE(activateResult.m_success) << activateResult.m_error.c_str();

        runtime.DestroyScriptInstance(instanceResult.m_instanceId);
    }

    TEST_F(AngelScriptRuntimeTests, SystemComponent_BeforeInit_ReportsRuntimeNotStarted)
    {
        TestableAngelScriptSystemComponent component;

        EXPECT_FALSE(component.IsRuntimeStarted());
        EXPECT_FALSE(component.IsRuntimeSdkAvailable());

        const ScriptCompileResult result = component.CompileScriptString("Smoke", "void Main() {}");
        EXPECT_FALSE(result.m_success);
        EXPECT_FALSE(result.m_error.empty());

        const ScriptCompileResult fileResult = component.CompileScriptFile("Smoke", "missing_script.as");
        EXPECT_FALSE(fileResult.m_success);
        EXPECT_FALSE(fileResult.m_error.empty());
    }

    TEST_F(AngelScriptRuntimeTests, AngelScriptComponent_DefaultState_HasNoScriptAsset)
    {
        AngelScriptComponent component;

        EXPECT_FALSE(component.GetScriptAsset().GetId().IsValid());
        EXPECT_TRUE(component.GetEntityVariables().empty());
    }
} // namespace AngelScript

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
