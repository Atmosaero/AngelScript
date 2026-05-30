#include <Bindings/Gameplay.h>

#include <Bindings/Debug.h>
#include <Bindings/BehaviorContextBus.h>
#include <Bindings/Entity.h>
#include <Bindings/EntityBus.h>
#include <Bindings/Math.h>
#include <Bindings/Physics.h>

#include <AzCore/Debug/Trace.h>

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
#include <angelscript.h>
#include <scriptarray.h>
#include <scriptstdstring.h>
#endif

namespace AngelScript
{
    namespace
    {
        constexpr const char* GameplayBindingLogWindow = "AngelScript";
    }

    bool RegisterGameplay(asIScriptEngine* engine)
    {
#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        if (!engine)
        {
            AZ_Error(GameplayBindingLogWindow, false, "Cannot register AngelScript gameplay bindings without an engine.");
            return false;
        }

        RegisterStdString(engine);
        RegisterScriptArray(engine, false);

        return RegisterMath(engine) &&
            RegisterEntity(engine) &&
            RegisterEntityBus(engine) &&
            RegisterBehaviorContextBuses(engine) &&
            RegisterPhysics(engine) &&
            RegisterDebug(engine);
#else
        (void)engine;
        return false;
#endif
    }
} // namespace AngelScript
