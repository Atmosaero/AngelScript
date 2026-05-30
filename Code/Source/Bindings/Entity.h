#pragma once

#include <AzCore/Component/EntityId.h>

class asIScriptEngine;

namespace AngelScript
{
    struct ScriptEntity
    {
        AZ::u64 m_entityId = AZ::EntityId::InvalidEntityId;

        bool IsValid() const;
    };

    bool RegisterEntity(asIScriptEngine* engine);
} // namespace AngelScript
