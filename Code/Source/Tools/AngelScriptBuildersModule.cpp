#include <AngelScriptModuleInterface.h>

#include <AzCore/Memory/Memory.h>

#include "AngelScriptAssetBuilderComponent.h"

namespace AngelScript
{
    class AngelScriptBuildersModule
        : public AngelScriptModuleInterface
    {
    public:
        AZ_RTTI(AngelScriptBuildersModule, "{F7FBE79D-6BBF-4423-8C30-77E67C4CAD75}", AngelScriptModuleInterface);
        AZ_CLASS_ALLOCATOR(AngelScriptBuildersModule, AZ::SystemAllocator);

        AngelScriptBuildersModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                AngelScriptAssetBuilderComponent::CreateDescriptor(),
            });
        }
    };
} // namespace AngelScript

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Builders), AngelScript::AngelScriptBuildersModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AngelScript_Builders, AngelScript::AngelScriptBuildersModule)
#endif
