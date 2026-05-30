
#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

#include <Clients/AngelScriptSystemComponent.h>

namespace AngelScript
{
    /// System component for AngelScript editor
    class AngelScriptEditorSystemComponent
        : public AngelScriptSystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
        , private AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        , private AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Handler
    {
        using BaseSystemComponent = AngelScriptSystemComponent;
    public:
        AZ_COMPONENT_DECL(AngelScriptEditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        AngelScriptEditorSystemComponent();
        ~AngelScriptEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus
        void AddSourceFileCreators(
            const char* fullSourceFolderName,
            const AZ::Uuid& sourceUUID,
            AzToolsFramework::AssetBrowser::SourceFileCreatorList& creators) override;

        // AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus
        void HandleInitialFilenameChange(AZStd::string_view fullFilepath) override;

        static AZStd::string GenerateScriptBoilerplate(AZStd::string_view className);

        AZStd::string MakeUniqueScriptFilename(AZStd::string_view directoryPath) const;
        AZ::Outcome<void, AZStd::string> SaveScriptFile(AZStd::string_view fullFilepath, AZStd::string_view fileContents) const;

        static constexpr char ScriptExtension[] = ".as";
        static constexpr AZ::Crc32 ScriptCreationBusId = AZ::Crc32("AngelScriptFileCreationHandler");
    };
} // namespace AngelScript
