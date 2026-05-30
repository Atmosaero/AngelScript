#include "EditorAngelScriptComponent.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string_view.h>

namespace AngelScript
{
    namespace
    {
        constexpr const char* EditorComponentLogWindow = "AngelScript";

        bool IsIdentifierStart(char value)
        {
            return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') || value == '_';
        }

        bool IsIdentifierContinue(char value)
        {
            return IsIdentifierStart(value) || (value >= '0' && value <= '9');
        }

        bool IsDigit(char value)
        {
            return value >= '0' && value <= '9';
        }

        bool IsWhitespace(char value)
        {
            return value == ' ' || value == '\t' || value == '\r' || value == '\n' || value == '\f' || value == '\v';
        }

        void SkipWhitespace(const AZStd::string& source, size_t& index)
        {
            while (index < source.size() && IsWhitespace(source[index]))
            {
                ++index;
            }
        }

        bool IsWordBoundary(const AZStd::string& source, size_t index)
        {
            return index >= source.size() || !IsIdentifierContinue(source[index]);
        }

        bool StartsWithAt(const AZStd::string& source, size_t index, AZStd::string_view value)
        {
            return index + value.size() <= source.size() && source.compare(index, value.size(), value.data(), value.size()) == 0;
        }

        AZStd::string MakeStableVariableName(AZStd::string_view scriptName)
        {
            size_t size = scriptName.size();
            while (size > 0 && IsDigit(scriptName[size - 1]))
            {
                --size;
            }

            return AZStd::string(scriptName.substr(0, size));
        }

        size_t FindMetadataEnd(const AZStd::string& source, size_t openBracketIndex)
        {
            bool inString = false;
            char stringDelimiter = '\0';
            for (size_t index = openBracketIndex + 1; index < source.size(); ++index)
            {
                const char value = source[index];
                if (inString)
                {
                    if (value == '\\')
                    {
                        ++index;
                    }
                    else if (value == stringDelimiter)
                    {
                        inString = false;
                    }
                    continue;
                }

                if (value == '"' || value == '\'')
                {
                    inString = true;
                    stringDelimiter = value;
                    continue;
                }

                if (value == ']')
                {
                    return index;
                }
            }

            return AZStd::string::npos;
        }

        AZStd::string ParseMetadataStringAttribute(const AZStd::string& metadata, AZStd::string_view attributeName)
        {
            size_t index = metadata.find(attributeName);
            while (index != AZStd::string::npos)
            {
                const bool hasPrefixBoundary = index == 0 || !IsIdentifierContinue(metadata[index - 1]);
                const size_t valueStart = index + attributeName.size();
                const bool hasSuffixBoundary = valueStart >= metadata.size() || !IsIdentifierContinue(metadata[valueStart]);
                if (hasPrefixBoundary && hasSuffixBoundary)
                {
                    index = valueStart;
                    SkipWhitespace(metadata, index);
                    if (index < metadata.size() && metadata[index] == '=')
                    {
                        ++index;
                        SkipWhitespace(metadata, index);
                        if (index < metadata.size() && metadata[index] == '"')
                        {
                            ++index;
                            AZStd::string value;
                            while (index < metadata.size() && metadata[index] != '"')
                            {
                                if (metadata[index] == '\\' && index + 1 < metadata.size())
                                {
                                    ++index;
                                }
                                value.push_back(metadata[index]);
                                ++index;
                            }
                            return value;
                        }
                    }
                }

                index = metadata.find(attributeName, index + 1);
            }

            return {};
        }

        bool ParseVisibleEntityVariable(
            const AZStd::string& source,
            size_t metadataEndIndex,
            const AZStd::string& metadata,
            ScriptEntityVariable& variable)
        {
            size_t index = metadataEndIndex + 1;
            SkipWhitespace(source, index);
            if (!StartsWithAt(source, index, "Entity") || !IsWordBoundary(source, index + 6))
            {
                return false;
            }

            index += 6;
            SkipWhitespace(source, index);
            if (index >= source.size() || !IsIdentifierStart(source[index]))
            {
                return false;
            }

            const size_t nameStart = index;
            while (index < source.size() && IsIdentifierContinue(source[index]))
            {
                ++index;
            }

            variable.m_scriptName = source.substr(nameStart, index - nameStart);
            variable.m_tooltip = ParseMetadataStringAttribute(metadata, "Tooltip");
            return true;
        }

        AZStd::vector<ScriptEntityVariable> FindVisibleEntityVariables(const AZStd::string& source)
        {
            AZStd::vector<ScriptEntityVariable> variables;
            size_t index = 0;
            while ((index = source.find("[Visible", index)) != AZStd::string::npos)
            {
                if (index + 8 < source.size() && IsIdentifierContinue(source[index + 8]))
                {
                    ++index;
                    continue;
                }

                const size_t metadataEndIndex = FindMetadataEnd(source, index);
                if (metadataEndIndex == AZStd::string::npos)
                {
                    return variables;
                }

                const AZStd::string metadata = source.substr(index + 1, metadataEndIndex - index - 1);
                ScriptEntityVariable variable;
                if (ParseVisibleEntityVariable(source, metadataEndIndex, metadata, variable))
                {
                    variables.push_back(AZStd::move(variable));
                }

                index = metadataEndIndex + 1;
            }

            return variables;
        }
    } // namespace

    void EditorAngelScriptComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorAngelScriptComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("ScriptAsset", &EditorAngelScriptComponent::m_scriptAsset)
                ->Field("EntityVariables", &EditorAngelScriptComponent::m_entityVariables)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorAngelScriptComponent>("AngelScript", "Runs a typed AngelScript gameplay component.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<ScriptAsset>::Uuid())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAngelScriptComponent::m_scriptAsset, "Script", "Processed .as AngelScript asset.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAngelScriptComponent::OnScriptAssetChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAngelScriptComponent::m_entityVariables, "Variables", "Script-visible variables declared with [Visible].")
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                    ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &EditorAngelScriptComponent::GetVariablesEditorTooltip)
                    ->Attribute(AZ::Edit::Attributes::IndexedChildNameLabelOverride, &EditorAngelScriptComponent::GetEntityVariableEditorLabel)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAngelScriptComponent::OnEntityVariablesChanged)
                    ;
            }
        }
    }

    void EditorAngelScriptComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AngelScriptComponentService"));
    }

    void EditorAngelScriptComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AngelScriptComponentService"));
    }

    void EditorAngelScriptComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void EditorAngelScriptComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void EditorAngelScriptComponent::Init()
    {
    }

    void EditorAngelScriptComponent::Activate()
    {
        QueueScriptAssetLoad();
    }

    void EditorAngelScriptComponent::Deactivate()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    void EditorAngelScriptComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto* runtimeComponent = gameEntity->CreateComponent<AngelScriptComponent>();
        runtimeComponent->SetScriptAsset(m_scriptAsset);
        runtimeComponent->SetEntityVariables(m_entityVariables);
    }

    AZStd::string EditorAngelScriptComponent::GetVariablesEditorTooltip()
    {
        return "Script-visible variables declared with [Visible].";
    }

    AZ::u32 EditorAngelScriptComponent::OnScriptAssetChanged()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        if (!m_scriptAsset.GetId().IsValid())
        {
            m_entityVariables.clear();
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        QueueScriptAssetLoad();
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::u32 EditorAngelScriptComponent::OnEntityVariablesChanged()
    {
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    AZStd::string EditorAngelScriptComponent::GetEntityVariableEditorLabel(size_t index) const
    {
        if (index >= m_entityVariables.size())
        {
            return {};
        }

        return m_entityVariables[index].GetEditorLabel();
    }

    void EditorAngelScriptComponent::RefreshVariablesFromScriptAsset()
    {
        const ScriptAsset* script = m_scriptAsset.Get();
        if (!script)
        {
            return;
        }

        SynchronizeEntityVariables(FindVisibleEntityVariables(script->m_source));
    }

    void EditorAngelScriptComponent::SynchronizeEntityVariables(const AZStd::vector<ScriptEntityVariable>& scriptEntityVariables)
    {
        AZStd::unordered_map<AZStd::string, AZ::EntityId> existingValues;
        AZStd::unordered_map<AZStd::string, AZ::EntityId> stableExistingValues;
        for (const ScriptEntityVariable& variable : m_entityVariables)
        {
            existingValues[variable.m_scriptName] = variable.m_entityId;
            stableExistingValues[MakeStableVariableName(variable.m_scriptName)] = variable.m_entityId;
        }

        m_entityVariables = scriptEntityVariables;
        for (ScriptEntityVariable& variable : m_entityVariables)
        {
            auto existingValue = existingValues.find(variable.m_scriptName);
            if (existingValue != existingValues.end())
            {
                variable.m_entityId = existingValue->second;
                continue;
            }

            auto stableExistingValue = stableExistingValues.find(MakeStableVariableName(variable.m_scriptName));
            if (stableExistingValue != stableExistingValues.end())
            {
                variable.m_entityId = stableExistingValue->second;
            }
        }
    }

    bool EditorAngelScriptComponent::QueueScriptAssetLoad()
    {
        const AZ::Data::AssetId scriptAssetId = m_scriptAsset.GetId();
        if (!scriptAssetId.IsValid())
        {
            return false;
        }

        AZ::Data::AssetBus::Handler::BusConnect(scriptAssetId);
        if (m_scriptAsset.IsReady())
        {
            RefreshVariablesFromScriptAsset();
        }
        else
        {
            m_scriptAsset.QueueLoad();
            m_scriptAsset.BlockUntilLoadComplete();
            if (m_scriptAsset.IsReady())
            {
                RefreshVariablesFromScriptAsset();
            }
        }

        return true;
    }

    void EditorAngelScriptComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_scriptAsset = asset;
        RefreshVariablesFromScriptAsset();
    }

    void EditorAngelScriptComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_scriptAsset = asset;
        RefreshVariablesFromScriptAsset();
    }

    void EditorAngelScriptComponent::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ_Warning(EditorComponentLogWindow, false, "Failed to load AngelScript asset '%s'.", asset.GetHint().c_str());
    }
} // namespace AngelScript
