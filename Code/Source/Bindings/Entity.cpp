#include <Bindings/Entity.h>

#include <Bindings/Math.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Vector3.h>

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
#include <angelscript.h>

#include <string>
#endif

namespace AngelScript
{
    namespace
    {
        constexpr const char* EntityBindingLogWindow = "AngelScript";

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        void ConstructEntity(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptEntity*>(generic->GetObject());
            new (self) ScriptEntity();
        }

        void ScriptEntityIsValid(asIScriptGeneric* generic)
        {
            const auto* entity = reinterpret_cast<const ScriptEntity*>(generic->GetObject());
            generic->SetReturnByte(entity && entity->IsValid());
        }

        void ScriptEntityGetName(asIScriptGeneric* generic)
        {
            std::string name;
            const auto* scriptEntity = reinterpret_cast<const ScriptEntity*>(generic->GetObject());
            if (scriptEntity)
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(
                    entity,
                    &AZ::ComponentApplicationRequests::FindEntity,
                    AZ::EntityId(scriptEntity->m_entityId));

                if (entity)
                {
                    name = entity->GetName().c_str();
                }
            }

            generic->SetReturnObject(&name);
        }

        void ScriptEntityGetWorldPosition(asIScriptGeneric* generic)
        {
            const auto* entity = reinterpret_cast<const ScriptEntity*>(generic->GetObject());
            ScriptVec3 position;
            if (entity)
            {
                const AZ::EntityId entityId(entity->m_entityId);
                if (entityId.IsValid())
                {
                    AZ::Vector3 worldPosition = AZ::Vector3::CreateZero();
                    AZ::TransformBus::EventResult(worldPosition, entityId, &AZ::TransformBus::Events::GetWorldTranslation);
                    position.x = worldPosition.GetX();
                    position.y = worldPosition.GetY();
                    position.z = worldPosition.GetZ();
                }
            }

            *reinterpret_cast<ScriptVec3*>(generic->GetAddressOfReturnLocation()) = position;
        }

        void ScriptEntitySetWorldPosition(asIScriptGeneric* generic)
        {
            const auto* entity = reinterpret_cast<const ScriptEntity*>(generic->GetObject());
            const auto* position = reinterpret_cast<const ScriptVec3*>(generic->GetArgObject(0));
            if (!entity || !position)
            {
                return;
            }

            const AZ::EntityId entityId(entity->m_entityId);
            if (!entityId.IsValid())
            {
                return;
            }

            AZ::TransformBus::Event(
                entityId,
                &AZ::TransformBus::Events::SetWorldTranslation,
                AZ::Vector3(position->x, position->y, position->z));
        }
#endif
    } // namespace

    bool ScriptEntity::IsValid() const
    {
        return AZ::EntityId(m_entityId).IsValid();
    }

    bool RegisterEntity(asIScriptEngine* engine)
    {
#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        int result = engine->RegisterObjectType("Entity", sizeof(ScriptEntity), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS | asOBJ_APP_CLASS_ALLINTS);
        if (result < 0)
        {
            AZ_Error(EntityBindingLogWindow, false, "Failed to register AngelScript Entity type. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("Entity", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructEntity), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(EntityBindingLogWindow, false, "Failed to register AngelScript Entity default constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("Entity", "bool IsValid() const", asFUNCTION(ScriptEntityIsValid), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(EntityBindingLogWindow, false, "Failed to register AngelScript Entity.IsValid. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("Entity", "string GetName() const", asFUNCTION(ScriptEntityGetName), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(EntityBindingLogWindow, false, "Failed to register AngelScript Entity.GetName. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("Entity", "Vec3 GetWorldPosition() const", asFUNCTION(ScriptEntityGetWorldPosition), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(EntityBindingLogWindow, false, "Failed to register AngelScript Entity.GetWorldPosition. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("Entity", "void SetWorldPosition(const Vec3 &in)", asFUNCTION(ScriptEntitySetWorldPosition), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(EntityBindingLogWindow, false, "Failed to register AngelScript Entity.SetWorldPosition. Result: %d", result);
            return false;
        }

        return true;
#else
        (void)engine;
        return false;
#endif
    }
} // namespace AngelScript
