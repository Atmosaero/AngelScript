#pragma once

#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace AngelScript
{
    class AngelScriptAssetBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_RTTI(AngelScriptAssetBuilderWorker, "{F109AA3D-CEFF-4F00-8EE2-5D9D3C9E881E}");

        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);
        void ShutDown() override;

    private:
        bool m_isShuttingDown = false;
    };
} // namespace AngelScript
