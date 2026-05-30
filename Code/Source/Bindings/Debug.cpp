#include <Bindings/Debug.h>

#include <Bindings/Math.h>
#include <Bindings/Physics.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/mutex.h>

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
#include <angelscript.h>

#include <string>
#endif

namespace AngelScript
{
    namespace
    {
        constexpr const char* DebugBindingLogWindow = "AngelScript";

        struct DebugLineCommand
        {
            AZ::Vector3 m_start = AZ::Vector3::CreateZero();
            AZ::Vector3 m_end = AZ::Vector3::CreateZero();
            AZ::Color m_color = AZ::Colors::White;
            float m_expireTime = 0.0f;
            bool m_persistent = false;
        };

        struct DebugRectCommand
        {
            AZ::Vector3 m_center = AZ::Vector3::CreateZero();
            AZ::Vector3 m_normal = AZ::Vector3::CreateAxisZ();
            float m_size = 0.1f;
            AZ::Color m_color = AZ::Colors::White;
            float m_expireTime = 0.0f;
            bool m_persistent = false;
        };

        struct DebugDrawState
        {
            AZStd::mutex m_mutex;
            AZStd::vector<DebugLineCommand> m_lines;
            AZStd::vector<DebugRectCommand> m_rects;
            float m_time = 0.0f;
        };

        DebugDrawState& GetDebugDrawState()
        {
            static DebugDrawState state;
            return state;
        }

        AZ::Color ToAzColor(const ScriptColor& value)
        {
            return AZ::Color(value.r, value.g, value.b, value.a);
        }

        AZ::Vector3 ToAzVector3Debug(const ScriptVec3& value)
        {
            return AZ::Vector3(value.x, value.y, value.z);
        }

        float GetExpireTime(float currentTime, float duration)
        {
            return currentTime + AZStd::max(duration, 0.0f);
        }

        void QueueLine(const AZ::Vector3& start, const AZ::Vector3& end, const AZ::Color& color, float duration = 0.0f)
        {
            DebugDrawState& state = GetDebugDrawState();
            AZStd::scoped_lock lock(state.m_mutex);
            state.m_lines.push_back(DebugLineCommand{ start, end, color, GetExpireTime(state.m_time, duration), duration < 0.0f });
        }

        void QueueRect(const AZ::Vector3& center, const AZ::Vector3& normal, float size, const AZ::Color& color, float duration = 0.0f)
        {
            DebugDrawState& state = GetDebugDrawState();
            AZStd::scoped_lock lock(state.m_mutex);
            state.m_rects.push_back(DebugRectCommand{
                center,
                normal,
                AZStd::max(size, 0.001f),
                color,
                GetExpireTime(state.m_time, duration),
                duration < 0.0f });
        }

        void DrawRect(AzFramework::DebugDisplayRequests& debugDisplay, const DebugRectCommand& rect)
        {
            AZ::Vector3 normal = rect.m_normal;
            if (normal.GetLengthSq() <= AZ::Constants::FloatEpsilon)
            {
                normal = AZ::Vector3::CreateAxisZ();
            }
            else
            {
                normal.Normalize();
            }

            AZ::Vector3 reference = AZStd::abs(normal.GetZ()) < 0.99f ? AZ::Vector3::CreateAxisZ() : AZ::Vector3::CreateAxisX();
            AZ::Vector3 tangent = normal.Cross(reference);
            if (tangent.GetLengthSq() <= AZ::Constants::FloatEpsilon)
            {
                tangent = AZ::Vector3::CreateAxisX();
            }
            tangent.Normalize();
            AZ::Vector3 bitangent = normal.Cross(tangent);
            bitangent.Normalize();

            const float halfSize = rect.m_size * 0.5f;
            const AZ::Vector3 p0 = rect.m_center - tangent * halfSize - bitangent * halfSize;
            const AZ::Vector3 p1 = rect.m_center + tangent * halfSize - bitangent * halfSize;
            const AZ::Vector3 p2 = rect.m_center + tangent * halfSize + bitangent * halfSize;
            const AZ::Vector3 p3 = rect.m_center - tangent * halfSize + bitangent * halfSize;

            debugDisplay.DrawTriangles(
                AZStd::vector<AZ::Vector3>{
                    p0, p1, p2,
                    p0, p2, p3,
                    p2, p1, p0,
                    p3, p2, p0 },
                rect.m_color);
        }

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        const std::string* GetStringArgument(asIScriptGeneric* generic)
        {
            return reinterpret_cast<const std::string*>(generic->GetArgObject(0));
        }

        void DebugLog(asIScriptGeneric* generic)
        {
            if (const std::string* message = GetStringArgument(generic))
            {
                AZ_TracePrintf(DebugBindingLogWindow, "%s\n", message->c_str());
            }
        }

        void DebugWarning(asIScriptGeneric* generic)
        {
            if (const std::string* message = GetStringArgument(generic))
            {
                AZ_Warning(DebugBindingLogWindow, false, "%s", message->c_str());
            }
        }

        void DebugError(asIScriptGeneric* generic)
        {
            if (const std::string* message = GetStringArgument(generic))
            {
                AZ_Error(DebugBindingLogWindow, false, "%s", message->c_str());
            }
        }

        void DebugDrawLine(asIScriptGeneric* generic)
        {
            const auto* start = reinterpret_cast<const ScriptVec3*>(generic->GetArgObject(0));
            const auto* end = reinterpret_cast<const ScriptVec3*>(generic->GetArgObject(1));
            const auto* color = reinterpret_cast<const ScriptColor*>(generic->GetArgObject(2));
            if (!start || !end || !color)
            {
                return;
            }

            QueueLine(ToAzVector3Debug(*start), ToAzVector3Debug(*end), ToAzColor(*color));
        }

        void DebugDrawRay(asIScriptGeneric* generic)
        {
            const auto* ray = reinterpret_cast<const ScriptRay*>(generic->GetArgObject(0));
            const auto* color = reinterpret_cast<const ScriptColor*>(generic->GetArgObject(1));
            if (!ray || !color)
            {
                return;
            }

            const AZ::Vector3 origin = ToAzVector3Debug(ray->origin);
            const AZ::Vector3 direction = ToAzVector3Debug(ray->direction);
            if (direction.GetLengthSq() <= AZ::Constants::FloatEpsilon || ray->distance <= 0.0f)
            {
                return;
            }

            QueueLine(origin, origin + direction.GetNormalized() * ray->distance, ToAzColor(*color));
        }

        void DebugDrawHitRect(asIScriptGeneric* generic)
        {
            const auto* hit = reinterpret_cast<const ScriptRayHit*>(generic->GetArgObject(0));
            const float size = generic->GetArgFloat(1);
            const auto* color = reinterpret_cast<const ScriptColor*>(generic->GetArgObject(2));
            if (!hit || !hit->hit || !color)
            {
                return;
            }

            QueueRect(ToAzVector3Debug(hit->position), ToAzVector3Debug(hit->normal), size, ToAzColor(*color));
        }

        void DebugDrawRaycast(asIScriptGeneric* generic)
        {
            const auto* ray = reinterpret_cast<const ScriptRay*>(generic->GetArgObject(0));
            const auto* hit = reinterpret_cast<const ScriptRayHit*>(generic->GetArgObject(1));
            const auto* lineColor = reinterpret_cast<const ScriptColor*>(generic->GetArgObject(2));
            const auto* hitColor = reinterpret_cast<const ScriptColor*>(generic->GetArgObject(3));
            const float hitSize = generic->GetArgFloat(4);
            if (!ray || !lineColor)
            {
                return;
            }

            const AZ::Vector3 origin = ToAzVector3Debug(ray->origin);
            const AZ::Vector3 direction = ToAzVector3Debug(ray->direction);
            if (direction.GetLengthSq() <= AZ::Constants::FloatEpsilon || ray->distance <= 0.0f)
            {
                return;
            }

            const AZ::Vector3 rayEnd = origin + direction.GetNormalized() * ray->distance;
            AZ::Vector3 lineEnd = rayEnd;
            if (hit && hit->hit)
            {
                const AZ::Vector3 hitPosition = ToAzVector3Debug(hit->position);
                const float hitDistance = (hitPosition - origin).GetLength();
                if (hitDistance > 0.001f)
                {
                    lineEnd = hitPosition;
                }
            }
            QueueLine(origin, lineEnd, ToAzColor(*lineColor));

            if (hit && hit->hit && hitColor)
            {
                QueueRect(ToAzVector3Debug(hit->position), ToAzVector3Debug(hit->normal), hitSize, ToAzColor(*hitColor));
            }
        }
#endif
    } // namespace

    void QueueScriptRayDebug(const ScriptRay& ray, const ScriptColor& color, float duration)
    {
        const AZ::Vector3 origin = ToAzVector3Debug(ray.origin);
        const AZ::Vector3 direction = ToAzVector3Debug(ray.direction);
        if (direction.GetLengthSq() <= AZ::Constants::FloatEpsilon || ray.distance <= 0.0f)
        {
            return;
        }

        QueueLine(origin, origin + direction.GetNormalized() * ray.distance, ToAzColor(color), duration);
    }

    void QueueScriptHitRectDebug(const ScriptRayHit& hit, float size, const ScriptColor& color, float duration)
    {
        if (!hit.hit)
        {
            return;
        }

        QueueRect(ToAzVector3Debug(hit.position), ToAzVector3Debug(hit.normal), size, ToAzColor(color), duration);
    }

    void QueueScriptRaycastDebug(
        const ScriptRay& ray,
        const ScriptRayHit& hit,
        const ScriptColor& lineColor,
        const ScriptColor& hitColor,
        float hitSize,
        bool drawOnlyToHit,
        float duration)
    {
        const AZ::Vector3 origin = ToAzVector3Debug(ray.origin);
        const AZ::Vector3 direction = ToAzVector3Debug(ray.direction);
        if (direction.GetLengthSq() <= AZ::Constants::FloatEpsilon || ray.distance <= 0.0f)
        {
            return;
        }

        const AZ::Vector3 rayEnd = origin + direction.GetNormalized() * ray.distance;
        AZ::Vector3 lineEnd = rayEnd;
        if (drawOnlyToHit && hit.hit)
        {
            const AZ::Vector3 hitPosition = ToAzVector3Debug(hit.position);
            const float hitDistance = (hitPosition - origin).GetLength();
            if (hitDistance > 0.001f)
            {
                lineEnd = hitPosition;
            }
        }

        QueueLine(origin, lineEnd, ToAzColor(lineColor), duration);
        QueueScriptHitRectDebug(hit, hitSize, hitColor, duration);
    }

    void AdvanceDebugDrawFrame(float deltaTime)
    {
        DebugDrawState& state = GetDebugDrawState();
        AZStd::scoped_lock lock(state.m_mutex);
        state.m_time += AZStd::max(deltaTime, 0.0f);

        auto linePredicate = [time = state.m_time](const DebugLineCommand& command)
        {
            return !command.m_persistent && command.m_expireTime < time;
        };
        state.m_lines.erase(AZStd::remove_if(state.m_lines.begin(), state.m_lines.end(), linePredicate), state.m_lines.end());

        auto rectPredicate = [time = state.m_time](const DebugRectCommand& command)
        {
            return !command.m_persistent && command.m_expireTime < time;
        };
        state.m_rects.erase(AZStd::remove_if(state.m_rects.begin(), state.m_rects.end(), rectPredicate), state.m_rects.end());
    }

    void FlushQueuedDebugDraw([[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DebugDrawState& state = GetDebugDrawState();
        AZStd::vector<DebugLineCommand> lines;
        AZStd::vector<DebugRectCommand> rects;
        {
            AZStd::scoped_lock lock(state.m_mutex);
            lines = state.m_lines;
            rects = state.m_rects;
        }

        for (const DebugLineCommand& line : lines)
        {
            debugDisplay.SetDrawInFrontMode(true);
            debugDisplay.SetLineWidth(3.0f);
            debugDisplay.SetColor(line.m_color);
            debugDisplay.DrawLine(line.m_start, line.m_end);
        }

        for (const DebugRectCommand& rect : rects)
        {
            debugDisplay.SetDrawInFrontMode(true);
            debugDisplay.SetLineWidth(3.0f);
            DrawRect(debugDisplay, rect);
        }
    }

    bool RegisterDebug(asIScriptEngine* engine)
    {
#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        int result = engine->SetDefaultNamespace("Debug");
        if (result < 0)
        {
            AZ_Error(DebugBindingLogWindow, false, "Failed to set AngelScript Debug namespace. Result: %d", result);
            return false;
        }

        result = engine->RegisterGlobalFunction("void Log(const string &in)", asFUNCTION(DebugLog), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(DebugBindingLogWindow, false, "Failed to register AngelScript Debug::Log. Result: %d", result);
            return false;
        }

        result = engine->RegisterGlobalFunction("void Warning(const string &in)", asFUNCTION(DebugWarning), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(DebugBindingLogWindow, false, "Failed to register AngelScript Debug::Warning. Result: %d", result);
            return false;
        }

        result = engine->RegisterGlobalFunction("void Error(const string &in)", asFUNCTION(DebugError), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(DebugBindingLogWindow, false, "Failed to register AngelScript Debug::Error. Result: %d", result);
            return false;
        }

        result = engine->RegisterGlobalFunction("void DrawLine(const Vec3 &in, const Vec3 &in, const Color &in)", asFUNCTION(DebugDrawLine), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(DebugBindingLogWindow, false, "Failed to register AngelScript Debug::DrawLine. Result: %d", result);
            return false;
        }

        result = engine->RegisterGlobalFunction("void DrawRay(const Ray &in, const Color &in)", asFUNCTION(DebugDrawRay), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(DebugBindingLogWindow, false, "Failed to register AngelScript Debug::DrawRay. Result: %d", result);
            return false;
        }

        result = engine->RegisterGlobalFunction("void DrawHitRect(const RayHit &in, float, const Color &in)", asFUNCTION(DebugDrawHitRect), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(DebugBindingLogWindow, false, "Failed to register AngelScript Debug::DrawHitRect. Result: %d", result);
            return false;
        }

        result = engine->RegisterGlobalFunction("void DrawHitRect(const TraceHit &in, float, const Color &in)", asFUNCTION(DebugDrawHitRect), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(DebugBindingLogWindow, false, "Failed to register AngelScript Debug::DrawHitRect TraceHit overload. Result: %d", result);
            return false;
        }

        result = engine->RegisterGlobalFunction("void DrawRaycast(const Ray &in, const RayHit &in, const Color &in, const Color &in, float = 0.1f)", asFUNCTION(DebugDrawRaycast), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(DebugBindingLogWindow, false, "Failed to register AngelScript Debug::DrawRaycast. Result: %d", result);
            return false;
        }

        result = engine->RegisterGlobalFunction("void DrawRaycast(const Ray &in, const TraceHit &in, const Color &in, const Color &in, float = 0.1f)", asFUNCTION(DebugDrawRaycast), asCALL_GENERIC);
        if (result < 0)
        {
            AZ_Error(DebugBindingLogWindow, false, "Failed to register AngelScript Debug::DrawRaycast TraceHit overload. Result: %d", result);
            return false;
        }

        result = engine->SetDefaultNamespace("");
        if (result < 0)
        {
            AZ_Error(DebugBindingLogWindow, false, "Failed to reset AngelScript namespace. Result: %d", result);
            return false;
        }

        return true;
#else
        (void)engine;
        return false;
#endif
    }
} // namespace AngelScript
