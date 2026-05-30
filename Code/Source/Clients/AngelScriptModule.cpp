
#include <AngelScript/AngelScriptTypeIds.h>
#include <AngelScriptModuleInterface.h>
#include "AngelScriptSystemComponent.h"

namespace AngelScript
{
    class AngelScriptModule
        : public AngelScriptModuleInterface
    {
    public:
        AZ_RTTI(AngelScriptModule, AngelScriptModuleTypeId, AngelScriptModuleInterface);
        AZ_CLASS_ALLOCATOR(AngelScriptModule, AZ::SystemAllocator);
    };
}// namespace AngelScript

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AngelScript::AngelScriptModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AngelScript, AngelScript::AngelScriptModule)
#endif
