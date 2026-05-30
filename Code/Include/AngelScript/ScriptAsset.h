#pragma once

#include <AngelScript/AngelScriptTypeIds.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Asset/GenericAssetHandler.h>

namespace AngelScript
{
    class ScriptAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptAsset, AZ::SystemAllocator);
        AZ_RTTI(ScriptAsset, AngelScriptAssetTypeId, AZ::Data::AssetData);

        static constexpr const char* GetDisplayName() { return "AngelScript"; }
        static constexpr const char* GetGroup() { return "Scripting"; }
        static constexpr const char* GetFileFilter() { return "as"; }

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_source;
    };

    using ScriptDataAsset = AZ::Data::Asset<ScriptAsset>;

    class ScriptAssetHandler
        : public AzFramework::GenericAssetHandler<ScriptAsset>
    {
    public:
        AZ_RTTI(ScriptAssetHandler, "{FB2AA650-AE18-4A7B-9731-841B2A9A879C}", AzFramework::GenericAssetHandler<ScriptAsset>);
        AZ_CLASS_ALLOCATOR(ScriptAssetHandler, AZ::SystemAllocator);

        ScriptAssetHandler()
            : AzFramework::GenericAssetHandler<ScriptAsset>(
                ScriptAsset::GetDisplayName(),
                ScriptAsset::GetGroup(),
                ScriptAsset::GetFileFilter(),
                AZ::AzTypeInfo<ScriptAsset>::Uuid(),
                nullptr,
                AZ::ObjectStream::ST_BINARY)
        {
        }
    };
} // namespace AngelScript
