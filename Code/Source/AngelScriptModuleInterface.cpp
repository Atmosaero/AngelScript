
#include "AngelScriptModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <AngelScript/AngelScriptTypeIds.h>

#include <Clients/AngelScriptSystemComponent.h>
#include <Components/AngelScriptComponent.h>

namespace AngelScript
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(AngelScriptModuleInterface,
        "AngelScriptModuleInterface", AngelScriptModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(AngelScriptModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(AngelScriptModuleInterface, AZ::SystemAllocator);

    AngelScriptModuleInterface::AngelScriptModuleInterface()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        // Add ALL components descriptors associated with this gem to m_descriptors.
        // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
        // This happens through the [MyComponent]::Reflect() function.
        m_descriptors.insert(m_descriptors.end(), {
            AngelScriptComponent::CreateDescriptor(),
            AngelScriptSystemComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList AngelScriptModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<AngelScriptSystemComponent>(),
        };
    }
} // namespace AngelScript
