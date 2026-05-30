
#pragma once

namespace AngelScript
{
    // System Component TypeIds
    inline constexpr const char* AngelScriptSystemComponentTypeId = "{8E10800E-E464-4B9E-994B-F1C025E57828}";
    inline constexpr const char* AngelScriptEditorSystemComponentTypeId = "{5FA24D50-6B26-49D7-8FE5-442F7B4D9C1B}";
    inline constexpr const char* AngelScriptComponentTypeId = "{5D746AC8-CA03-4061-B473-73A03ADC260D}";
    inline constexpr const char* EditorAngelScriptComponentTypeId = "{0A81EA9A-321D-4A89-9B0A-46D26D140A35}";
    inline constexpr const char* AngelScriptAssetTypeId = "{1BF0A0E3-94A0-4BE6-8487-8A360DEA976E}";

    // Module derived classes TypeIds
    inline constexpr const char* AngelScriptModuleInterfaceTypeId = "{2B2E01F1-1E2E-46C7-884C-F0024CF14D6B}";
    inline constexpr const char* AngelScriptModuleTypeId = "{1D1FC7CE-3354-4D92-91AE-4BF56012AE92}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* AngelScriptEditorModuleTypeId = AngelScriptModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* AngelScriptRequestsTypeId = "{AFC4BF65-4877-4E78-91FE-C8233641624A}";
} // namespace AngelScript
