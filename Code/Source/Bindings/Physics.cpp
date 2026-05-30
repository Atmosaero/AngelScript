#include <Bindings/Physics.h>

#include <Bindings/Debug.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzCore/std/containers/array.h>

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
#include <angelscript.h>
#endif

namespace AngelScript
{
    namespace
    {
        constexpr const char* PhysicsBindingLogWindow = "AngelScript";
        constexpr int DebugTraceNone = 0;
        constexpr int DebugTraceForOneFrame = 1;
        constexpr int DebugTraceForDuration = 2;
        constexpr int DebugTracePersistent = 3;
        constexpr int TraceChannelVisibility = 0;

        AZ::Vector3 ToAzVector3(const ScriptVec3& value)
        {
            return AZ::Vector3(value.x, value.y, value.z);
        }

        ScriptVec3 ToScriptVec3(const AZ::Vector3& value)
        {
            return ScriptVec3{ value.GetX(), value.GetY(), value.GetZ() };
        }

        AzPhysics::SceneHandle ResolveSceneHandle()
        {
            AzPhysics::SceneHandle sceneHandle = AzPhysics::InvalidSceneHandle;
            Physics::DefaultWorldBus::BroadcastResult(sceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);
            if (sceneHandle != AzPhysics::InvalidSceneHandle)
            {
                return sceneHandle;
            }

            Physics::EditorWorldBus::BroadcastResult(sceneHandle, &Physics::EditorWorldRequests::GetEditorSceneHandle);
            return sceneHandle;
        }

        AzPhysics::CollisionGroup ResolveTraceChannelCollisionGroup(int traceChannel)
        {
            if (traceChannel != TraceChannelVisibility)
            {
                return AzPhysics::CollisionGroup::All;
            }

            struct CachedTraceChannelResolution
            {
                bool m_attempted = false;
                AzPhysics::CollisionGroup m_collisionGroup = AzPhysics::CollisionGroup::All;
            };

            static CachedTraceChannelResolution visibilityResolution;
            if (visibilityResolution.m_attempted)
            {
                return visibilityResolution.m_collisionGroup;
            }

            // Mark the lookup as attempted before probing PhysX so repeated calls in the
            // same session cannot re-enter the name-resolution path and spam warnings.
            visibilityResolution.m_attempted = true;

            const AZStd::array<AZStd::string, 4> groupNameCandidates{
                AZStd::string("Visibility"),
                AZStd::string("visibility"),
                AZStd::string("group:Visibility"),
                AZStd::string("Group:Visibility")
            };

            for (const AZStd::string& groupName : groupNameCandidates)
            {
                bool foundVisibilityGroup = false;
                AzPhysics::CollisionGroup visibilityGroup = AzPhysics::CollisionGroup::All;
                Physics::CollisionRequestBus::BroadcastResult(
                    foundVisibilityGroup,
                    &Physics::CollisionRequests::TryGetCollisionGroupByName,
                    groupName,
                    visibilityGroup);

                if (foundVisibilityGroup)
                {
                    visibilityResolution.m_collisionGroup = visibilityGroup;
                    return visibilityResolution.m_collisionGroup;
                }
            }

            const AZStd::array<AZStd::string, 2> layerNameCandidates{
                AZStd::string("Visibility"),
                AZStd::string("visibility")
            };

            for (const AZStd::string& layerName : layerNameCandidates)
            {
                bool foundVisibilityLayer = false;
                AzPhysics::CollisionLayer visibilityLayer;
                Physics::CollisionRequestBus::BroadcastResult(
                    foundVisibilityLayer,
                    &Physics::CollisionRequests::TryGetCollisionLayerByName,
                    layerName,
                    visibilityLayer);

                if (foundVisibilityLayer)
                {
                    AzPhysics::CollisionGroup visibilityGroup = AzPhysics::CollisionGroup::None;
                    visibilityGroup.SetLayer(visibilityLayer, true);
                    visibilityResolution.m_collisionGroup = visibilityGroup;
                    return visibilityResolution.m_collisionGroup;
                }
            }

            static bool warnedMissingVisibilityGroup = false;
            if (!warnedMissingVisibilityGroup)
            {
                AZ_Warning(
                    PhysicsBindingLogWindow,
                    false,
                    "AngelScript TraceChannel::Visibility requested, but no PhysX collision group or layer named 'Visibility' is configured or loaded. Falling back to CollisionGroup::All.");
                warnedMissingVisibilityGroup = true;
            }

            visibilityResolution.m_collisionGroup = AzPhysics::CollisionGroup::All;
            return visibilityResolution.m_collisionGroup;
        }

        ScriptRay MakeRayFromLine(const ScriptVec3& start, const ScriptVec3& end)
        {
            const AZ::Vector3 startVector = ToAzVector3(start);
            const AZ::Vector3 endVector = ToAzVector3(end);
            const AZ::Vector3 delta = endVector - startVector;

            ScriptRay ray;
            ray.origin = start;
            ray.distance = delta.GetLength();
            if (ray.distance > AZ::Constants::FloatEpsilon)
            {
                ray.direction = ToScriptVec3(delta / ray.distance);
            }
            return ray;
        }

        ScriptRayHit PerformRaycast(const ScriptRay& ray, int traceChannel, const ScriptTraceQuery* query)
        {
            ScriptRayHit scriptHit;

            const AZ::Vector3 direction = ToAzVector3(ray.direction);
            const float directionLengthSq = direction.GetLengthSq();
            if (directionLengthSq <= AZ::Constants::FloatEpsilon || ray.distance <= 0.0f)
            {
                return scriptHit;
            }

            const AzPhysics::SceneHandle sceneHandle = ResolveSceneHandle();
            auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
            if (sceneHandle == AzPhysics::InvalidSceneHandle || sceneInterface == nullptr)
            {
                return scriptHit;
            }

            AzPhysics::RayCastRequest request;
            request.m_start = ToAzVector3(ray.origin);
            request.m_direction = direction.GetNormalized();
            request.m_distance = ray.distance;
            request.m_hitFlags = AzPhysics::SceneQuery::HitFlags::Default;
            request.m_collisionGroup = ResolveTraceChannelCollisionGroup(traceChannel);
            const AZ::EntityId ignoredEntityId = query != nullptr ? AZ::EntityId(query->ignoredEntity.m_entityId) : AZ::EntityId();
            if (ignoredEntityId.IsValid())
            {
                request.m_filterCallback = [ignoredEntityId](const AzPhysics::SimulatedBody* body, const Physics::Shape*)
                {
                    if (body != nullptr && body->GetEntityId() == ignoredEntityId)
                    {
                        return AzPhysics::SceneQuery::QueryHitType::None;
                    }

                    return AzPhysics::SceneQuery::QueryHitType::Block;
                };
            }

            AzPhysics::SceneQueryHits hits;
            if (!sceneInterface->QueryScene(sceneHandle, &request, hits) || hits.m_hits.empty())
            {
                return scriptHit;
            }

            for (const AzPhysics::SceneQueryHit& hit : hits.m_hits)
            {
                if (!hit.IsValid())
                {
                    continue;
                }

                if (ignoredEntityId.IsValid() && hit.m_entityId == ignoredEntityId)
                {
                    continue;
                }

                scriptHit.hit = true;
                scriptHit.entity.m_entityId = hit.m_entityId.IsValid() ? static_cast<AZ::u64>(hit.m_entityId) : AZ::EntityId::InvalidEntityId;
                scriptHit.position = ToScriptVec3(hit.m_position);
                scriptHit.normal = ToScriptVec3(hit.m_normal);
                scriptHit.distance = hit.m_distance;
                return scriptHit;
            }

            return scriptHit;
        }

        void DrawTraceDebug(const ScriptRay& ray, const ScriptRayHit& hit, const ScriptTraceQuery* query)
        {
            if (query == nullptr || query->debugTrace == DebugTraceNone)
            {
                return;
            }

            float duration = 0.0f;
            if (query->debugTrace == DebugTraceForDuration)
            {
                duration = AZStd::max(query->debugDuration, 0.0f);
            }
            else if (query->debugTrace == DebugTracePersistent)
            {
                duration = -1.0f;
            }

            const ScriptColor lineColor{ 1.0f, 0.55f, 0.1f, 1.0f };
            const ScriptColor hitColor{ 0.1f, 1.0f, 0.3f, 1.0f };
            const ScriptColor missColor{ 1.0f, 0.2f, 0.2f, 1.0f };
            if (hit.hit)
            {
                QueueScriptRaycastDebug(ray, hit, lineColor, hitColor, 0.25f, false, duration);
            }
            else
            {
                QueueScriptRayDebug(ray, missColor, duration);
            }
        }

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        void ConstructRay(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptRay*>(generic->GetObject());
            new (self) ScriptRay();
        }

        void ConstructRayFromArgs(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptRay*>(generic->GetObject());
            const auto* origin = reinterpret_cast<const ScriptVec3*>(generic->GetArgObject(0));
            const auto* direction = reinterpret_cast<const ScriptVec3*>(generic->GetArgObject(1));
            const float distance = generic->GetArgFloat(2);
            new (self) ScriptRay{
                origin ? *origin : ScriptVec3{},
                direction ? *direction : ScriptVec3{},
                distance
            };
        }

        void ConstructRayCopy(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptRay*>(generic->GetObject());
            const auto* other = reinterpret_cast<const ScriptRay*>(generic->GetArgObject(0));
            new (self) ScriptRay(*other);
        }

        void DestructRay(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptRay*>(generic->GetObject());
            self->~ScriptRay();
        }

        void AssignRay(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptRay*>(generic->GetObject());
            const auto* other = reinterpret_cast<const ScriptRay*>(generic->GetArgObject(0));
            *self = *other;
            generic->SetReturnAddress(self);
        }

        void ConstructRayHit(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptRayHit*>(generic->GetObject());
            new (self) ScriptRayHit();
        }

        void ConstructRayHitCopy(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptRayHit*>(generic->GetObject());
            const auto* other = reinterpret_cast<const ScriptRayHit*>(generic->GetArgObject(0));
            new (self) ScriptRayHit(*other);
        }

        void DestructRayHit(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptRayHit*>(generic->GetObject());
            self->~ScriptRayHit();
        }

        void AssignRayHit(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptRayHit*>(generic->GetObject());
            const auto* other = reinterpret_cast<const ScriptRayHit*>(generic->GetArgObject(0));
            *self = *other;
            generic->SetReturnAddress(self);
        }

        void CastRay(asIScriptGeneric* generic)
        {
            const auto* ray = reinterpret_cast<const ScriptRay*>(generic->GetArgObject(0));
            if (!ray)
            {
                *reinterpret_cast<ScriptRayHit*>(generic->GetAddressOfReturnLocation()) = ScriptRayHit();
                return;
            }

            const ScriptRayHit scriptHit = PerformRaycast(*ray, TraceChannelVisibility, nullptr);
            *reinterpret_cast<ScriptRayHit*>(generic->GetAddressOfReturnLocation()) = scriptHit;
        }

        void ConstructTraceQuery(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptTraceQuery*>(generic->GetObject());
            new (self) ScriptTraceQuery();
        }

        void ConstructTraceQueryCopy(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptTraceQuery*>(generic->GetObject());
            const auto* other = reinterpret_cast<const ScriptTraceQuery*>(generic->GetArgObject(0));
            new (self) ScriptTraceQuery(other ? *other : ScriptTraceQuery());
        }

        void DestructTraceQuery(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptTraceQuery*>(generic->GetObject());
            self->~ScriptTraceQuery();
        }

        void AssignTraceQuery(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptTraceQuery*>(generic->GetObject());
            const auto* other = reinterpret_cast<const ScriptTraceQuery*>(generic->GetArgObject(0));
            if (other)
            {
                *self = *other;
            }
            generic->SetReturnAddress(self);
        }

        void TraceQueryIgnore(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptTraceQuery*>(generic->GetObject());
            const auto* entity = reinterpret_cast<const ScriptEntity*>(generic->GetArgObject(0));
            if (self && entity)
            {
                self->ignoredEntity = *entity;
            }
            generic->SetReturnAddress(self);
        }

        void TraceQueryDrawDebug(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptTraceQuery*>(generic->GetObject());
            if (self)
            {
                self->debugTrace = generic->GetArgDWord(0);
                self->debugDuration = generic->GetArgFloat(1);
            }
            generic->SetReturnAddress(self);
        }

        void LineTraceSimple(asIScriptGeneric* generic)
        {
            const auto* start = reinterpret_cast<const ScriptVec3*>(generic->GetArgObject(0));
            const auto* end = reinterpret_cast<const ScriptVec3*>(generic->GetArgObject(1));
            if (!start || !end)
            {
                *reinterpret_cast<ScriptRayHit*>(generic->GetAddressOfReturnLocation()) = ScriptRayHit();
                return;
            }

            const ScriptRay ray = MakeRayFromLine(*start, *end);
            const ScriptRayHit scriptHit = PerformRaycast(ray, TraceChannelVisibility, nullptr);
            *reinterpret_cast<ScriptRayHit*>(generic->GetAddressOfReturnLocation()) = scriptHit;
        }

        void LineTraceWithQuery(asIScriptGeneric* generic)
        {
            const auto* start = reinterpret_cast<const ScriptVec3*>(generic->GetArgObject(0));
            const auto* end = reinterpret_cast<const ScriptVec3*>(generic->GetArgObject(1));
            const int traceChannel = generic->GetArgDWord(2);
            const auto* query = reinterpret_cast<const ScriptTraceQuery*>(generic->GetArgObject(3));
            if (!start || !end)
            {
                *reinterpret_cast<ScriptRayHit*>(generic->GetAddressOfReturnLocation()) = ScriptRayHit();
                return;
            }

            const ScriptRay ray = MakeRayFromLine(*start, *end);
            const ScriptRayHit scriptHit = PerformRaycast(ray, traceChannel, query);
            DrawTraceDebug(ray, scriptHit, query);
            *reinterpret_cast<ScriptRayHit*>(generic->GetAddressOfReturnLocation()) = scriptHit;
        }
#endif
    } // namespace

    bool RegisterPhysics(asIScriptEngine* engine)
    {
#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        int result = engine->RegisterObjectType("Ray", sizeof(ScriptRay), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript Ray type. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("Ray", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructRay), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript Ray default constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("Ray", asBEHAVE_CONSTRUCT, "void f(const Vec3 &in, const Vec3 &in, float)", asFUNCTION(ConstructRayFromArgs), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript Ray argument constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("Ray", asBEHAVE_CONSTRUCT, "void f(const Ray &in)", asFUNCTION(ConstructRayCopy), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript Ray copy constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("Ray", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructRay), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript Ray destructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("Ray", "Ray& opAssign(const Ray &in)", asFUNCTION(AssignRay), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript Ray assignment. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Ray", "Vec3 origin", asOFFSET(ScriptRay, origin));
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript Ray.origin. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Ray", "Vec3 direction", asOFFSET(ScriptRay, direction));
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript Ray.direction. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Ray", "float distance", asOFFSET(ScriptRay, distance));
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript Ray.distance. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectType("RayHit", sizeof(ScriptRayHit), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript RayHit type. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("RayHit", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructRayHit), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript RayHit default constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("RayHit", asBEHAVE_CONSTRUCT, "void f(const RayHit &in)", asFUNCTION(ConstructRayHitCopy), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript RayHit copy constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("RayHit", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructRayHit), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript RayHit destructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("RayHit", "RayHit& opAssign(const RayHit &in)", asFUNCTION(AssignRayHit), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript RayHit assignment. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("RayHit", "bool hit", asOFFSET(ScriptRayHit, hit));
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript RayHit.hit. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("RayHit", "Entity entity", asOFFSET(ScriptRayHit, entity));
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript RayHit.entity. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("RayHit", "Vec3 position", asOFFSET(ScriptRayHit, position));
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript RayHit.position. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("RayHit", "Vec3 normal", asOFFSET(ScriptRayHit, normal));
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript RayHit.normal. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("RayHit", "float distance", asOFFSET(ScriptRayHit, distance));
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript RayHit.distance. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectType("TraceHit", sizeof(ScriptRayHit), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceHit type. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("TraceHit", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructRayHit), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceHit default constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("TraceHit", asBEHAVE_CONSTRUCT, "void f(const TraceHit &in)", asFUNCTION(ConstructRayHitCopy), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceHit copy constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("TraceHit", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructRayHit), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceHit destructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("TraceHit", "TraceHit& opAssign(const TraceHit &in)", asFUNCTION(AssignRayHit), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceHit assignment. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("TraceHit", "bool hit", asOFFSET(ScriptRayHit, hit));
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceHit.hit. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("TraceHit", "Entity entity", asOFFSET(ScriptRayHit, entity));
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceHit.entity. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("TraceHit", "Vec3 position", asOFFSET(ScriptRayHit, position));
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceHit.position. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("TraceHit", "Vec3 normal", asOFFSET(ScriptRayHit, normal));
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceHit.normal. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("TraceHit", "float distance", asOFFSET(ScriptRayHit, distance));
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceHit.distance. Result: %d", result);
            return false;
        }

        result = engine->RegisterEnum("TraceChannel");
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceChannel enum. Result: %d", result);
            return false;
        }

        result = engine->RegisterEnumValue("TraceChannel", "Visibility", TraceChannelVisibility);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceChannel::Visibility. Result: %d", result);
            return false;
        }

        result = engine->RegisterEnum("DebugTrace");
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript DebugTrace enum. Result: %d", result);
            return false;
        }

        result = engine->RegisterEnumValue("DebugTrace", "None", DebugTraceNone);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript DebugTrace::None. Result: %d", result);
            return false;
        }

        result = engine->RegisterEnumValue("DebugTrace", "ForOneFrame", DebugTraceForOneFrame);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript DebugTrace::ForOneFrame. Result: %d", result);
            return false;
        }

        result = engine->RegisterEnumValue("DebugTrace", "ForDuration", DebugTraceForDuration);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript DebugTrace::ForDuration. Result: %d", result);
            return false;
        }

        result = engine->RegisterEnumValue("DebugTrace", "Persistent", DebugTracePersistent);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript DebugTrace::Persistent. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectType("TraceQuery", sizeof(ScriptTraceQuery), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceQuery type. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("TraceQuery", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructTraceQuery), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceQuery default constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("TraceQuery", asBEHAVE_CONSTRUCT, "void f(const TraceQuery &in)", asFUNCTION(ConstructTraceQueryCopy), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceQuery copy constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("TraceQuery", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructTraceQuery), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceQuery destructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("TraceQuery", "TraceQuery& opAssign(const TraceQuery &in)", asFUNCTION(AssignTraceQuery), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceQuery assignment. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("TraceQuery", "TraceQuery& Ignore(const Entity &in)", asFUNCTION(TraceQueryIgnore), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceQuery.Ignore. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("TraceQuery", "TraceQuery& DrawDebug(DebugTrace, float = 0.0f)", asFUNCTION(TraceQueryDrawDebug), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript TraceQuery.DrawDebug. Result: %d", result);
            return false;
        }

        result = engine->SetDefaultNamespace("Physics");
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to set AngelScript Physics namespace. Result: %d", result);
            return false;
        }

        result = engine->RegisterGlobalFunction("RayHit CastRay(const Ray &in)", asFUNCTION(CastRay), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript Physics::CastRay. Result: %d", result);
            return false;
        }

        result = engine->RegisterGlobalFunction("TraceHit LineTrace(const Vec3 &in, const Vec3 &in)", asFUNCTION(LineTraceSimple), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript Physics::LineTrace. Result: %d", result);
            return false;
        }

        result = engine->RegisterGlobalFunction(
            "TraceHit LineTrace(const Vec3 &in, const Vec3 &in, TraceChannel, const TraceQuery &in)",
            asFUNCTION(LineTraceWithQuery),
            asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to register AngelScript Physics::LineTrace with query. Result: %d", result);
            return false;
        }

        result = engine->SetDefaultNamespace("");
        if (result < 0)
        {
            AZ_Error(PhysicsBindingLogWindow, false, "Failed to reset AngelScript namespace after Physics. Result: %d", result);
            return false;
        }

        return true;
#else
        (void)engine;
        return false;
#endif
    }
} // namespace AngelScript
