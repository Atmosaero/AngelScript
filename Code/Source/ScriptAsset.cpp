#include <AngelScript/ScriptAsset.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AngelScript
{
    void ScriptAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ScriptAsset, AZ::Data::AssetData>()
                ->Version(1)
                ->Field("Source", &ScriptAsset::m_source)
                ;
        }
    }
} // namespace AngelScript
