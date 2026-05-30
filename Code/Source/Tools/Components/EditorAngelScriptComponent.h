#pragma once

#include <AngelScript/AngelScriptTypeIds.h>
#include <Components/AngelScriptComponent.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace AngelScript
{
    class EditorAngelScriptComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AZ::Data::AssetBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorAngelScriptComponent, EditorAngelScriptComponentTypeId);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        void Init() override;
        void Activate() override;
        void Deactivate() override;
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        static AZStd::string GetVariablesEditorTooltip();
        AZ::u32 OnScriptAssetChanged();
        AZ::u32 OnEntityVariablesChanged();
        AZStd::string GetEntityVariableEditorLabel(size_t index) const;
        void RefreshVariablesFromScriptAsset();
        void SynchronizeEntityVariables(const AZStd::vector<ScriptEntityVariable>& scriptEntityVariables);
        bool QueueScriptAssetLoad();

        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        ScriptDataAsset m_scriptAsset;
        AZStd::vector<ScriptEntityVariable> m_entityVariables;
    };
} // namespace AngelScript
