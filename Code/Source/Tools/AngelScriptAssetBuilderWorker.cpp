#include "AngelScriptAssetBuilderWorker.h"

#include <AngelScript/ScriptAsset.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AngelScript
{
    namespace
    {
        constexpr const char* JobKey = "AngelScript Source";
        constexpr const char* ScriptExtension = "as";
    }

    void AngelScriptAssetBuilderWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void AngelScriptAssetBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        AZStd::string extension;
        AZ::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), extension, false);
        if (!AZ::StringFunc::Equal(extension.c_str(), ScriptExtension))
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
            return;
        }

        for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor descriptor;
            descriptor.m_jobKey = JobKey;
            descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
            response.m_createJobOutputs.push_back(descriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void AngelScriptAssetBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;

        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
        if (jobCancelListener.IsCancelled() || m_isShuttingDown)
        {
            return;
        }

        AZStd::string extension;
        AZ::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), extension, false);
        if (!AZ::StringFunc::Equal(extension.c_str(), ScriptExtension) ||
            !AZ::StringFunc::Equal(request.m_jobDescription.m_jobKey.c_str(), JobKey))
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        ScriptAsset::Reflect(serializeContext);

        AZStd::string source;
        {
            AZ::IO::SystemFile file;
            if (!file.Open(request.m_fullPath.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            const AZ::IO::SystemFile::SizeType fileLength = file.Length();
            source.resize_no_construct(static_cast<size_t>(fileLength));
            if (fileLength > 0)
            {
                file.Read(fileLength, source.data());
            }
        }

        ScriptAsset scriptAsset;
        scriptAsset.m_source = AZStd::move(source);

        AZStd::string outputFullPath;
        AZ::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), request.m_sourceFile.c_str(), outputFullPath, false);

        AZStd::string outputDir;
        AZ::StringFunc::Path::GetFolderPath(outputFullPath.c_str(), outputDir);
        if (!outputDir.empty())
        {
            if (auto* fileIo = AZ::IO::FileIOBase::GetInstance())
            {
                fileIo->CreatePath(outputDir.c_str());
            }
        }

        if (!AZ::Utils::SaveObjectToFile(outputFullPath, AZ::DataStream::ST_BINARY, &scriptAsset, serializeContext))
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        AssetBuilderSDK::JobProduct jobProduct{ outputFullPath };
        jobProduct.m_productAssetType = AZ::AzTypeInfo<ScriptAsset>::Uuid();
        jobProduct.m_dependenciesHandled = true;
        jobProduct.m_productSubID = 0;
        response.m_outputProducts.push_back(jobProduct);

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }
} // namespace AngelScript
