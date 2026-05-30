
#include <AngelScript/AngelScriptTypeIds.h>
#include <AngelScriptModuleInterface.h>
#include "AngelScriptEditorSystemComponent.h"
#include "Components/EditorAngelScriptComponent.h"

namespace AngelScript
{
    class AngelScriptEditorModule
        : public AngelScriptModuleInterface
    {
    public:
        AZ_RTTI(AngelScriptEditorModule, AngelScriptEditorModuleTypeId, AngelScriptModuleInterface);
        AZ_CLASS_ALLOCATOR(AngelScriptEditorModule, AZ::SystemAllocator);

        AngelScriptEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                EditorAngelScriptComponent::CreateDescriptor(),
                AngelScriptEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<AngelScriptEditorSystemComponent>(),
            };
        }
    };
}// namespace AngelScript

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), AngelScript::AngelScriptEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AngelScript_Editor, AngelScript::AngelScriptEditorModule)
#endif
