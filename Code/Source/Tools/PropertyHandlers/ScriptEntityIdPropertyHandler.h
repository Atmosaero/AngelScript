#pragma once

#include <AzToolsFramework/UI/PropertyEditor/PropertyEntityIdCtrl.hxx>

namespace AngelScript
{
    class ScriptEntityIdPropertyHandler
        : public AzToolsFramework::EntityIdPropertyHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptEntityIdPropertyHandler, AZ::SystemAllocator);

        AZ::u32 GetHandlerName() const override;
        bool IsDefaultHandler() const override;
        bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;
        bool ReadValuesIntoGUI(
            size_t index,
            AzToolsFramework::PropertyEntityIdCtrl* GUI,
            const property_t& instance,
            AzToolsFramework::InstanceDataNode* node) override;

        static void Register();
        static void Unregister();

    private:
        static ScriptEntityIdPropertyHandler* s_instance;
    };
} // namespace AngelScript
