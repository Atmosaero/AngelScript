#include "AngelScriptAssetBuilderComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AngelScript
{
    void AngelScriptAssetBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AngelScriptAssetBuilderComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
                ;
        }
    }

    void AngelScriptAssetBuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AngelScriptAssetBuilderService"));
    }

    void AngelScriptAssetBuilderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AngelScriptAssetBuilderService"));
    }

    void AngelScriptAssetBuilderComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void AngelScriptAssetBuilderComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void AngelScriptAssetBuilderComponent::Activate()
    {
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "AngelScript Source Builder";
        builderDescriptor.m_version = 1;
        builderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.as", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_busId = azrtti_typeid<AngelScriptAssetBuilderWorker>();
        builderDescriptor.m_createJobFunction = AZStd::bind(&AngelScriptAssetBuilderWorker::CreateJobs, &m_worker, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&AngelScriptAssetBuilderWorker::ProcessJob, &m_worker, AZStd::placeholders::_1, AZStd::placeholders::_2);

        m_worker.BusConnect(builderDescriptor.m_busId);
        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDescriptor);
    }

    void AngelScriptAssetBuilderComponent::Deactivate()
    {
        m_worker.BusDisconnect();
    }
} // namespace AngelScript
