
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AngelScript/AngelScriptBus.h>

namespace AngelScript
{
    class AngelScriptRuntime;
    class ScriptAssetHandler;

    class AngelScriptSystemComponent
        : public AZ::Component
        , protected AngelScriptRequestBus::Handler
        , public AZ::TickBus::Handler
        , private AzFramework::ViewportDebugDisplayEventBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(AngelScriptSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        AngelScriptSystemComponent();
        ~AngelScriptSystemComponent();

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AngelScriptRequestBus interface implementation
        bool IsRuntimeStarted() const override;
        bool IsRuntimeSdkAvailable() const override;
        ScriptCompileResult CompileScriptString(const char* moduleName, const char* source) override;
        ScriptCompileResult CompileScriptFile(const char* moduleName, const char* scriptPath) override;
        ScriptInstanceResult CreateScriptInstance(const char* moduleName, const char* className) override;
        ScriptCompileResult SetScriptInstanceEntityProperty(ScriptInstanceId instanceId, const char* propertyName, AZ::EntityId entityId) override;
        ScriptCompileResult ExecuteScriptInstanceSetEntity(ScriptInstanceId instanceId, AZ::EntityId entityId) override;
        ScriptCompileResult ExecuteScriptInstanceOnActivate(ScriptInstanceId instanceId) override;
        ScriptCompileResult ExecuteScriptInstanceOnDeactivate(ScriptInstanceId instanceId) override;
        ScriptCompileResult ExecuteScriptInstanceOnTick(ScriptInstanceId instanceId, float deltaTime) override;
        void DestroyScriptInstance(ScriptInstanceId instanceId) override;

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZTickBus interface implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzFramework::ViewportDebugDisplayEventBus
        void DisplayViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;
        ////////////////////////////////////////////////////////////////////////

        void TryConnectViewportDebugDisplay();

    private:
        AZStd::unique_ptr<AngelScriptRuntime> m_runtime;
        AZStd::unique_ptr<ScriptAssetHandler> m_scriptAssetHandler;
        AzFramework::EntityContextId m_viewportDebugDisplayContextId;
    };

} // namespace AngelScript
