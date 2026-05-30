
#include "AngelScriptSystemComponent.h"

#include <Bindings/Debug.h>
#include <AngelScript/ScriptAsset.h>
#include <AngelScript/AngelScriptTypeIds.h>
#include <Runtime/AngelScriptRuntime.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Entity/GameEntityContextBus.h>

namespace AngelScript
{
    AZ_COMPONENT_IMPL(AngelScriptSystemComponent, "AngelScriptSystemComponent",
        AngelScriptSystemComponentTypeId);

    void AngelScriptSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AngelScriptSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }

        ScriptAsset::Reflect(context);
    }

    void AngelScriptSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AngelScriptService"));
    }

    void AngelScriptSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AngelScriptService"));
    }

    void AngelScriptSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void AngelScriptSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    bool AngelScriptSystemComponent::IsRuntimeStarted() const
    {
        return m_runtime && m_runtime->IsStarted();
    }

    bool AngelScriptSystemComponent::IsRuntimeSdkAvailable() const
    {
        return m_runtime && m_runtime->IsSdkAvailable();
    }

    ScriptCompileResult AngelScriptSystemComponent::CompileScriptString(const char* moduleName, const char* source)
    {
        if (!m_runtime)
        {
            return { false, "AngelScript runtime is not initialized." };
        }

        return m_runtime->CompileScriptString(moduleName, source);
    }

    ScriptCompileResult AngelScriptSystemComponent::CompileScriptFile(const char* moduleName, const char* scriptPath)
    {
        if (!m_runtime)
        {
            return { false, "AngelScript runtime is not initialized." };
        }

        return m_runtime->CompileScriptFile(moduleName, scriptPath);
    }

    ScriptInstanceResult AngelScriptSystemComponent::CreateScriptInstance(const char* moduleName, const char* className)
    {
        if (!m_runtime)
        {
            return { false, "AngelScript runtime is not initialized." };
        }

        return m_runtime->CreateScriptInstance(moduleName, className);
    }

    ScriptCompileResult AngelScriptSystemComponent::SetScriptInstanceEntityProperty(
        ScriptInstanceId instanceId,
        const char* propertyName,
        AZ::EntityId entityId)
    {
        if (!m_runtime)
        {
            return { false, "AngelScript runtime is not initialized." };
        }

        return m_runtime->SetScriptInstanceEntityProperty(instanceId, propertyName, entityId);
    }

    ScriptCompileResult AngelScriptSystemComponent::ExecuteScriptInstanceSetEntity(ScriptInstanceId instanceId, AZ::EntityId entityId)
    {
        if (!m_runtime)
        {
            return { false, "AngelScript runtime is not initialized." };
        }

        return m_runtime->ExecuteScriptInstanceSetEntity(instanceId, entityId);
    }

    ScriptCompileResult AngelScriptSystemComponent::ExecuteScriptInstanceOnActivate(ScriptInstanceId instanceId)
    {
        if (!m_runtime)
        {
            return { false, "AngelScript runtime is not initialized." };
        }

        return m_runtime->ExecuteScriptInstanceOnActivate(instanceId);
    }

    ScriptCompileResult AngelScriptSystemComponent::ExecuteScriptInstanceOnDeactivate(ScriptInstanceId instanceId)
    {
        if (!m_runtime)
        {
            return { false, "AngelScript runtime is not initialized." };
        }

        return m_runtime->ExecuteScriptInstanceOnDeactivate(instanceId);
    }

    ScriptCompileResult AngelScriptSystemComponent::ExecuteScriptInstanceOnTick(ScriptInstanceId instanceId, float deltaTime)
    {
        if (!m_runtime)
        {
            return { false, "AngelScript runtime is not initialized." };
        }

        return m_runtime->ExecuteScriptInstanceOnTick(instanceId, deltaTime);
    }

    void AngelScriptSystemComponent::DestroyScriptInstance(ScriptInstanceId instanceId)
    {
        if (m_runtime)
        {
            m_runtime->DestroyScriptInstance(instanceId);
        }
    }

    AngelScriptSystemComponent::AngelScriptSystemComponent()
    {
        if (AngelScriptInterface::Get() == nullptr)
        {
            AngelScriptInterface::Register(this);
        }
    }

    AngelScriptSystemComponent::~AngelScriptSystemComponent()
    {
        if (AngelScriptInterface::Get() == this)
        {
            AngelScriptInterface::Unregister(this);
        }
    }

    void AngelScriptSystemComponent::Init()
    {
        m_runtime = AZStd::make_unique<AngelScriptRuntime>();
    }

    void AngelScriptSystemComponent::Activate()
    {
        AngelScriptRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
        TryConnectViewportDebugDisplay();

        if (m_runtime)
        {
            m_runtime->Start();
        }

        m_scriptAssetHandler = AZStd::make_unique<ScriptAssetHandler>();
        m_scriptAssetHandler->Register();

        if (auto assetCatalog = AZ::Data::AssetCatalogRequestBus::FindFirstHandler())
        {
            assetCatalog->EnableCatalogForAsset(AZ::AzTypeInfo<ScriptAsset>::Uuid());
            assetCatalog->AddExtension("as");
        }
    }

    void AngelScriptSystemComponent::Deactivate()
    {
        if (m_scriptAssetHandler)
        {
            m_scriptAssetHandler->Unregister();
            m_scriptAssetHandler.reset();
        }

        if (m_runtime)
        {
            m_runtime->Stop();
        }

        if (AzFramework::ViewportDebugDisplayEventBus::Handler::BusIsConnected())
        {
            AzFramework::ViewportDebugDisplayEventBus::Handler::BusDisconnect();
        }
        m_viewportDebugDisplayContextId = AzFramework::EntityContextId();

        AZ::TickBus::Handler::BusDisconnect();
        AngelScriptRequestBus::Handler::BusDisconnect();
    }

    void AngelScriptSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        TryConnectViewportDebugDisplay();
        AdvanceDebugDrawFrame(deltaTime);
    }

    void AngelScriptSystemComponent::DisplayViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        FlushQueuedDebugDraw(viewportInfo, debugDisplay);
    }

    void AngelScriptSystemComponent::TryConnectViewportDebugDisplay()
    {
        if (AzFramework::ViewportDebugDisplayEventBus::Handler::BusIsConnected())
        {
            return;
        }

        AzFramework::EntityContextId contextId;
        AzFramework::GameEntityContextRequestBus::BroadcastResult(
            contextId,
            &AzFramework::GameEntityContextRequests::GetGameEntityContextId);

        if (contextId.IsNull())
        {
            return;
        }

        m_viewportDebugDisplayContextId = contextId;
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusConnect(m_viewportDebugDisplayContextId);
    }

} // namespace AngelScript
