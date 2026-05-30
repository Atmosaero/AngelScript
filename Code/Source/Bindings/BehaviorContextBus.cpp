#include <Bindings/BehaviorContextBus.h>

#include <Bindings/Entity.h>
#include <Bindings/Math.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
#include <angelscript.h>
#include <scriptarray.h>
#endif

namespace AngelScript
{
    namespace
    {
        constexpr const char* BehaviorContextBusLogWindow = "AngelScript";

#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        constexpr asPWORD BehaviorContextFunctionUserDataSlot = 0x42435553; // 'BCUS'

        enum class MarshallKind
        {
            Void,
            Bool,
            Char,
            Int8,
            Int16,
            Int32,
            Int64,
            UInt8,
            UInt16,
            UInt32,
            UInt64,
            Float,
            Double,
            String,
            CString,
            StringView,
            Entity,
            Vec2,
            Vec3,
            Quat,
            Transform,
            EntityArray
        };

        struct TypeBinding
        {
            MarshallKind m_kind = MarshallKind::Void;
            const char* m_scriptType = nullptr;
            bool m_isObject = false;
            bool m_isReference = false;
            bool m_isConst = false;
            bool m_isHandleReturn = false;

            bool IsOutReference() const
            {
                return m_isReference && !m_isConst;
            }
        };

        struct CachedBehaviorEbusEvent
        {
            const char* m_ebusName = nullptr;
            const char* m_eventName = nullptr;
            const AZ::BehaviorMethod* m_method = nullptr;
            bool m_resolved = false;
        };

        struct RegisteredBehaviorEbusFunction
        {
            AZStd::string m_scriptNamespace;
            AZStd::string m_scriptDeclaration;
            AZStd::string m_ebusName;
            AZStd::string m_eventName;
            AZStd::vector<TypeBinding> m_argumentBindings;
            TypeBinding m_resultBinding;
            CachedBehaviorEbusEvent m_cachedEvent;
        };

        struct MarshallStorage
        {
            bool m_bool = false;
            char m_char = 0;
            AZ::s8 m_int8 = 0;
            AZ::s16 m_int16 = 0;
            int m_int32 = 0;
            AZ::s64 m_int64 = 0;
            AZ::u8 m_uint8 = 0;
            AZ::u16 m_uint16 = 0;
            unsigned int m_uint32 = 0;
            AZ::u64 m_uint64 = 0;
            float m_float = 0.0f;
            double m_double = 0.0;
            AZStd::string m_string;
            const char* m_cString = nullptr;
            AZStd::string_view m_stringView;
            AZ::EntityId m_entityId;
            AZ::Vector2 m_vector2 = AZ::Vector2::CreateZero();
            AZ::Vector3 m_vector3 = AZ::Vector3::CreateZero();
            AZ::Quaternion m_quaternion = AZ::Quaternion::CreateIdentity();
            AZ::Transform m_transform = AZ::Transform::CreateIdentity();
            AZStd::vector<AZ::EntityId> m_entityIds;

            void* GetValueAddress(MarshallKind kind)
            {
                switch (kind)
                {
                case MarshallKind::Bool:
                    return &m_bool;
                case MarshallKind::Char:
                    return &m_char;
                case MarshallKind::Int8:
                    return &m_int8;
                case MarshallKind::Int16:
                    return &m_int16;
                case MarshallKind::Int32:
                    return &m_int32;
                case MarshallKind::Int64:
                    return &m_int64;
                case MarshallKind::UInt8:
                    return &m_uint8;
                case MarshallKind::UInt16:
                    return &m_uint16;
                case MarshallKind::UInt32:
                    return &m_uint32;
                case MarshallKind::UInt64:
                    return &m_uint64;
                case MarshallKind::Float:
                    return &m_float;
                case MarshallKind::Double:
                    return &m_double;
                case MarshallKind::String:
                    return &m_string;
                case MarshallKind::CString:
                    return &m_cString;
                case MarshallKind::StringView:
                    return &m_stringView;
                case MarshallKind::Entity:
                    return &m_entityId;
                case MarshallKind::Vec2:
                    return &m_vector2;
                case MarshallKind::Vec3:
                    return &m_vector3;
                case MarshallKind::Quat:
                    return &m_quaternion;
                case MarshallKind::Transform:
                    return &m_transform;
                case MarshallKind::EntityArray:
                    return &m_entityIds;
                case MarshallKind::Void:
                default:
                    return nullptr;
                }
            }
        };

        AZStd::vector<AZStd::unique_ptr<RegisteredBehaviorEbusFunction>>& GetRegisteredBehaviorFunctions()
        {
            static AZStd::vector<AZStd::unique_ptr<RegisteredBehaviorEbusFunction>> functions;
            return functions;
        }

        AZ::BehaviorContext* GetBehaviorContext()
        {
            AZ::BehaviorContext* behaviorContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                behaviorContext,
                &AZ::ComponentApplicationBus::Events::GetBehaviorContext);
            return behaviorContext;
        }

        bool HasTrait(const AZ::BehaviorParameter& parameter, AZ::BehaviorParameter::Traits trait)
        {
            return (parameter.m_traits & trait) != 0;
        }

        bool HasUnsupportedTraits(const AZ::BehaviorParameter& parameter)
        {
            constexpr AZ::u32 UnsupportedTraits =
                AZ::BehaviorParameter::TR_ARRAY_BEGIN |
                AZ::BehaviorParameter::TR_ARRAY_END |
                AZ::BehaviorParameter::TR_ARRAY_SIZE;
            if ((parameter.m_traits & UnsupportedTraits) != 0)
            {
                return true;
            }

            const bool isPointer = HasTrait(parameter, AZ::BehaviorParameter::TR_POINTER);
            const bool isString = HasTrait(parameter, AZ::BehaviorParameter::TR_STRING);
            return isPointer && !isString;
        }

        ScriptEntity ToScriptEntity(AZ::EntityId entityId)
        {
            ScriptEntity value;
            value.m_entityId = entityId.IsValid()
                ? static_cast<AZ::u64>(entityId)
                : AZ::EntityId::InvalidEntityId;
            return value;
        }

        AZ::EntityId ToEntityId(const ScriptEntity& entity)
        {
            return AZ::EntityId(entity.m_entityId);
        }

        ScriptVec2 ToScriptVec2(const AZ::Vector2& value)
        {
            return ScriptVec2{ value.GetX(), value.GetY() };
        }

        AZ::Vector2 ToAzVector2(const ScriptVec2& value)
        {
            return AZ::Vector2(value.x, value.y);
        }

        ScriptVec3 ToScriptVec3(const AZ::Vector3& value)
        {
            return ScriptVec3{ value.GetX(), value.GetY(), value.GetZ() };
        }

        AZ::Vector3 ToAzVector3(const ScriptVec3& value)
        {
            return AZ::Vector3(value.x, value.y, value.z);
        }

        ScriptQuat ToScriptQuat(const AZ::Quaternion& value)
        {
            return ScriptQuat{ value.GetX(), value.GetY(), value.GetZ(), value.GetW() };
        }

        AZ::Quaternion ToAzQuaternion(const ScriptQuat& value)
        {
            return AZ::Quaternion(value.x, value.y, value.z, value.w);
        }

        ScriptTransform ToScriptTransform(const AZ::Transform& value)
        {
            return ScriptTransform{
                ToScriptVec3(value.GetBasisX()),
                ToScriptVec3(value.GetBasisY()),
                ToScriptVec3(value.GetBasisZ()),
                ToScriptVec3(value.GetTranslation())
            };
        }

        AZ::Transform ToAzTransform(const ScriptTransform& value)
        {
            const AZ::Matrix3x3 basis = AZ::Matrix3x3::CreateFromColumns(
                ToAzVector3(value.basisX),
                ToAzVector3(value.basisY),
                ToAzVector3(value.basisZ));
            return AZ::Transform::CreateFromMatrix3x3AndTranslation(
                basis,
                ToAzVector3(value.translation));
        }

        bool IsTransformBusTransformCall(const RegisteredBehaviorEbusFunction& function)
        {
            return function.m_ebusName == "TransformBus" &&
                (function.m_eventName == "SetLocalTM" || function.m_eventName == "SetWorldTM");
        }

        bool IsTransformBusTransformGetter(const RegisteredBehaviorEbusFunction& function)
        {
            return function.m_ebusName == "TransformBus" &&
                (function.m_eventName == "GetLocalTM" || function.m_eventName == "GetWorldTM");
        }

        bool TryDispatchNativeTransformBusCall(
            const RegisteredBehaviorEbusFunction& function,
            const AZStd::vector<TypeBinding>& argumentBindings,
            const AZStd::vector<MarshallStorage>& argumentStorage)
        {
            if (function.m_ebusName != "TransformBus" || argumentBindings.size() < 2 || argumentStorage.size() < 2)
            {
                return false;
            }

            if (argumentBindings[0].m_kind != MarshallKind::Entity || argumentBindings[1].m_kind != MarshallKind::Transform)
            {
                return false;
            }

            const AZ::EntityId entityId = argumentStorage[0].m_entityId;
            if (!entityId.IsValid())
            {
                return false;
            }

            if (function.m_eventName == "SetLocalTM")
            {
                AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetLocalTM, argumentStorage[1].m_transform);
                return true;
            }

            return false;
        }

        bool TryDispatchNativeTransformBusResult(
            const RegisteredBehaviorEbusFunction& function,
            const AZStd::vector<TypeBinding>& argumentBindings,
            const AZStd::vector<MarshallStorage>& argumentStorage,
            MarshallStorage& resultStorage)
        {
            if (!IsTransformBusTransformGetter(function) ||
                function.m_resultBinding.m_kind != MarshallKind::Transform ||
                argumentBindings.empty() ||
                argumentStorage.empty() ||
                argumentBindings[0].m_kind != MarshallKind::Entity)
            {
                return false;
            }

            const AZ::EntityId entityId = argumentStorage[0].m_entityId;
            if (!entityId.IsValid())
            {
                return false;
            }

            if (function.m_eventName == "GetLocalTM")
            {
                AZ::TransformBus::EventResult(resultStorage.m_transform, entityId, &AZ::TransformBus::Events::GetLocalTM);
                return true;
            }

            if (function.m_eventName == "GetWorldTM")
            {
                AZ::TransformBus::EventResult(resultStorage.m_transform, entityId, &AZ::TransformBus::Events::GetWorldTM);
                return true;
            }

            return false;
        }

        void CopyEntityIdsFromScriptArray(AZStd::vector<AZ::EntityId>& destination, const CScriptArray& source)
        {
            const asUINT size = source.GetSize();
            destination.clear();
            destination.reserve(size);
            for (asUINT index = 0; index < size; ++index)
            {
                const auto* entity = reinterpret_cast<const ScriptEntity*>(source.At(index));
                destination.push_back(entity != nullptr ? ToEntityId(*entity) : AZ::EntityId());
            }
        }

        CScriptArray* CreateScriptEntityArray(asIScriptEngine* engine, const AZStd::vector<AZ::EntityId>& source)
        {
            if (engine == nullptr)
            {
                return nullptr;
            }

            asITypeInfo* arrayType = engine->GetTypeInfoByDecl("array<Entity>");
            if (arrayType == nullptr)
            {
                AZ_Warning(BehaviorContextBusLogWindow, false, "AngelScript type 'array<Entity>' is not registered.");
                return nullptr;
            }

            CScriptArray* array = CScriptArray::Create(arrayType, static_cast<asUINT>(source.size()));
            if (array == nullptr)
            {
                return nullptr;
            }

            for (asUINT index = 0; index < source.size(); ++index)
            {
                *reinterpret_cast<ScriptEntity*>(array->At(index)) = ToScriptEntity(source[index]);
            }

            return array;
        }

        TypeBinding ResolveTypeBinding(const AZ::BehaviorParameter& parameter)
        {
            if (parameter.m_typeId.IsNull())
            {
                return {};
            }

            const AZ::TypeId typeId = parameter.m_typeId;
            const bool isReference = HasTrait(parameter, AZ::BehaviorParameter::TR_REFERENCE);
            const bool isConst = HasTrait(parameter, AZ::BehaviorParameter::TR_CONST);
            if (HasTrait(parameter, AZ::BehaviorParameter::TR_STRING))
            {
                if (typeId == azrtti_typeid<char>() &&
                    HasTrait(parameter, AZ::BehaviorParameter::TR_POINTER) &&
                    isConst)
                {
                    return { MarshallKind::CString, "string", true, false, true, false };
                }

                if (typeId == azrtti_typeid<AZStd::string>())
                {
                    return { MarshallKind::String, "string", true, isReference, isConst, false };
                }

                if (typeId == azrtti_typeid<AZStd::string_view>())
                {
                    return { MarshallKind::StringView, "string", true, isReference, isConst, false };
                }
            }

            if (HasUnsupportedTraits(parameter))
            {
                return {};
            }

            if (typeId == azrtti_typeid<bool>())
            {
                return { MarshallKind::Bool, "bool", false, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<char>())
            {
                return { MarshallKind::Char, "int8", false, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<AZ::s8>())
            {
                return { MarshallKind::Int8, "int8", false, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<AZ::s16>())
            {
                return { MarshallKind::Int16, "int16", false, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<int>())
            {
                return { MarshallKind::Int32, "int", false, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<AZ::s64>())
            {
                return { MarshallKind::Int64, "int64", false, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<AZ::u8>())
            {
                return { MarshallKind::UInt8, "uint8", false, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<AZ::u16>())
            {
                return { MarshallKind::UInt16, "uint16", false, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<unsigned int>())
            {
                return { MarshallKind::UInt32, "uint", false, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<AZ::u64>())
            {
                return { MarshallKind::UInt64, "uint64", false, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<float>())
            {
                return { MarshallKind::Float, "float", false, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<double>())
            {
                return { MarshallKind::Double, "double", false, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<AZ::EntityId>())
            {
                return { MarshallKind::Entity, "Entity", true, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<AZ::Vector2>())
            {
                return { MarshallKind::Vec2, "Vec2", true, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<AZ::Vector3>())
            {
                return { MarshallKind::Vec3, "Vec3", true, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<AZ::Quaternion>())
            {
                return { MarshallKind::Quat, "Quat", true, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<AZ::Transform>())
            {
                return { MarshallKind::Transform, "Transform", true, isReference, isConst, false };
            }
            if (typeId == azrtti_typeid<AZStd::vector<AZ::EntityId>>())
            {
                return { MarshallKind::EntityArray, "array<Entity>", true, isReference, isConst, true };
            }

            return {};
        }

        AZStd::string SanitizeIdentifier(AZStd::string_view value)
        {
            AZStd::string result;
            result.reserve(value.size() + 1);

            auto appendSanitizedCharacter = [&result](char character, bool firstCharacter)
            {
                const bool isAlpha = (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
                const bool isDigit = character >= '0' && character <= '9';
                if (isAlpha || character == '_' || (!firstCharacter && isDigit))
                {
                    result.push_back(character);
                }
                else if (result.empty() || result.back() != '_')
                {
                    result.push_back('_');
                }
            };

            for (char character : value)
            {
                appendSanitizedCharacter(character, result.empty());
            }

            if (result.empty())
            {
                result = "_";
            }
            else if (result.front() >= '0' && result.front() <= '9')
            {
                result.insert(result.begin(), '_');
            }

            return result;
        }

        AZStd::string BuildScriptNamespace(AZStd::string_view ebusName)
        {
            AZStd::string namespaceName(ebusName);
            constexpr AZStd::string_view RequestBusSuffix = "RequestBus";
            if (namespaceName.size() > RequestBusSuffix.size() &&
                namespaceName.compare(namespaceName.size() - RequestBusSuffix.size(), RequestBusSuffix.size(), RequestBusSuffix.data()) == 0)
            {
                namespaceName.replace(namespaceName.size() - RequestBusSuffix.size(), RequestBusSuffix.size(), "Bus");
            }

            return SanitizeIdentifier(namespaceName);
        }

        AZStd::string BuildArgumentDeclaration(const TypeBinding& binding)
        {
            if (binding.IsOutReference())
            {
                return AZStd::string::format("%s &out", binding.m_scriptType);
            }

            if (binding.m_isReference || binding.m_isObject)
            {
                return AZStd::string::format("const %s &in", binding.m_scriptType);
            }

            return AZStd::string(binding.m_scriptType);
        }

        AZStd::string BuildScriptDeclaration(
            const AZ::BehaviorMethod& method,
            AZStd::vector<TypeBinding>& argumentBindings,
            TypeBinding& resultBinding)
        {
            const AZ::BehaviorParameter* resultParameter = method.HasResult() ? method.GetResult() : nullptr;
            resultBinding = {};
            if (resultParameter != nullptr)
            {
                resultBinding = ResolveTypeBinding(*resultParameter);
                if (resultBinding.m_scriptType == nullptr)
                {
                    return {};
                }
            }

            AZStd::string resultScriptType = "void";
            if (resultParameter != nullptr)
            {
                resultScriptType = resultBinding.m_scriptType;
                if (resultBinding.m_isHandleReturn)
                {
                    resultScriptType += "@";
                }
            }

            argumentBindings.clear();
            argumentBindings.reserve(method.GetNumArguments());

            AZStd::string declaration = AZStd::string::format("%s %s(", resultScriptType.c_str(), SanitizeIdentifier(method.m_name).c_str());
            for (size_t argumentIndex = 0; argumentIndex < method.GetNumArguments(); ++argumentIndex)
            {
                const AZ::BehaviorParameter* parameter = method.GetArgument(argumentIndex);
                if (parameter == nullptr)
                {
                    return {};
                }

                const TypeBinding binding = ResolveTypeBinding(*parameter);
                if (binding.m_scriptType == nullptr)
                {
                    return {};
                }

                if (argumentIndex > 0)
                {
                    declaration += ", ";
                }

                declaration += BuildArgumentDeclaration(binding);
                argumentBindings.push_back(binding);
            }

            declaration += ")";
            return declaration;
        }

        const AZ::BehaviorMethod* SelectBehaviorEbusMethod(const AZ::BehaviorEBus& ebus, const AZ::BehaviorEBusEventSender& sender)
        {
            const bool hasBusId = !ebus.m_idParam.m_typeId.IsNull();
            return hasBusId
                ? (sender.m_event != nullptr ? sender.m_event : sender.m_broadcast)
                : (sender.m_broadcast != nullptr ? sender.m_broadcast : sender.m_event);
        }

        const AZ::BehaviorMethod* FindBehaviorEbusEvent(const char* ebusName, const char* eventName)
        {
            AZ::BehaviorContext* behaviorContext = GetBehaviorContext();
            if (behaviorContext == nullptr)
            {
                AZ_Warning(BehaviorContextBusLogWindow, false, "BehaviorContext is not available while resolving %s::%s.", ebusName, eventName);
                return nullptr;
            }

            const AZ::BehaviorEBus* ebus = behaviorContext->FindEBusByReflectedName(ebusName);
            if (ebus == nullptr)
            {
                AZ_Warning(BehaviorContextBusLogWindow, false, "BehaviorContext EBus '%s' was not found.", ebusName);
                return nullptr;
            }

            auto eventIterator = ebus->m_events.find(eventName);
            if (eventIterator == ebus->m_events.end())
            {
                AZ_Warning(BehaviorContextBusLogWindow, false, "BehaviorContext EBus event '%s::%s' was not found.", ebusName, eventName);
                return nullptr;
            }

            const AZ::BehaviorMethod* method = SelectBehaviorEbusMethod(*ebus, eventIterator->second);
            if (method == nullptr)
            {
                AZ_Warning(BehaviorContextBusLogWindow, false, "BehaviorContext EBus event '%s::%s' does not expose a callable method.", ebusName, eventName);
            }

            return method;
        }

        const AZ::BehaviorMethod* ResolveCachedBehaviorEbusEvent(CachedBehaviorEbusEvent& event)
        {
            if (!event.m_resolved)
            {
                event.m_method = FindBehaviorEbusEvent(event.m_ebusName, event.m_eventName);
                event.m_resolved = true;
            }

            return event.m_method;
        }

        bool ReadArgument(asIScriptGeneric* generic, asUINT argumentIndex, MarshallKind kind, MarshallStorage& storage)
        {
            switch (kind)
            {
            case MarshallKind::Bool:
                storage.m_bool = generic->GetArgByte(argumentIndex) != 0;
                return true;
            case MarshallKind::Char:
                storage.m_char = static_cast<char>(generic->GetArgByte(argumentIndex));
                return true;
            case MarshallKind::Int8:
                storage.m_int8 = static_cast<AZ::s8>(generic->GetArgByte(argumentIndex));
                return true;
            case MarshallKind::Int16:
                storage.m_int16 = static_cast<AZ::s16>(generic->GetArgWord(argumentIndex));
                return true;
            case MarshallKind::Int32:
                storage.m_int32 = static_cast<int>(generic->GetArgDWord(argumentIndex));
                return true;
            case MarshallKind::Int64:
                storage.m_int64 = static_cast<AZ::s64>(generic->GetArgQWord(argumentIndex));
                return true;
            case MarshallKind::UInt8:
                storage.m_uint8 = static_cast<AZ::u8>(generic->GetArgByte(argumentIndex));
                return true;
            case MarshallKind::UInt16:
                storage.m_uint16 = static_cast<AZ::u16>(generic->GetArgWord(argumentIndex));
                return true;
            case MarshallKind::UInt32:
                storage.m_uint32 = static_cast<unsigned int>(generic->GetArgDWord(argumentIndex));
                return true;
            case MarshallKind::UInt64:
                storage.m_uint64 = static_cast<AZ::u64>(generic->GetArgQWord(argumentIndex));
                return true;
            case MarshallKind::Float:
                storage.m_float = generic->GetArgFloat(argumentIndex);
                return true;
            case MarshallKind::Double:
                storage.m_double = generic->GetArgDouble(argumentIndex);
                return true;
            case MarshallKind::String:
            case MarshallKind::CString:
            case MarshallKind::StringView:
            {
                const auto* value = reinterpret_cast<const std::string*>(generic->GetArgObject(argumentIndex));
                if (value == nullptr)
                {
                    return false;
                }

                storage.m_string = value->c_str();
                storage.m_cString = storage.m_string.c_str();
                storage.m_stringView = storage.m_string;
                return true;
            }
            case MarshallKind::Entity:
            {
                const auto* value = reinterpret_cast<const ScriptEntity*>(generic->GetArgObject(argumentIndex));
                if (value == nullptr)
                {
                    return false;
                }

                storage.m_entityId = AZ::EntityId(value->m_entityId);
                return true;
            }
            case MarshallKind::Vec2:
            {
                const auto* value = reinterpret_cast<const ScriptVec2*>(generic->GetArgObject(argumentIndex));
                if (value == nullptr)
                {
                    return false;
                }

                storage.m_vector2 = AZ::Vector2(value->x, value->y);
                return true;
            }
            case MarshallKind::Vec3:
            {
                const auto* value = reinterpret_cast<const ScriptVec3*>(generic->GetArgObject(argumentIndex));
                if (value == nullptr)
                {
                    return false;
                }

                storage.m_vector3 = AZ::Vector3(value->x, value->y, value->z);
                return true;
            }
            case MarshallKind::Quat:
            {
                const auto* value = reinterpret_cast<const ScriptQuat*>(generic->GetArgObject(argumentIndex));
                if (value == nullptr)
                {
                    return false;
                }

                storage.m_quaternion = AZ::Quaternion(value->x, value->y, value->z, value->w);
                return true;
            }
            case MarshallKind::Void:
            default:
                return false;
            }
        }

        bool ReadArgument(asIScriptGeneric* generic, asUINT argumentIndex, const TypeBinding& binding, MarshallStorage& storage)
        {
            if (binding.m_isReference)
            {
                void* address = generic->GetArgAddress(argumentIndex);
                if (address == nullptr)
                {
                    return false;
                }

                switch (binding.m_kind)
                {
                case MarshallKind::Bool:
                    storage.m_bool = *reinterpret_cast<bool*>(address);
                    return true;
                case MarshallKind::Char:
                    storage.m_char = *reinterpret_cast<char*>(address);
                    return true;
                case MarshallKind::Int8:
                    storage.m_int8 = *reinterpret_cast<AZ::s8*>(address);
                    return true;
                case MarshallKind::Int16:
                    storage.m_int16 = *reinterpret_cast<AZ::s16*>(address);
                    return true;
                case MarshallKind::Int32:
                    storage.m_int32 = *reinterpret_cast<int*>(address);
                    return true;
                case MarshallKind::Int64:
                    storage.m_int64 = *reinterpret_cast<AZ::s64*>(address);
                    return true;
                case MarshallKind::UInt8:
                    storage.m_uint8 = *reinterpret_cast<AZ::u8*>(address);
                    return true;
                case MarshallKind::UInt16:
                    storage.m_uint16 = *reinterpret_cast<AZ::u16*>(address);
                    return true;
                case MarshallKind::UInt32:
                    storage.m_uint32 = *reinterpret_cast<unsigned int*>(address);
                    return true;
                case MarshallKind::UInt64:
                    storage.m_uint64 = *reinterpret_cast<AZ::u64*>(address);
                    return true;
                case MarshallKind::Float:
                    storage.m_float = *reinterpret_cast<float*>(address);
                    return true;
                case MarshallKind::Double:
                    storage.m_double = *reinterpret_cast<double*>(address);
                    return true;
                case MarshallKind::String:
                case MarshallKind::CString:
                case MarshallKind::StringView:
                {
                    const auto* value = reinterpret_cast<const std::string*>(address);
                    storage.m_string = value != nullptr ? value->c_str() : "";
                    storage.m_cString = storage.m_string.c_str();
                    storage.m_stringView = storage.m_string;
                    return true;
                }
                case MarshallKind::Entity:
                    storage.m_entityId = ToEntityId(*reinterpret_cast<const ScriptEntity*>(address));
                    return true;
                case MarshallKind::Vec2:
                    storage.m_vector2 = ToAzVector2(*reinterpret_cast<const ScriptVec2*>(address));
                    return true;
                case MarshallKind::Vec3:
                    storage.m_vector3 = ToAzVector3(*reinterpret_cast<const ScriptVec3*>(address));
                    return true;
                case MarshallKind::Quat:
                    storage.m_quaternion = ToAzQuaternion(*reinterpret_cast<const ScriptQuat*>(address));
                    return true;
                case MarshallKind::Transform:
                    storage.m_transform = ToAzTransform(*reinterpret_cast<const ScriptTransform*>(address));
                    return true;
                case MarshallKind::EntityArray:
                    CopyEntityIdsFromScriptArray(storage.m_entityIds, *reinterpret_cast<const CScriptArray*>(address));
                    return true;
                case MarshallKind::Void:
                default:
                    return false;
                }
            }

            return ReadArgument(generic, argumentIndex, binding.m_kind, storage);
        }

        bool WriteBackArgument(asIScriptGeneric* generic, asUINT argumentIndex, const TypeBinding& binding, const MarshallStorage& storage)
        {
            if (!binding.IsOutReference())
            {
                return true;
            }

            void* address = generic->GetArgAddress(argumentIndex);
            if (address == nullptr)
            {
                return false;
            }

            switch (binding.m_kind)
            {
            case MarshallKind::Bool:
                *reinterpret_cast<bool*>(address) = storage.m_bool;
                return true;
            case MarshallKind::Char:
                *reinterpret_cast<char*>(address) = storage.m_char;
                return true;
            case MarshallKind::Int8:
                *reinterpret_cast<AZ::s8*>(address) = storage.m_int8;
                return true;
            case MarshallKind::Int16:
                *reinterpret_cast<AZ::s16*>(address) = storage.m_int16;
                return true;
            case MarshallKind::Int32:
                *reinterpret_cast<int*>(address) = storage.m_int32;
                return true;
            case MarshallKind::Int64:
                *reinterpret_cast<AZ::s64*>(address) = storage.m_int64;
                return true;
            case MarshallKind::UInt8:
                *reinterpret_cast<AZ::u8*>(address) = storage.m_uint8;
                return true;
            case MarshallKind::UInt16:
                *reinterpret_cast<AZ::u16*>(address) = storage.m_uint16;
                return true;
            case MarshallKind::UInt32:
                *reinterpret_cast<unsigned int*>(address) = storage.m_uint32;
                return true;
            case MarshallKind::UInt64:
                *reinterpret_cast<AZ::u64*>(address) = storage.m_uint64;
                return true;
            case MarshallKind::Float:
                *reinterpret_cast<float*>(address) = storage.m_float;
                return true;
            case MarshallKind::Double:
                *reinterpret_cast<double*>(address) = storage.m_double;
                return true;
            case MarshallKind::String:
            case MarshallKind::CString:
            case MarshallKind::StringView:
                *reinterpret_cast<std::string*>(address) = storage.m_string.c_str();
                return true;
            case MarshallKind::Entity:
                *reinterpret_cast<ScriptEntity*>(address) = ToScriptEntity(storage.m_entityId);
                return true;
            case MarshallKind::Vec2:
                *reinterpret_cast<ScriptVec2*>(address) = ToScriptVec2(storage.m_vector2);
                return true;
            case MarshallKind::Vec3:
                *reinterpret_cast<ScriptVec3*>(address) = ToScriptVec3(storage.m_vector3);
                return true;
            case MarshallKind::Quat:
                *reinterpret_cast<ScriptQuat*>(address) = ToScriptQuat(storage.m_quaternion);
                return true;
            case MarshallKind::Transform:
                *reinterpret_cast<ScriptTransform*>(address) = ToScriptTransform(storage.m_transform);
                return true;
            case MarshallKind::EntityArray:
            {
                auto* array = reinterpret_cast<CScriptArray*>(address);
                array->Resize(static_cast<asUINT>(storage.m_entityIds.size()));
                for (asUINT index = 0; index < storage.m_entityIds.size(); ++index)
                {
                    *reinterpret_cast<ScriptEntity*>(array->At(index)) = ToScriptEntity(storage.m_entityIds[index]);
                }
                return true;
            }
            case MarshallKind::Void:
            default:
                return false;
            }
        }

        void SetDefaultReturn(asIScriptGeneric* generic, const TypeBinding& binding)
        {
            switch (binding.m_kind)
            {
            case MarshallKind::Void:
                return;
            case MarshallKind::Bool:
                generic->SetReturnByte(0);
                return;
            case MarshallKind::Char:
            case MarshallKind::Int8:
            case MarshallKind::UInt8:
                generic->SetReturnByte(0);
                return;
            case MarshallKind::Int16:
            case MarshallKind::UInt16:
                generic->SetReturnWord(0);
                return;
            case MarshallKind::Int32:
            case MarshallKind::UInt32:
                generic->SetReturnDWord(0);
                return;
            case MarshallKind::Int64:
            case MarshallKind::UInt64:
                generic->SetReturnQWord(0);
                return;
            case MarshallKind::Float:
                generic->SetReturnFloat(0.0f);
                return;
            case MarshallKind::Double:
                generic->SetReturnDouble(0.0);
                return;
            case MarshallKind::String:
            case MarshallKind::CString:
            case MarshallKind::StringView:
            {
                std::string value;
                generic->SetReturnObject(&value);
                return;
            }
            case MarshallKind::Entity:
            {
                ScriptEntity value;
                *reinterpret_cast<ScriptEntity*>(generic->GetAddressOfReturnLocation()) = value;
                return;
            }
            case MarshallKind::Vec2:
            {
                ScriptVec2 value;
                *reinterpret_cast<ScriptVec2*>(generic->GetAddressOfReturnLocation()) = value;
                return;
            }
            case MarshallKind::Vec3:
            {
                ScriptVec3 value;
                *reinterpret_cast<ScriptVec3*>(generic->GetAddressOfReturnLocation()) = value;
                return;
            }
            case MarshallKind::Quat:
            {
                ScriptQuat value;
                *reinterpret_cast<ScriptQuat*>(generic->GetAddressOfReturnLocation()) = value;
                return;
            }
            case MarshallKind::Transform:
            {
                ScriptTransform value;
                *reinterpret_cast<ScriptTransform*>(generic->GetAddressOfReturnLocation()) = value;
                return;
            }
            case MarshallKind::EntityArray:
                generic->SetReturnAddress(nullptr);
                return;
            }
        }

        void SetReturnValue(asIScriptGeneric* generic, const TypeBinding& binding, const MarshallStorage& storage)
        {
            switch (binding.m_kind)
            {
            case MarshallKind::Void:
                return;
            case MarshallKind::Bool:
                generic->SetReturnByte(storage.m_bool ? 1 : 0);
                return;
            case MarshallKind::Char:
                generic->SetReturnByte(static_cast<asBYTE>(storage.m_char));
                return;
            case MarshallKind::Int8:
                generic->SetReturnByte(static_cast<asBYTE>(storage.m_int8));
                return;
            case MarshallKind::UInt8:
                generic->SetReturnByte(static_cast<asBYTE>(storage.m_uint8));
                return;
            case MarshallKind::Int16:
                generic->SetReturnWord(static_cast<asWORD>(storage.m_int16));
                return;
            case MarshallKind::UInt16:
                generic->SetReturnWord(static_cast<asWORD>(storage.m_uint16));
                return;
            case MarshallKind::Int32:
                generic->SetReturnDWord(static_cast<asDWORD>(storage.m_int32));
                return;
            case MarshallKind::UInt32:
                generic->SetReturnDWord(static_cast<asDWORD>(storage.m_uint32));
                return;
            case MarshallKind::Int64:
                generic->SetReturnQWord(static_cast<asQWORD>(storage.m_int64));
                return;
            case MarshallKind::UInt64:
                generic->SetReturnQWord(static_cast<asQWORD>(storage.m_uint64));
                return;
            case MarshallKind::Float:
                generic->SetReturnFloat(storage.m_float);
                return;
            case MarshallKind::Double:
                generic->SetReturnDouble(storage.m_double);
                return;
            case MarshallKind::String:
            {
                std::string value = storage.m_string.c_str();
                generic->SetReturnObject(&value);
                return;
            }
            case MarshallKind::CString:
            {
                std::string value = storage.m_cString != nullptr ? storage.m_cString : "";
                generic->SetReturnObject(&value);
                return;
            }
            case MarshallKind::StringView:
            {
                std::string value(storage.m_stringView.data(), storage.m_stringView.size());
                generic->SetReturnObject(&value);
                return;
            }
            case MarshallKind::Entity:
            {
                *reinterpret_cast<ScriptEntity*>(generic->GetAddressOfReturnLocation()) = ToScriptEntity(storage.m_entityId);
                return;
            }
            case MarshallKind::Vec2:
            {
                *reinterpret_cast<ScriptVec2*>(generic->GetAddressOfReturnLocation()) = ToScriptVec2(storage.m_vector2);
                return;
            }
            case MarshallKind::Vec3:
            {
                *reinterpret_cast<ScriptVec3*>(generic->GetAddressOfReturnLocation()) = ToScriptVec3(storage.m_vector3);
                return;
            }
            case MarshallKind::Quat:
            {
                *reinterpret_cast<ScriptQuat*>(generic->GetAddressOfReturnLocation()) = ToScriptQuat(storage.m_quaternion);
                return;
            }
            case MarshallKind::Transform:
            {
                *reinterpret_cast<ScriptTransform*>(generic->GetAddressOfReturnLocation()) = ToScriptTransform(storage.m_transform);
                return;
            }
            case MarshallKind::EntityArray:
            {
                CScriptArray* array = CreateScriptEntityArray(generic->GetEngine(), storage.m_entityIds);
                generic->SetReturnAddress(array);
                return;
            }
            }
        }

        void BehaviorContextEbusDispatch(asIScriptGeneric* generic)
        {
            const auto* function = reinterpret_cast<RegisteredBehaviorEbusFunction*>(
                generic->GetFunction()->GetUserData(BehaviorContextFunctionUserDataSlot));
            if (function == nullptr)
            {
                AZ_Warning(BehaviorContextBusLogWindow, false, "BehaviorContext dispatch missing function metadata.");
                return;
            }

            RegisteredBehaviorEbusFunction& mutableFunction = *const_cast<RegisteredBehaviorEbusFunction*>(function);
            const AZ::BehaviorMethod* method = ResolveCachedBehaviorEbusEvent(mutableFunction.m_cachedEvent);
            if (method == nullptr)
            {
                SetDefaultReturn(generic, function->m_resultBinding);
                return;
            }

            const size_t argumentCount = function->m_argumentBindings.size();
            AZStd::vector<MarshallStorage> argumentStorage(argumentCount);
            AZStd::vector<AZ::BehaviorArgument> behaviorArguments(argumentCount);
            for (size_t argumentIndex = 0; argumentIndex < argumentCount; ++argumentIndex)
            {
                if (!ReadArgument(generic, static_cast<asUINT>(argumentIndex), function->m_argumentBindings[argumentIndex], argumentStorage[argumentIndex]))
                {
                    AZ_Warning(
                        BehaviorContextBusLogWindow,
                        false,
                        "Failed to marshal AngelScript argument %zu for %s::%s.",
                        argumentIndex,
                        function->m_ebusName.c_str(),
                        function->m_eventName.c_str());
                    SetDefaultReturn(generic, function->m_resultBinding);
                    return;
                }

                behaviorArguments[argumentIndex].Set(*method->GetArgument(argumentIndex));
                behaviorArguments[argumentIndex].m_value = argumentStorage[argumentIndex].GetValueAddress(function->m_argumentBindings[argumentIndex].m_kind);
            }

            if (function->m_resultBinding.m_kind == MarshallKind::Void)
            {
                const bool handledByNativePath = TryDispatchNativeTransformBusCall(*function, function->m_argumentBindings, argumentStorage);
                if (!handledByNativePath &&
                    !method->Call(behaviorArguments.data(), static_cast<AZ::u32>(behaviorArguments.size())))
                {
                    AZ_Warning(
                        BehaviorContextBusLogWindow,
                        false,
                        "BehaviorContext call %s::%s failed.",
                        function->m_ebusName.c_str(),
                        function->m_eventName.c_str());
                    return;
                }

                for (size_t argumentIndex = 0; argumentIndex < argumentCount; ++argumentIndex)
                {
                    if (!WriteBackArgument(generic, static_cast<asUINT>(argumentIndex), function->m_argumentBindings[argumentIndex], argumentStorage[argumentIndex]))
                    {
                        AZ_Warning(
                            BehaviorContextBusLogWindow,
                            false,
                            "Failed to write back AngelScript out argument %zu for %s::%s.",
                            argumentIndex,
                            function->m_ebusName.c_str(),
                            function->m_eventName.c_str());
                    }
                }

                return;
            }

            MarshallStorage resultStorage;
            const bool handledByNativeResultPath =
                TryDispatchNativeTransformBusResult(*function, function->m_argumentBindings, argumentStorage, resultStorage);
            if (!handledByNativeResultPath)
            {
                AZ::BehaviorArgument resultArgument;
                resultArgument.Set(*method->GetResult());
                resultArgument.m_value = resultStorage.GetValueAddress(function->m_resultBinding.m_kind);
                if (!method->Call(behaviorArguments.data(), static_cast<AZ::u32>(behaviorArguments.size()), &resultArgument))
                {
                    AZ_Warning(
                        BehaviorContextBusLogWindow,
                        false,
                        "BehaviorContext call %s::%s failed.",
                        function->m_ebusName.c_str(),
                        function->m_eventName.c_str());
                    SetDefaultReturn(generic, function->m_resultBinding);
                    return;
                }
            }

            for (size_t argumentIndex = 0; argumentIndex < argumentCount; ++argumentIndex)
            {
                if (!WriteBackArgument(generic, static_cast<asUINT>(argumentIndex), function->m_argumentBindings[argumentIndex], argumentStorage[argumentIndex]))
                {
                    AZ_Warning(
                        BehaviorContextBusLogWindow,
                        false,
                        "Failed to write back AngelScript out argument %zu for %s::%s.",
                        argumentIndex,
                        function->m_ebusName.c_str(),
                        function->m_eventName.c_str());
                }
            }

            SetReturnValue(generic, function->m_resultBinding, resultStorage);
        }

        bool RegisterBehaviorContextFunction(
            asIScriptEngine* engine,
            AZStd::unordered_set<AZStd::string>& registeredDeclarations,
            AZStd::unique_ptr<RegisteredBehaviorEbusFunction> function)
        {
            const AZStd::string declarationKey = function->m_scriptNamespace + "::" + function->m_scriptDeclaration;
            if (!registeredDeclarations.insert(declarationKey).second)
            {
                return false;
            }

            int result = engine->SetDefaultNamespace(function->m_scriptNamespace.c_str());
            if (result < 0)
            {
                AZ_Warning(
                    BehaviorContextBusLogWindow,
                    false,
                    "Failed to set AngelScript namespace '%s'. Result: %d",
                    function->m_scriptNamespace.c_str(),
                    result);
                return false;
            }

            result = engine->RegisterGlobalFunction(function->m_scriptDeclaration.c_str(), asFUNCTION(BehaviorContextEbusDispatch), asCALL_GENERIC);
            if (result < 0)
            {
                AZ_Warning(
                    BehaviorContextBusLogWindow,
                    false,
                    "Failed to register AngelScript function '%s::%s'. Result: %d",
                    function->m_scriptNamespace.c_str(),
                    function->m_scriptDeclaration.c_str(),
                    result);
                return false;
            }

            asIScriptFunction* scriptFunction = engine->GetGlobalFunctionByDecl(function->m_scriptDeclaration.c_str());
            if (scriptFunction == nullptr)
            {
                AZ_Warning(
                    BehaviorContextBusLogWindow,
                    false,
                    "Registered AngelScript function '%s::%s' could not be retrieved for user data binding.",
                    function->m_scriptNamespace.c_str(),
                    function->m_scriptDeclaration.c_str());
                return false;
            }

            function->m_cachedEvent = { function->m_ebusName.c_str(), function->m_eventName.c_str() };
            scriptFunction->SetUserData(function.get(), BehaviorContextFunctionUserDataSlot);
            GetRegisteredBehaviorFunctions().push_back(AZStd::move(function));
            return true;
        }

#endif
    } // namespace

    bool RegisterBehaviorContextBuses(asIScriptEngine* engine)
    {
#if defined(ANGELSCRIPT_RUNTIME_HAS_SDK)
        if (engine == nullptr)
        {
            AZ_Error(BehaviorContextBusLogWindow, false, "Cannot register BehaviorContext EBus bindings without an engine.");
            return false;
        }

        AZ::BehaviorContext* behaviorContext = GetBehaviorContext();
        if (behaviorContext == nullptr)
        {
            AZ_Warning(
                BehaviorContextBusLogWindow,
                false,
                "BehaviorContext is not available during AngelScript EBus registration. Skipping reflected EBus bindings.");
            return true;
        }

        auto& registeredFunctions = GetRegisteredBehaviorFunctions();
        registeredFunctions.clear();

        AZStd::unordered_set<AZStd::string> registeredDeclarations;
        size_t registeredCount = 0;

        for (const auto& ebusPair : behaviorContext->m_ebuses)
        {
            const AZ::BehaviorEBus* ebus = ebusPair.second;
            if (ebus == nullptr)
            {
                continue;
            }

            const AZStd::string scriptNamespace = BuildScriptNamespace(ebus->m_name);
            for (const auto& eventPair : ebus->m_events)
            {
                const AZ::BehaviorMethod* method = SelectBehaviorEbusMethod(*ebus, eventPair.second);
                if (method == nullptr)
                {
                    continue;
                }

                AZStd::unique_ptr<RegisteredBehaviorEbusFunction> function = AZStd::make_unique<RegisteredBehaviorEbusFunction>();
                function->m_scriptNamespace = scriptNamespace;
                function->m_ebusName = ebus->m_name;
                function->m_eventName = eventPair.first;
                function->m_scriptDeclaration = BuildScriptDeclaration(*method, function->m_argumentBindings, function->m_resultBinding);
                if (function->m_scriptDeclaration.empty())
                {
                    continue;
                }

                if (RegisterBehaviorContextFunction(engine, registeredDeclarations, AZStd::move(function)))
                {
                    ++registeredCount;
                }
            }
        }

        const int result = engine->SetDefaultNamespace("");
        if (result < 0)
        {
            AZ_Error(BehaviorContextBusLogWindow, false, "Failed to reset AngelScript namespace after BehaviorContext bus registration. Result: %d", result);
            return false;
        }

        AZ_TracePrintf(
            BehaviorContextBusLogWindow,
            "BehaviorContextBus: registered %zu AngelScript EBus functions.\n",
            registeredCount);

        return registeredCount > 0;
#else
        (void)engine;
        return false;
#endif
    }
} // namespace AngelScript
