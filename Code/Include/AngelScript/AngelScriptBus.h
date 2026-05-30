
#pragma once

#include <AngelScript/AngelScriptTypeIds.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/string/string.h>

namespace AngelScript
{
    using ScriptInstanceId = AZ::u64;
    inline constexpr ScriptInstanceId InvalidScriptInstanceId = 0;

    struct ScriptCompileResult
    {
        bool m_success = false;
        AZStd::string m_error;
    };

    struct ScriptInstanceResult
    {
        bool m_success = false;
        AZStd::string m_error;
        ScriptInstanceId m_instanceId = InvalidScriptInstanceId;
        bool m_hasSetEntity = false;
        bool m_hasOnActivate = false;
        bool m_hasOnDeactivate = false;
        bool m_hasOnTick = false;
    };

    class AngelScriptRequests
    {
    public:
        AZ_RTTI(AngelScriptRequests, AngelScriptRequestsTypeId);
        virtual ~AngelScriptRequests() = default;

        virtual bool IsRuntimeStarted() const = 0;
        virtual bool IsRuntimeSdkAvailable() const = 0;
        virtual ScriptCompileResult CompileScriptString(const char* moduleName, const char* source) = 0;
        virtual ScriptCompileResult CompileScriptFile(const char* moduleName, const char* scriptPath) = 0;
        virtual ScriptInstanceResult CreateScriptInstance(const char* moduleName, const char* className) = 0;
        virtual ScriptCompileResult SetScriptInstanceEntityProperty(
            ScriptInstanceId instanceId,
            const char* propertyName,
            AZ::EntityId entityId) = 0;
        virtual ScriptCompileResult ExecuteScriptInstanceSetEntity(ScriptInstanceId instanceId, AZ::EntityId entityId) = 0;
        virtual ScriptCompileResult ExecuteScriptInstanceOnActivate(ScriptInstanceId instanceId) = 0;
        virtual ScriptCompileResult ExecuteScriptInstanceOnDeactivate(ScriptInstanceId instanceId) = 0;
        virtual ScriptCompileResult ExecuteScriptInstanceOnTick(ScriptInstanceId instanceId, float deltaTime) = 0;
        virtual void DestroyScriptInstance(ScriptInstanceId instanceId) = 0;
    };

    class AngelScriptBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using AngelScriptRequestBus = AZ::EBus<AngelScriptRequests, AngelScriptBusTraits>;
    using AngelScriptInterface = AZ::Interface<AngelScriptRequests>;

} // namespace AngelScript
