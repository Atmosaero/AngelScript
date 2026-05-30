#pragma once

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

class asIScriptEngine;

namespace AngelScript
{
    struct ScriptColor;
    struct ScriptRay;
    struct ScriptRayHit;

    void AdvanceDebugDrawFrame(float deltaTime);
    void FlushQueuedDebugDraw(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay);
    void QueueScriptRayDebug(const ScriptRay& ray, const ScriptColor& color, float duration = 0.0f);
    void QueueScriptHitRectDebug(const ScriptRayHit& hit, float size, const ScriptColor& color, float duration = 0.0f);
    void QueueScriptRaycastDebug(
        const ScriptRay& ray,
        const ScriptRayHit& hit,
        const ScriptColor& lineColor,
        const ScriptColor& hitColor,
        float hitSize,
        bool drawOnlyToHit,
        float duration = 0.0f);

    bool RegisterDebug(asIScriptEngine* engine);
} // namespace AngelScript
