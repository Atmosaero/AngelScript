#pragma once

#include "AngelScriptAssetBuilderWorker.h"

#include <AzCore/Component/Component.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace AngelScript
{
    class AngelScriptAssetBuilderComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(AngelScriptAssetBuilderComponent, "{CF574103-CA43-4071-A1DF-C2C264DE57CC}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        void Activate() override;
        void Deactivate() override;

    private:
        AngelScriptAssetBuilderWorker m_worker;
    };
} // namespace AngelScript
