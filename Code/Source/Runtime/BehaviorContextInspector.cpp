#include <Runtime/BehaviorContextInspector.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string_view.h>

namespace AngelScript
{
    namespace
    {
        constexpr const char* LogWindow = "AngelScript";

        bool HasTrait(const AZ::BehaviorParameter& parameter, AZ::BehaviorParameter::Traits trait)
        {
            return (parameter.m_traits & trait) != 0;
        }

        bool IsReferenceOrPointer(const AZ::BehaviorParameter& parameter)
        {
            return HasTrait(parameter, AZ::BehaviorParameter::TR_REFERENCE) || HasTrait(parameter, AZ::BehaviorParameter::TR_POINTER);
        }

        bool IsSupportedStringParameter(const AZ::BehaviorParameter& parameter)
        {
            if (!HasTrait(parameter, AZ::BehaviorParameter::TR_STRING))
            {
                return false;
            }

            const AZ::TypeId typeId = parameter.m_typeId;
            if (typeId == azrtti_typeid<char>())
            {
                return HasTrait(parameter, AZ::BehaviorParameter::TR_POINTER) &&
                    HasTrait(parameter, AZ::BehaviorParameter::TR_CONST);
            }

            return typeId == azrtti_typeid<AZStd::string>() ||
                typeId == azrtti_typeid<AZStd::string_view>();
        }
    } // namespace

    BehaviorContextInspectionStats BehaviorContextInspector::InspectAndLog(AZ::BehaviorContext& behaviorContext) const
    {
        BehaviorContextInspectionStats stats;
        stats.m_classCount = behaviorContext.m_classes.size();
        stats.m_globalMethodCount = behaviorContext.m_methods.size();
        stats.m_ebusCount = behaviorContext.m_ebuses.size();

        AZ_TracePrintf(
            LogWindow,
            "BehaviorContextInspector: classes=%zu globalMethods=%zu ebuses=%zu\n",
            stats.m_classCount,
            stats.m_globalMethodCount,
            stats.m_ebusCount);

        for (const auto& ebusPair : behaviorContext.m_ebuses)
        {
            const AZ::BehaviorEBus* ebus = ebusPair.second;
            if (ebus == nullptr)
            {
                continue;
            }

            size_t supportedEventCount = 0;
            const AZStd::string idTypeName = ebus->m_idParam.m_typeId.IsNull()
                ? AZStd::string("none")
                : GetTypeName(behaviorContext, ebus->m_idParam);

            for (const auto& eventPair : ebus->m_events)
            {
                const AZ::BehaviorEBusEventSender& sender = eventPair.second;
                const AZ::BehaviorMethod* method = sender.m_event != nullptr ? sender.m_event : sender.m_broadcast;
                if (method == nullptr)
                {
                    continue;
                }

                ++stats.m_ebusEventCount;
                if (IsSupportedMethod(*method))
                {
                    ++supportedEventCount;
                    ++stats.m_supportedEbusEventCount;
                }
            }

            AZ_TracePrintf(
                LogWindow,
                "BehaviorContextInspector: EBus %s id=%s events=%zu supported=%zu\n",
                ebus->m_name.c_str(),
                idTypeName.c_str(),
                ebus->m_events.size(),
                supportedEventCount);

            for (const auto& eventPair : ebus->m_events)
            {
                const AZ::BehaviorEBusEventSender& sender = eventPair.second;
                const AZ::BehaviorMethod* method = sender.m_event != nullptr ? sender.m_event : sender.m_broadcast;
                if (method == nullptr)
                {
                    continue;
                }

                AZ_TracePrintf(
                    LogWindow,
                    "BehaviorContextInspector:   [%s] %s\n",
                    IsSupportedMethod(*method) ? "supported" : "blocked",
                    FormatMethodSignature(behaviorContext, *method).c_str());
            }
        }

        AZ_TracePrintf(
            LogWindow,
            "BehaviorContextInspector: ebusEvents=%zu supportedEbusEvents=%zu\n",
            stats.m_ebusEventCount,
            stats.m_supportedEbusEventCount);

        return stats;
    }

    AZStd::string BehaviorContextInspector::GetTypeName(AZ::BehaviorContext& behaviorContext, const AZ::BehaviorParameter& parameter) const
    {
        if (parameter.m_typeId.IsNull())
        {
            return "void";
        }

        if (const AZ::BehaviorClass* behaviorClass = behaviorContext.FindClassByTypeId(parameter.m_typeId); behaviorClass != nullptr)
        {
            return behaviorClass->m_name;
        }

        if (parameter.m_name != nullptr && parameter.m_name[0] != '\0')
        {
            return parameter.m_name;
        }

        return parameter.m_typeId.ToString<AZStd::string>();
    }

    bool BehaviorContextInspector::IsSupportedType(const AZ::BehaviorParameter& parameter) const
    {
        if (parameter.m_typeId.IsNull())
        {
            return true;
        }

        if (IsSupportedStringParameter(parameter))
        {
            return true;
        }

        const AZ::TypeId typeId = parameter.m_typeId;
        if (typeId == azrtti_typeid<bool>() ||
            typeId == azrtti_typeid<char>() ||
            typeId == azrtti_typeid<AZ::s8>() ||
            typeId == azrtti_typeid<short>() ||
            typeId == azrtti_typeid<int>() ||
            typeId == azrtti_typeid<long>() ||
            typeId == azrtti_typeid<AZ::s64>() ||
            typeId == azrtti_typeid<unsigned char>() ||
            typeId == azrtti_typeid<unsigned short>() ||
            typeId == azrtti_typeid<unsigned int>() ||
            typeId == azrtti_typeid<unsigned long>() ||
            typeId == azrtti_typeid<AZ::u64>() ||
            typeId == azrtti_typeid<float>() ||
            typeId == azrtti_typeid<double>() ||
            typeId == azrtti_typeid<AZ::EntityId>() ||
            typeId == azrtti_typeid<AZStd::vector<AZ::EntityId>>() ||
            typeId == azrtti_typeid<AZ::Vector2>() ||
            typeId == azrtti_typeid<AZ::Vector3>() ||
            typeId == azrtti_typeid<AZ::Quaternion>() ||
            typeId == azrtti_typeid<AZ::Transform>())
        {
            return !HasTrait(parameter, AZ::BehaviorParameter::TR_POINTER);
        }

        return false;
    }

    bool BehaviorContextInspector::IsSupportedMethod(const AZ::BehaviorMethod& method) const
    {
        if (method.HasResult())
        {
            const AZ::BehaviorParameter* result = method.GetResult();
            if (result == nullptr || !IsSupportedType(*result))
            {
                return false;
            }
        }

        for (size_t argumentIndex = 0; argumentIndex < method.GetNumArguments(); ++argumentIndex)
        {
            const AZ::BehaviorParameter* argument = method.GetArgument(argumentIndex);
            if (argument == nullptr || !IsSupportedType(*argument))
            {
                return false;
            }
        }

        return true;
    }

    AZStd::string BehaviorContextInspector::FormatMethodSignature(AZ::BehaviorContext& behaviorContext, const AZ::BehaviorMethod& method) const
    {
        AZStd::string signature;
        if (method.HasResult())
        {
            signature += GetTypeName(behaviorContext, *method.GetResult());
        }
        else
        {
            signature += "void";
        }

        signature += " ";
        signature += method.m_name;
        signature += "(";

        for (size_t argumentIndex = 0; argumentIndex < method.GetNumArguments(); ++argumentIndex)
        {
            if (argumentIndex > 0)
            {
                signature += ", ";
            }

            const AZ::BehaviorParameter* argument = method.GetArgument(argumentIndex);
            if (argument == nullptr)
            {
                signature += "<null>";
                continue;
            }

            if (HasTrait(*argument, AZ::BehaviorParameter::TR_CONST))
            {
                signature += "const ";
            }

            signature += GetTypeName(behaviorContext, *argument);

            if (HasTrait(*argument, AZ::BehaviorParameter::TR_POINTER))
            {
                signature += "*";
            }
            else if (HasTrait(*argument, AZ::BehaviorParameter::TR_REFERENCE))
            {
                signature += "&";
            }

            if (const AZStd::string* argumentName = method.GetArgumentName(argumentIndex);
                argumentName != nullptr && !argumentName->empty())
            {
                signature += " ";
                signature += *argumentName;
            }
            else if (IsReferenceOrPointer(*argument))
            {
                signature += " value";
            }
        }

        signature += ")";
        return signature;
    }
} // namespace AngelScript
