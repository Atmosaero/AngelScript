#include "ScriptEntityIdPropertyHandler.h"

#include <Components/AngelScriptComponent.h>

#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <QString>

namespace AngelScript
{
    namespace
    {
        constexpr AZ::u32 ScriptEntityIdHandlerName = AZ_CRC_CE("AngelScriptEntityId");
        constexpr const char* TooltipPropertyName = "AngelScriptTooltip";

        const ScriptEntityVariable* FindOwningScriptEntityVariable(AzToolsFramework::InstanceDataNode* node, size_t index)
        {
            for (AzToolsFramework::InstanceDataNode* currentNode = node; currentNode != nullptr; currentNode = currentNode->GetParent())
            {
                const AZ::SerializeContext::ClassData* classData = currentNode->GetClassMetadata();
                if (classData == nullptr || classData->m_typeId != azrtti_typeid<ScriptEntityVariable>())
                {
                    continue;
                }

                if (currentNode->GetNumInstances() <= index)
                {
                    return nullptr;
                }

                AZ::SerializeContext* serializeContext = currentNode->GetSerializeContext();
                if (serializeContext == nullptr)
                {
                    return nullptr;
                }

                void* instance = currentNode->GetInstance(index);
                if (instance == nullptr)
                {
                    return nullptr;
                }

                return static_cast<const ScriptEntityVariable*>(
                    serializeContext->DownCast(instance, classData->m_typeId, azrtti_typeid<ScriptEntityVariable>()));
            }

            return nullptr;
        }
    } // namespace

    ScriptEntityIdPropertyHandler* ScriptEntityIdPropertyHandler::s_instance = nullptr;

    AZ::u32 ScriptEntityIdPropertyHandler::GetHandlerName() const
    {
        return ScriptEntityIdHandlerName;
    }

    bool ScriptEntityIdPropertyHandler::IsDefaultHandler() const
    {
        return false;
    }

    bool ScriptEntityIdPropertyHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        const QVariant tooltipValue = widget->property(TooltipPropertyName);
        if (tooltipValue.isValid())
        {
            toolTipString = tooltipValue.toString();
            return true;
        }

        return false;
    }

    bool ScriptEntityIdPropertyHandler::ReadValuesIntoGUI(
        size_t index,
        AzToolsFramework::PropertyEntityIdCtrl* GUI,
        const property_t& instance,
        AzToolsFramework::InstanceDataNode* node)
    {
        const bool result = AzToolsFramework::EntityIdPropertyHandler::ReadValuesIntoGUI(index, GUI, instance, node);

        AZStd::string tooltipText;
        if (const ScriptEntityVariable* variable = FindOwningScriptEntityVariable(node, index); variable != nullptr)
        {
            tooltipText = variable->m_tooltip;
        }

        const QString tooltip = QString::fromUtf8(tooltipText.c_str());
        GUI->setProperty(TooltipPropertyName, tooltip);
        GUI->setToolTip(tooltip);
        GUI->SetChildWidgetsProperty("toolTip", tooltip);
        return result;
    }

    void ScriptEntityIdPropertyHandler::Register()
    {
        if (s_instance == nullptr)
        {
            s_instance = aznew ScriptEntityIdPropertyHandler();
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, s_instance);
        }
    }

    void ScriptEntityIdPropertyHandler::Unregister()
    {
        if (s_instance != nullptr)
        {
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Events::UnregisterPropertyType, s_instance);
            delete s_instance;
            s_instance = nullptr;
        }
    }
} // namespace AngelScript
