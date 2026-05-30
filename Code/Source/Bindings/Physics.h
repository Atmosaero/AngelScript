#pragma once

#include <Bindings\Entity.h>
#include <Bindings\Math.h>

class asIScriptEngine;

namespace AngelScript
{
    struct ScriptRay
    {
        ScriptVec3 origin;
        ScriptVec3 direction;
        float distance = 1000.0f;
    };

    struct ScriptRayHit
    {
        bool hit = false;
        ScriptEntity entity;
        ScriptVec3 position;
        ScriptVec3 normal;
        float distance = 0.0f;
    };

    struct ScriptTraceQuery
    {
        ScriptEntity ignoredEntity;
        int debugTrace = 0;
        float debugDuration = 0.0f;
    };

    bool RegisterPhysics(asIScriptEngine* engine);
} // namespace AngelScript
