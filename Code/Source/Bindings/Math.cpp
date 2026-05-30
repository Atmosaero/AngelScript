#include <Bindings/Math.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
#include <angelscript.h>
#endif

namespace AngelScript
{
    namespace
    {
        constexpr const char* MathBindingLogWindow = "AngelScript";

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        void ConstructQuat(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptQuat*>(generic->GetObject());
            new (self) ScriptQuat();
        }

        void ConstructQuatXYZW(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptQuat*>(generic->GetObject());
            const float x = generic->GetArgFloat(0);
            const float y = generic->GetArgFloat(1);
            const float z = generic->GetArgFloat(2);
            const float w = generic->GetArgFloat(3);
            new (self) ScriptQuat{ x, y, z, w };
        }

        void ConstructVec2(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptVec2*>(generic->GetObject());
            new (self) ScriptVec2();
        }

        void ConstructVec2XY(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptVec2*>(generic->GetObject());
            const float x = generic->GetArgFloat(0);
            const float y = generic->GetArgFloat(1);
            new (self) ScriptVec2{ x, y };
        }

        void AddVec2(asIScriptGeneric* generic)
        {
            const auto* self = reinterpret_cast<const ScriptVec2*>(generic->GetObject());
            const auto* other = reinterpret_cast<const ScriptVec2*>(generic->GetArgObject(0));
            *reinterpret_cast<ScriptVec2*>(generic->GetAddressOfReturnLocation()) =
                ScriptVec2{ self->x + other->x, self->y + other->y };
        }

        void SubtractVec2(asIScriptGeneric* generic)
        {
            const auto* self = reinterpret_cast<const ScriptVec2*>(generic->GetObject());
            const auto* other = reinterpret_cast<const ScriptVec2*>(generic->GetArgObject(0));
            *reinterpret_cast<ScriptVec2*>(generic->GetAddressOfReturnLocation()) =
                ScriptVec2{ self->x - other->x, self->y - other->y };
        }

        void MultiplyVec2ByFloat(asIScriptGeneric* generic)
        {
            const auto* self = reinterpret_cast<const ScriptVec2*>(generic->GetObject());
            const float scale = generic->GetArgFloat(0);
            *reinterpret_cast<ScriptVec2*>(generic->GetAddressOfReturnLocation()) =
                ScriptVec2{ self->x * scale, self->y * scale };
        }

        void MultiplyFloatByVec2(asIScriptGeneric* generic)
        {
            const float scale = generic->GetArgFloat(0);
            const auto* self = reinterpret_cast<const ScriptVec2*>(generic->GetObject());
            *reinterpret_cast<ScriptVec2*>(generic->GetAddressOfReturnLocation()) =
                ScriptVec2{ self->x * scale, self->y * scale };
        }

        void ConstructVec3(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptVec3*>(generic->GetObject());
            new (self) ScriptVec3();
        }

        void ConstructVec3XYZ(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptVec3*>(generic->GetObject());
            const float x = generic->GetArgFloat(0);
            const float y = generic->GetArgFloat(1);
            const float z = generic->GetArgFloat(2);
            new (self) ScriptVec3{ x, y, z };
        }

        void ConstructColor(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptColor*>(generic->GetObject());
            new (self) ScriptColor();
        }

        void ConstructColorRGBA(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptColor*>(generic->GetObject());
            const float r = generic->GetArgFloat(0);
            const float g = generic->GetArgFloat(1);
            const float b = generic->GetArgFloat(2);
            const float a = generic->GetArgFloat(3);
            new (self) ScriptColor{ r, g, b, a };
        }

        void ConstructTransform(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptTransform*>(generic->GetObject());
            new (self) ScriptTransform();
        }

        void ConstructTransformCopy(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptTransform*>(generic->GetObject());
            const auto* other = reinterpret_cast<const ScriptTransform*>(generic->GetArgObject(0));
            new (self) ScriptTransform(*other);
        }

        void DestructTransform(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptTransform*>(generic->GetObject());
            self->~ScriptTransform();
        }

        void AssignTransform(asIScriptGeneric* generic)
        {
            auto* self = reinterpret_cast<ScriptTransform*>(generic->GetObject());
            const auto* other = reinterpret_cast<const ScriptTransform*>(generic->GetArgObject(0));
            *self = *other;
            generic->SetReturnAddress(self);
        }

        void AddVec3(asIScriptGeneric* generic)
        {
            const auto* self = reinterpret_cast<const ScriptVec3*>(generic->GetObject());
            const auto* other = reinterpret_cast<const ScriptVec3*>(generic->GetArgObject(0));
            *reinterpret_cast<ScriptVec3*>(generic->GetAddressOfReturnLocation()) =
                ScriptVec3{ self->x + other->x, self->y + other->y, self->z + other->z };
        }

        void SubtractVec3(asIScriptGeneric* generic)
        {
            const auto* self = reinterpret_cast<const ScriptVec3*>(generic->GetObject());
            const auto* other = reinterpret_cast<const ScriptVec3*>(generic->GetArgObject(0));
            *reinterpret_cast<ScriptVec3*>(generic->GetAddressOfReturnLocation()) =
                ScriptVec3{ self->x - other->x, self->y - other->y, self->z - other->z };
        }

        void MultiplyVec3ByFloat(asIScriptGeneric* generic)
        {
            const auto* self = reinterpret_cast<const ScriptVec3*>(generic->GetObject());
            const float scale = generic->GetArgFloat(0);
            *reinterpret_cast<ScriptVec3*>(generic->GetAddressOfReturnLocation()) =
                ScriptVec3{ self->x * scale, self->y * scale, self->z * scale };
        }

        void MultiplyFloatByVec3(asIScriptGeneric* generic)
        {
            const float scale = generic->GetArgFloat(0);
            const auto* self = reinterpret_cast<const ScriptVec3*>(generic->GetObject());
            *reinterpret_cast<ScriptVec3*>(generic->GetAddressOfReturnLocation()) =
                ScriptVec3{ self->x * scale, self->y * scale, self->z * scale };
        }
#endif
    } // namespace

    bool RegisterMath(asIScriptEngine* engine)
    {
#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        int result = engine->RegisterObjectType("Vec2", sizeof(ScriptVec2), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<ScriptVec2>() | asOBJ_APP_CLASS_ALLFLOATS);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec2 type. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("Vec2", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructVec2), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec2 default constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("Vec2", asBEHAVE_CONSTRUCT, "void f(float, float)", asFUNCTION(ConstructVec2XY), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec2 float constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("Vec2", "Vec2 opAdd(const Vec2 &in) const", asFUNCTION(AddVec2), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec2 addition. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("Vec2", "Vec2 opSub(const Vec2 &in) const", asFUNCTION(SubtractVec2), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec2 subtraction. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("Vec2", "Vec2 opMul(float) const", asFUNCTION(MultiplyVec2ByFloat), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec2 scalar multiply. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("Vec2", "Vec2 opMul_r(float) const", asFUNCTION(MultiplyFloatByVec2), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec2 reflected scalar multiply. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Vec2", "float x", asOFFSET(ScriptVec2, x));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec2.x. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Vec2", "float y", asOFFSET(ScriptVec2, y));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec2.y. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectType("Quat", sizeof(ScriptQuat), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<ScriptQuat>() | asOBJ_APP_CLASS_ALLFLOATS);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Quat type. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("Quat", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructQuat), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Quat default constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("Quat", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTION(ConstructQuatXYZW), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Quat float constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Quat", "float x", asOFFSET(ScriptQuat, x));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Quat.x. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Quat", "float y", asOFFSET(ScriptQuat, y));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Quat.y. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Quat", "float z", asOFFSET(ScriptQuat, z));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Quat.z. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Quat", "float w", asOFFSET(ScriptQuat, w));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Quat.w. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectType("Vec3", sizeof(ScriptVec3), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<ScriptVec3>() | asOBJ_APP_CLASS_ALLFLOATS);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec3 type. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("Vec3", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructVec3), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec3 default constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("Vec3", asBEHAVE_CONSTRUCT, "void f(float, float, float)", asFUNCTION(ConstructVec3XYZ), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec3 float constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("Vec3", "Vec3 opAdd(const Vec3 &in) const", asFUNCTION(AddVec3), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec3 addition. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("Vec3", "Vec3 opSub(const Vec3 &in) const", asFUNCTION(SubtractVec3), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec3 subtraction. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("Vec3", "Vec3 opMul(float) const", asFUNCTION(MultiplyVec3ByFloat), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec3 scalar multiply. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("Vec3", "Vec3 opMul_r(float) const", asFUNCTION(MultiplyFloatByVec3), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec3 reflected scalar multiply. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Vec3", "float x", asOFFSET(ScriptVec3, x));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec3.x. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Vec3", "float y", asOFFSET(ScriptVec3, y));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec3.y. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Vec3", "float z", asOFFSET(ScriptVec3, z));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Vec3.z. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectType("Color", sizeof(ScriptColor), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<ScriptColor>() | asOBJ_APP_CLASS_ALLFLOATS);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Color type. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("Color", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructColor), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Color default constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("Color", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTION(ConstructColorRGBA), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Color float constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Color", "float r", asOFFSET(ScriptColor, r));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Color.r. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Color", "float g", asOFFSET(ScriptColor, g));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Color.g. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Color", "float b", asOFFSET(ScriptColor, b));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Color.b. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Color", "float a", asOFFSET(ScriptColor, a));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Color.a. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectType(
            "Transform",
            sizeof(ScriptTransform),
            asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Transform type. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("Transform", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructTransform), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Transform default constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("Transform", asBEHAVE_CONSTRUCT, "void f(const Transform &in)", asFUNCTION(ConstructTransformCopy), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Transform copy constructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectBehaviour("Transform", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructTransform), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Transform destructor. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectMethod("Transform", "Transform& opAssign(const Transform &in)", asFUNCTION(AssignTransform), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Transform assignment. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Transform", "Vec3 basisX", asOFFSET(ScriptTransform, basisX));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Transform.basisX. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Transform", "Vec3 basisY", asOFFSET(ScriptTransform, basisY));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Transform.basisY. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Transform", "Vec3 basisZ", asOFFSET(ScriptTransform, basisZ));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Transform.basisZ. Result: %d", result);
            return false;
        }

        result = engine->RegisterObjectProperty("Transform", "Vec3 translation", asOFFSET(ScriptTransform, translation));
        if (result < 0)
        {
            AZ_Error(MathBindingLogWindow, false, "Failed to register AngelScript Transform.translation. Result: %d", result);
            return false;
        }

        return true;
#else
        (void)engine;
        return false;
#endif
    }
} // namespace AngelScript
