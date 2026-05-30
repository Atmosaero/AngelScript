#pragma once

class asIScriptEngine;

namespace AngelScript
{
    struct ScriptQuat
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 1.0f;
    };

    struct ScriptVec2
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct ScriptVec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct ScriptColor
    {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
    };

    struct ScriptTransform
    {
        ScriptVec3 basisX{ 1.0f, 0.0f, 0.0f };
        ScriptVec3 basisY{ 0.0f, 1.0f, 0.0f };
        ScriptVec3 basisZ{ 0.0f, 0.0f, 1.0f };
        ScriptVec3 translation;
    };

    bool RegisterMath(asIScriptEngine* engine);
} // namespace AngelScript
