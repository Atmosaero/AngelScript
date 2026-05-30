#include <Components/AngelScriptComponent.h>

#include <AngelScript/AngelScriptBus.h>
#include <AngelScript/AngelScriptTypeIds.h>
#include <Bindings/Debug.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string_view.h>

namespace AngelScript
{
    namespace
    {
        constexpr const char* ComponentLogWindow = "AngelScript";

        bool IsIdentifierContinue(char value)
        {
            return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') || (value >= '0' && value <= '9') || value == '_';
        }

        bool IsIdentifierStart(char value)
        {
            return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') || value == '_';
        }

        bool IsWhitespace(char value)
        {
            return value == ' ' || value == '\t' || value == '\r' || value == '\n' || value == '\f' || value == '\v';
        }

        bool IsLower(char value)
        {
            return value >= 'a' && value <= 'z';
        }

        bool IsUpper(char value)
        {
            return value >= 'A' && value <= 'Z';
        }

        bool IsDigit(char value)
        {
            return value >= '0' && value <= '9';
        }

        bool IsAlpha(char value)
        {
            return IsLower(value) || IsUpper(value);
        }

        char ToUpperAscii(char value)
        {
            return IsLower(value) ? static_cast<char>(value - 'a' + 'A') : value;
        }

        char ToLowerAscii(char value)
        {
            return IsUpper(value) ? static_cast<char>(value - 'A' + 'a') : value;
        }

        AZStd::string MakeEditorLabelFromScriptName(AZStd::string_view scriptName)
        {
            AZStd::string label;
            bool nextStartsWord = true;

            for (size_t index = 0; index < scriptName.size(); ++index)
            {
                const char value = scriptName[index];
                if (value == '_' || value == '-')
                {
                    if (!label.empty() && label.back() != ' ')
                    {
                        label.push_back(' ');
                    }
                    nextStartsWord = true;
                    continue;
                }

                if (!label.empty() && label.back() != ' ')
                {
                    const char previous = scriptName[index - 1];
                    const char next = index + 1 < scriptName.size() ? scriptName[index + 1] : '\0';
                    const bool startsCamelWord = IsUpper(value) && (IsLower(previous) || IsDigit(previous) || (IsUpper(previous) && IsLower(next)));
                    const bool changesAlphaNumericGroup = (IsDigit(value) && IsAlpha(previous)) || (IsAlpha(value) && IsDigit(previous));
                    if (startsCamelWord || changesAlphaNumericGroup)
                    {
                        label.push_back(' ');
                        nextStartsWord = true;
                    }
                }

                label.push_back(nextStartsWord ? ToUpperAscii(value) : ToLowerAscii(value));
                nextStartsWord = false;
            }

            while (!label.empty() && label.back() == ' ')
            {
                label.pop_back();
            }

            return label.empty() ? AZStd::string(scriptName) : label;
        }

        bool IsWordBoundary(const AZStd::string& source, size_t index)
        {
            return index >= source.size() || !IsIdentifierContinue(source[index]);
        }

        void SkipWhitespace(const AZStd::string& source, size_t& index)
        {
            while (index < source.size() && IsWhitespace(source[index]))
            {
                ++index;
            }
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

        AZStd::string FindFirstScriptClassName(const AZStd::string& source)
        {
            constexpr AZStd::string_view ClassKeyword = "class";
            size_t index = 0;
            while (index < source.size())
            {
                if (source[index] == '/' && index + 1 < source.size() && source[index + 1] == '/')
                {
                    index = source.find('\n', index + 2);
                    if (index == AZStd::string::npos)
                    {
                        return {};
                    }
                    continue;
                }

                if (source[index] == '/' && index + 1 < source.size() && source[index + 1] == '*')
                {
                    index = source.find("*/", index + 2);
                    if (index == AZStd::string::npos)
                    {
                        return {};
                    }
                    index += 2;
                    continue;
                }

                const bool startsClassKeyword =
                    (index == 0 || !IsIdentifierContinue(source[index - 1])) &&
                    source.compare(index, ClassKeyword.size(), ClassKeyword.data(), ClassKeyword.size()) == 0 &&
                    IsWordBoundary(source, index + ClassKeyword.size());

                if (!startsClassKeyword)
                {
                    ++index;
                    continue;
                }

                index += ClassKeyword.size();
                while (index < source.size() && IsWhitespace(source[index]))
                {
                    ++index;
                }

                if (index >= source.size() || !IsIdentifierStart(source[index]))
                {
                    return {};
                }

                const size_t classNameStart = index;
                while (index < source.size() && IsIdentifierContinue(source[index]))
                {
                    ++index;
                }

                return source.substr(classNameStart, index - classNameStart);
            }

            return {};
        }
    }

    AZ_COMPONENT_IMPL(AngelScriptComponent, "AngelScriptComponent", AngelScriptComponentTypeId);

    AZStd::string ScriptEntityVariable::GetEditorLabel() const
    {
        return MakeEditorLabelFromScriptName(m_scriptName);
    }

    AZStd::string ScriptEntityVariable::GetEditorTooltip() const
    {
        return m_tooltip;
    }

    AZ::u32 ScriptEntityVariable::OnEntityIdChanged()
    {
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    void ScriptEntityVariable::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            if (serializeContext->FindClassData(AZ::AzTypeInfo<ScriptEntityVariable>::Uuid()) != nullptr)
            {
                return;
            }

            serializeContext->Class<ScriptEntityVariable>()
                ->Version(2)
                ->Field("ScriptName", &ScriptEntityVariable::m_scriptName)
                ->Field("Tooltip", &ScriptEntityVariable::m_tooltip)
                ->Field("Entity", &ScriptEntityVariable::m_entityId)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ScriptEntityVariable>("Entity Variable", "Script-visible Entity variable.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ConciseEditorStringRepresentation, &ScriptEntityVariable::GetEditorLabel)
                    ->DataElement(AZ_CRC_CE("AngelScriptEntityId"), &ScriptEntityVariable::m_entityId, "Entity", "")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &ScriptEntityVariable::GetEditorLabel)
                    ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &ScriptEntityVariable::GetEditorTooltip)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ScriptEntityVariable::OnEntityIdChanged)
                    ;
            }
        }
    }

    void AngelScriptComponent::Reflect(AZ::ReflectContext* context)
    {
        ScriptEntityVariable::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AngelScriptComponent, AZ::Component>()
                ->Version(5)
                ->Field("ScriptAsset", &AngelScriptComponent::m_scriptAsset)
                ->Field("EntityVariables", &AngelScriptComponent::m_entityVariables)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AngelScriptComponent>("AngelScript", "Runs a typed AngelScript gameplay component.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AddableByUser, false)
                    ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<ScriptAsset>::Uuid())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AngelScriptComponent::m_scriptAsset, "Script", "Processed .as AngelScript asset.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AngelScriptComponent::OnScriptAssetChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AngelScriptComponent::m_entityVariables, "Variables", "Script-visible variables declared with [Visible].")
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                    ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &AngelScriptComponent::GetVariablesEditorTooltip)
                    ->Attribute(AZ::Edit::Attributes::IndexedChildNameLabelOverride, &AngelScriptComponent::GetEntityVariableEditorLabel)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AngelScriptComponent::OnEntityVariablesChanged)
                    ;
            }
        }
    }

    void AngelScriptComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AngelScriptComponentService"));
    }

    void AngelScriptComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AngelScriptComponentService"));
    }

    void AngelScriptComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void AngelScriptComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    const ScriptDataAsset& AngelScriptComponent::GetScriptAsset() const
    {
        return m_scriptAsset;
    }

    const AZStd::vector<ScriptEntityVariable>& AngelScriptComponent::GetEntityVariables() const
    {
        return m_entityVariables;
    }

    void AngelScriptComponent::SetScriptAsset(const ScriptDataAsset& scriptAsset)
    {
        m_scriptAsset = scriptAsset;
    }

    void AngelScriptComponent::SetEntityVariables(const AZStd::vector<ScriptEntityVariable>& entityVariables)
    {
        m_entityVariables = entityVariables;
    }

    AZStd::string AngelScriptComponent::GetVariablesEditorTooltip()
    {
        return "Script-visible variables declared with [Visible].";
    }

    AZStd::string AngelScriptComponent::GetEntityVariableEditorLabel(size_t index) const
    {
        if (index >= m_entityVariables.size())
        {
            return {};
        }

        return m_entityVariables[index].GetEditorLabel();
    }

    AZ::u32 AngelScriptComponent::OnScriptAssetChanged()
    {
        DestroyScriptInstance();
        AZ::Data::AssetBus::Handler::BusDisconnect();

        if (!m_scriptAsset.GetId().IsValid())
        {
            m_entityVariables.clear();
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        AZ::Data::AssetBus::Handler::BusConnect(m_scriptAsset.GetId());
        if (!m_scriptAsset.IsReady())
        {
            m_scriptAsset.QueueLoad();
            m_scriptAsset.BlockUntilLoadComplete();
        }

        if (m_scriptAsset.IsReady())
        {
            RefreshVariablesFromScriptAsset();
            if (m_isActive)
            {
                CompileScriptAsset(m_scriptAsset);
            }
        }

        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::u32 AngelScriptComponent::OnEntityVariablesChanged()
    {
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    void AngelScriptComponent::RefreshVariablesFromScriptAsset()
    {
        const ScriptAsset* script = m_scriptAsset.Get();
        if (!script)
        {
            return;
        }

        SynchronizeEntityVariables(FindVisibleEntityVariables(script->m_source));
    }

    void AngelScriptComponent::SynchronizeEntityVariables(const AZStd::vector<ScriptEntityVariable>& scriptEntityVariables)
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

    void AngelScriptComponent::CompileScriptAsset(const ScriptDataAsset& scriptAsset)
    {
        DestroyScriptInstance();

        const ScriptAsset* script = scriptAsset.Get();
        if (!script)
        {
            AZ_Warning(ComponentLogWindow, false, "AngelScript asset is ready but has no script data.");
            return;
        }

        const AZStd::string className = FindFirstScriptClassName(script->m_source);
        if (className.empty())
        {
            AZ_Warning(ComponentLogWindow, false, "AngelScript asset must contain one script class.");
            return;
        }

        bool sdkAvailable = false;
        AngelScriptRequestBus::BroadcastResult(sdkAvailable, &AngelScriptRequests::IsRuntimeSdkAvailable);
        if (!sdkAvailable)
        {
            return;
        }

        const char* moduleName = className.c_str();
        ScriptCompileResult result;
        AngelScriptRequestBus::BroadcastResult(result, &AngelScriptRequests::CompileScriptString, moduleName, script->m_source.c_str());

        if (!result.m_success)
        {
            AZ_Warning(ComponentLogWindow, false, "Failed to compile AngelScript asset: %s", result.m_error.c_str());
            return;
        }

        CreateAndActivateScriptInstance(moduleName, className.c_str());
    }

    void AngelScriptComponent::CreateAndActivateScriptInstance(const char* moduleName, const char* className)
    {
        ScriptInstanceResult createResult;
        AngelScriptRequestBus::BroadcastResult(createResult, &AngelScriptRequests::CreateScriptInstance, moduleName, className);
        if (!createResult.m_success)
        {
            AZ_Warning(ComponentLogWindow, false, "Failed to create AngelScript instance '%s': %s", className, createResult.m_error.c_str());
            return;
        }

        m_activeScriptClass = className;
        m_scriptInstanceId = createResult.m_instanceId;
        m_hasScriptOnDeactivate = createResult.m_hasOnDeactivate;
        m_hasScriptOnTick = createResult.m_hasOnTick;

        if (createResult.m_hasSetEntity)
        {
            ScriptCompileResult entityResult;
            AngelScriptRequestBus::BroadcastResult(entityResult, &AngelScriptRequests::ExecuteScriptInstanceSetEntity, m_scriptInstanceId, GetEntityId());
            if (!entityResult.m_success)
            {
                AZ_Warning(ComponentLogWindow, false, "AngelScript SetEntity failed for '%s': %s", m_activeScriptClass.c_str(), entityResult.m_error.c_str());
            }
        }
        else
        {
            AZ_TracePrintf(ComponentLogWindow, "AngelScript class '%s' does not define void SetEntity(Entity).\n", m_activeScriptClass.c_str());
        }

        ApplySerializedEntityVariables();

        if (!createResult.m_hasOnActivate)
        {
            AZ_TracePrintf(ComponentLogWindow, "AngelScript class '%s' does not define void OnActivate().\n", m_activeScriptClass.c_str());
        }
        else
        {
            ScriptCompileResult activateResult;
            AngelScriptRequestBus::BroadcastResult(activateResult, &AngelScriptRequests::ExecuteScriptInstanceOnActivate, m_scriptInstanceId);
            if (!activateResult.m_success)
            {
                AZ_Warning(ComponentLogWindow, false, "AngelScript OnActivate failed for '%s': %s", m_activeScriptClass.c_str(), activateResult.m_error.c_str());
            }
        }

        if (m_hasScriptOnTick)
        {
            AZ::TickBus::Handler::BusConnect();
        }
        else
        {
            AZ::TickBus::Handler::BusDisconnect();
            AZ_TracePrintf(ComponentLogWindow, "AngelScript class '%s' does not define void OnTick(float).\n", m_activeScriptClass.c_str());
        }
    }

    void AngelScriptComponent::ApplySerializedEntityVariables()
    {
        for (const ScriptEntityVariable& variable : m_entityVariables)
        {
            if (variable.m_scriptName.empty() || !variable.m_entityId.IsValid())
            {
                continue;
            }

            ScriptCompileResult propertyResult;
            AngelScriptRequestBus::BroadcastResult(
                propertyResult,
                &AngelScriptRequests::SetScriptInstanceEntityProperty,
                m_scriptInstanceId,
                variable.m_scriptName.c_str(),
                variable.m_entityId);

            if (!propertyResult.m_success)
            {
                AZ_Warning(
                    ComponentLogWindow,
                    false,
                    "Failed to assign AngelScript Entity variable '%s' for '%s': %s",
                    variable.m_scriptName.c_str(),
                    m_activeScriptClass.c_str(),
                    propertyResult.m_error.c_str());
                continue;
            }

            AZ_TracePrintf(
                ComponentLogWindow,
                "Assigned AngelScript Entity variable '%s' for '%s'.\n",
                variable.m_scriptName.c_str(),
                m_activeScriptClass.c_str());
        }
    }

    void AngelScriptComponent::DestroyScriptInstance()
    {
        AZ::TickBus::Handler::BusDisconnect();

        if (m_scriptInstanceId == InvalidScriptInstanceId)
        {
            m_hasScriptOnDeactivate = false;
            m_hasScriptOnTick = false;
            return;
        }

        if (m_hasScriptOnDeactivate)
        {
            ScriptCompileResult deactivateResult;
            AngelScriptRequestBus::BroadcastResult(deactivateResult, &AngelScriptRequests::ExecuteScriptInstanceOnDeactivate, m_scriptInstanceId);
            if (!deactivateResult.m_success)
            {
                AZ_Warning(ComponentLogWindow, false, "AngelScript OnDeactivate failed for '%s': %s", m_activeScriptClass.c_str(), deactivateResult.m_error.c_str());
            }
        }

        AngelScriptRequestBus::Broadcast(&AngelScriptRequests::DestroyScriptInstance, m_scriptInstanceId);
        m_scriptInstanceId = InvalidScriptInstanceId;
        m_activeScriptClass.clear();
        m_hasScriptOnDeactivate = false;
        m_hasScriptOnTick = false;
    }

    bool AngelScriptComponent::QueueScriptAssetLoad()
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
            CompileScriptAsset(m_scriptAsset);
        }
        else
        {
            m_scriptAsset.QueueLoad();
        }

        return true;
    }

    void AngelScriptComponent::Init()
    {
        if (!m_scriptAsset.GetId().IsValid())
        {
            return;
        }

        if (!m_scriptAsset.IsReady())
        {
            m_scriptAsset.QueueLoad();
            m_scriptAsset.BlockUntilLoadComplete();
        }

        if (m_scriptAsset.IsReady())
        {
            RefreshVariablesFromScriptAsset();
        }
    }

    void AngelScriptComponent::Activate()
    {
        m_isActive = true;
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        if (!QueueScriptAssetLoad())
        {
            AZ_Warning(ComponentLogWindow, false, "AngelScriptComponent has no script asset assigned.");
        }
    }

    void AngelScriptComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        DestroyScriptInstance();
        AZ::Data::AssetBus::Handler::BusDisconnect();
        m_isActive = false;
    }

    void AngelScriptComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_scriptAsset = asset;
        RefreshVariablesFromScriptAsset();
        if (m_isActive)
        {
            CompileScriptAsset(m_scriptAsset);
        }
    }

    void AngelScriptComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_scriptAsset = asset;
        RefreshVariablesFromScriptAsset();
        if (m_isActive)
        {
            CompileScriptAsset(m_scriptAsset);
        }
    }

    void AngelScriptComponent::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ_Warning(ComponentLogWindow, false, "Failed to load AngelScript asset '%s'.", asset.GetHint().c_str());
    }

    void AngelScriptComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_scriptInstanceId == InvalidScriptInstanceId || !m_hasScriptOnTick)
        {
            AZ::TickBus::Handler::BusDisconnect();
            return;
        }

        ScriptCompileResult result;
        AngelScriptRequestBus::BroadcastResult(result, &AngelScriptRequests::ExecuteScriptInstanceOnTick, m_scriptInstanceId, deltaTime);
        if (!result.m_success)
        {
            AZ_Warning(ComponentLogWindow, false, "AngelScript OnTick failed for '%s': %s", m_activeScriptClass.c_str(), result.m_error.c_str());
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    void AngelScriptComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        FlushQueuedDebugDraw(viewportInfo, debugDisplay);
    }
} // namespace AngelScript
