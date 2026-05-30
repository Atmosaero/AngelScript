
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string_view.h>
#include "AngelScriptEditorSystemComponent.h"
#include "PropertyHandlers/ScriptEntityIdPropertyHandler.h"

#include <AngelScript/AngelScriptTypeIds.h>

#include <QIcon>

namespace AngelScript
{
    namespace
    {
        constexpr const char* LogName = "AngelScriptEditor";
        constexpr const char* DefaultScriptName = "NewAngelScript";

        bool IsAsciiLetter(char value)
        {
            return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z');
        }

        bool IsAsciiDigit(char value)
        {
            return value >= '0' && value <= '9';
        }

        bool IsValidIdentifierCharacter(char value)
        {
            return IsAsciiLetter(value) || IsAsciiDigit(value) || value == '_';
        }

        AZStd::string GetStemFromPath(AZStd::string_view fullFilepath)
        {
            const size_t slash = fullFilepath.find_last_of("/\\");
            const size_t nameStart = slash == AZStd::string_view::npos ? 0 : slash + 1;
            const size_t dot = fullFilepath.find_last_of('.');
            const size_t nameEnd = dot == AZStd::string_view::npos || dot < nameStart ? fullFilepath.size() : dot;

            return AZStd::string(fullFilepath.substr(nameStart, nameEnd - nameStart));
        }

        AZStd::string SanitizeClassName(AZStd::string_view rawName)
        {
            AZStd::string className;
            className.reserve(rawName.size());

            for (char value : rawName)
            {
                className.push_back(IsValidIdentifierCharacter(value) ? value : '_');
            }

            if (className.empty())
            {
                return DefaultScriptName;
            }

            if (!IsAsciiLetter(className.front()) && className.front() != '_')
            {
                className.insert(className.begin(), '_');
            }

            return className;
        }

        AZStd::string GetClassNameFromPath(AZStd::string_view fullFilepath)
        {
            return SanitizeClassName(GetStemFromPath(fullFilepath));
        }
    } // namespace

    AZ_COMPONENT_IMPL(AngelScriptEditorSystemComponent, "AngelScriptEditorSystemComponent",
        AngelScriptEditorSystemComponentTypeId, BaseSystemComponent);

    void AngelScriptEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AngelScriptEditorSystemComponent, AngelScriptSystemComponent>()
                ->Version(0);
        }
    }

    AngelScriptEditorSystemComponent::AngelScriptEditorSystemComponent() = default;

    AngelScriptEditorSystemComponent::~AngelScriptEditorSystemComponent() = default;

    void AngelScriptEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("AngelScriptEditorService"));
    }

    void AngelScriptEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("AngelScriptEditorService"));
    }

    void AngelScriptEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void AngelScriptEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void AngelScriptEditorSystemComponent::Activate()
    {
        AngelScriptSystemComponent::Activate();
        ScriptEntityIdPropertyHandler::Register();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Handler::BusConnect(ScriptCreationBusId);
    }

    void AngelScriptEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Handler::BusDisconnect(ScriptCreationBusId);
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        ScriptEntityIdPropertyHandler::Unregister();
        AngelScriptSystemComponent::Deactivate();
    }

    void AngelScriptEditorSystemComponent::AddSourceFileCreators(
        const char* fullSourceFolderName,
        [[maybe_unused]] const AZ::Uuid& sourceUUID,
        AzToolsFramework::AssetBrowser::SourceFileCreatorList& creators)
    {
        if (fullSourceFolderName == nullptr || fullSourceFolderName[0] == '\0')
        {
            return;
        }

        creators.emplace_back(
            "AngelScript.Script",
            "Angel Script",
            QIcon(),
            [this](const AZStd::string& fullSourceFolderName, [[maybe_unused]] const AZ::Uuid& sourceUUID)
            {
                const AZStd::string fullFilepath = MakeUniqueScriptFilename(fullSourceFolderName);
                const AZStd::string fileContents = GenerateScriptBoilerplate(GetClassNameFromPath(fullFilepath));
                const AZ::Outcome<void, AZStd::string> saveResult = SaveScriptFile(fullFilepath, fileContents);
                if (!saveResult.IsSuccess())
                {
                    AZ_Error(LogName, false, "Failed to create AngelScript file '%s': %s", fullFilepath.c_str(), saveResult.GetError().c_str());
                    return;
                }

                AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Event(
                    AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::FileCreationNotificationBusId,
                    &AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::HandleAssetCreatedInEditor,
                    AZStd::string_view(fullFilepath),
                    ScriptCreationBusId,
                    true);
            });
    }

    void AngelScriptEditorSystemComponent::HandleInitialFilenameChange(AZStd::string_view fullFilepath)
    {
        const AZStd::string fileContents = GenerateScriptBoilerplate(GetClassNameFromPath(fullFilepath));
        const AZ::Outcome<void, AZStd::string> saveResult = SaveScriptFile(fullFilepath, fileContents);
        AZ_Error(LogName, saveResult.IsSuccess(), "Failed to update AngelScript file '%.*s': %s",
            aznumeric_cast<int>(fullFilepath.size()),
            fullFilepath.data(),
            saveResult.IsSuccess() ? "" : saveResult.GetError().c_str());
    }

    AZStd::string AngelScriptEditorSystemComponent::GenerateScriptBoilerplate(AZStd::string_view className)
    {
        const AZStd::string sanitizedClassName = SanitizeClassName(className);
        return AZStd::string::format(
            "class %s\n"
            "{\n"
            "    void OnActivate()\n"
            "    {\n"
            "    }\n"
            "\n"
            "    void OnTick(float deltaTime)\n"
            "    {\n"
            "    }\n"
            "}\n",
            sanitizedClassName.c_str());
    }

    AZStd::string AngelScriptEditorSystemComponent::MakeUniqueScriptFilename(AZStd::string_view directoryPath) const
    {
        AZStd::string normalizedDirectory(directoryPath);
        if (!normalizedDirectory.empty() && normalizedDirectory.back() != '/' && normalizedDirectory.back() != '\\')
        {
            normalizedDirectory.push_back('/');
        }

        AZStd::string fullFilepath = normalizedDirectory + DefaultScriptName + ScriptExtension;
        for (int suffix = 1; AZ::IO::SystemFile::Exists(fullFilepath.c_str()); ++suffix)
        {
            fullFilepath = AZStd::string::format("%s%s%d%s", normalizedDirectory.c_str(), DefaultScriptName, suffix, ScriptExtension);
        }

        return fullFilepath;
    }

    AZ::Outcome<void, AZStd::string> AngelScriptEditorSystemComponent::SaveScriptFile(
        AZStd::string_view fullFilepath,
        AZStd::string_view fileContents) const
    {
        AZ::IO::SystemFile file;
        const AZStd::string filepath(fullFilepath);
        if (!file.Open(filepath.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            return AZ::Failure(AZStd::string::format("Could not open file for writing."));
        }

        const AZ::IO::SystemFile::SizeType bytesWritten = file.Write(fileContents.data(), fileContents.size());
        if (bytesWritten != fileContents.size())
        {
            return AZ::Failure(AZStd::string::format("Expected to write %zu bytes, wrote %llu bytes.",
                fileContents.size(),
                aznumeric_cast<unsigned long long>(bytesWritten)));
        }

        return AZ::Success();
    }

} // namespace AngelScript
