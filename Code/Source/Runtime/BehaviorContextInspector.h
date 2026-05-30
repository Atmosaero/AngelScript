#pragma once

#include <AzCore/std/string/string.h>

namespace AZ
{
    class BehaviorContext;
    class BehaviorMethod;
    struct BehaviorParameter;
} // namespace AZ

namespace AngelScript
{
    struct BehaviorContextInspectionStats
    {
        size_t m_classCount = 0;
        size_t m_globalMethodCount = 0;
        size_t m_ebusCount = 0;
        size_t m_ebusEventCount = 0;
        size_t m_supportedEbusEventCount = 0;
    };

    class BehaviorContextInspector
    {
    public:
        BehaviorContextInspectionStats InspectAndLog(AZ::BehaviorContext& behaviorContext) const;

    private:
        AZStd::string GetTypeName(AZ::BehaviorContext& behaviorContext, const AZ::BehaviorParameter& parameter) const;
        bool IsSupportedType(const AZ::BehaviorParameter& parameter) const;
        bool IsSupportedMethod(const AZ::BehaviorMethod& method) const;
        AZStd::string FormatMethodSignature(AZ::BehaviorContext& behaviorContext, const AZ::BehaviorMethod& method) const;
    };
} // namespace AngelScript
