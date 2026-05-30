#pragma once

#include <AngelScript/AngelScriptBus.h>
#include <AngelScript/ScriptAsset.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AngelScript
{
    class ScriptEntityVariable
    {
    public:
        AZ_TYPE_INFO(ScriptEntityVariable, "{7A731335-4030-4B13-914E-D2825C63B9E9}");

        static void Reflect(AZ::ReflectContext* context);
        AZStd::string GetEditorLabel() const;
        AZStd::string GetEditorTooltip() const;
        AZ::u32 OnEntityIdChanged();

        AZStd::string m_scriptName;
        AZStd::string m_tooltip;
        AZ::EntityId m_entityId;
    };

    class AngelScriptComponent
        : public AZ::Component
        , private AZ::Data::AssetBus::Handler
        , public AZ::TickBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(AngelScriptComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static AZStd::string GetVariablesEditorTooltip();

        const ScriptDataAsset& GetScriptAsset() const;
        const AZStd::vector<ScriptEntityVariable>& GetEntityVariables() const;
        void SetScriptAsset(const ScriptDataAsset& scriptAsset);
        void SetEntityVariables(const AZStd::vector<ScriptEntityVariable>& entityVariables);

    protected:
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

    private:
        void CompileScriptAsset(const ScriptDataAsset& scriptAsset);
        void CreateAndActivateScriptInstance(const char* moduleName, const char* className);
        AZStd::string GetEntityVariableEditorLabel(size_t index) const;
        AZ::u32 OnScriptAssetChanged();
        AZ::u32 OnEntityVariablesChanged();
        void RefreshVariablesFromScriptAsset();
        void SynchronizeEntityVariables(const AZStd::vector<ScriptEntityVariable>& scriptEntityVariables);
        void ApplySerializedEntityVariables();
        void DestroyScriptInstance();
        bool QueueScriptAssetLoad();

        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        ScriptDataAsset m_scriptAsset;
        AZStd::vector<ScriptEntityVariable> m_entityVariables;
        AZStd::string m_activeScriptClass;
        ScriptInstanceId m_scriptInstanceId = InvalidScriptInstanceId;
        bool m_hasScriptOnDeactivate = false;
        bool m_hasScriptOnTick = false;
        bool m_isActive = false;
    };
} // namespace AngelScript
