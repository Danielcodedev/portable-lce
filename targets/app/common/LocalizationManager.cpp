#include "app/common/LocalizationManager.h"

#include <assert.h>
#include <stdlib.h>
#include <wchar.h>

#include <string>

#include "minecraft/GameEnums.h"
#include "app/common/App_structs.h"
#include "app/common/Localisation/StringTable.h"
#include "app/common/Colours/ColourTable.h"
#include "app/common/UI/All Platforms/ArchiveFile.h"
#include "app/linux/LinuxGame.h"
#include "java/Random.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "platform/input/input.h"
#include "platform/sdl2/Render.h"
#include "platform/XboxStubs.h"
#include "strings.h"
#include "util/StringHelpers.h"

int LocalizationManager::s_iHTMLFontSizesA[eHTMLSize_COUNT] = {
    20, 13, 20, 26};

TIPSTRUCT LocalizationManager::m_GameTipA[MAX_TIPS_GAMETIP] = {
    {0, IDS_TIPS_GAMETIP_1},  {0, IDS_TIPS_GAMETIP_2},
    {0, IDS_TIPS_GAMETIP_3},  {0, IDS_TIPS_GAMETIP_4},
    {0, IDS_TIPS_GAMETIP_5},  {0, IDS_TIPS_GAMETIP_6},
    {0, IDS_TIPS_GAMETIP_7},  {0, IDS_TIPS_GAMETIP_8},
    {0, IDS_TIPS_GAMETIP_9},  {0, IDS_TIPS_GAMETIP_10},
    {0, IDS_TIPS_GAMETIP_11}, {0, IDS_TIPS_GAMETIP_12},
    {0, IDS_TIPS_GAMETIP_13}, {0, IDS_TIPS_GAMETIP_14},
    {0, IDS_TIPS_GAMETIP_15}, {0, IDS_TIPS_GAMETIP_16},
    {0, IDS_TIPS_GAMETIP_17}, {0, IDS_TIPS_GAMETIP_18},
    {0, IDS_TIPS_GAMETIP_19}, {0, IDS_TIPS_GAMETIP_20},
    {0, IDS_TIPS_GAMETIP_21}, {0, IDS_TIPS_GAMETIP_22},
    {0, IDS_TIPS_GAMETIP_23}, {0, IDS_TIPS_GAMETIP_24},
    {0, IDS_TIPS_GAMETIP_25}, {0, IDS_TIPS_GAMETIP_26},
    {0, IDS_TIPS_GAMETIP_27}, {0, IDS_TIPS_GAMETIP_28},
    {0, IDS_TIPS_GAMETIP_29}, {0, IDS_TIPS_GAMETIP_30},
    {0, IDS_TIPS_GAMETIP_31}, {0, IDS_TIPS_GAMETIP_32},
    {0, IDS_TIPS_GAMETIP_33}, {0, IDS_TIPS_GAMETIP_34},
    {0, IDS_TIPS_GAMETIP_35}, {0, IDS_TIPS_GAMETIP_36},
    {0, IDS_TIPS_GAMETIP_37}, {0, IDS_TIPS_GAMETIP_38},
    {0, IDS_TIPS_GAMETIP_39}, {0, IDS_TIPS_GAMETIP_40},
    {0, IDS_TIPS_GAMETIP_41}, {0, IDS_TIPS_GAMETIP_42},
    {0, IDS_TIPS_GAMETIP_43}, {0, IDS_TIPS_GAMETIP_44},
    {0, IDS_TIPS_GAMETIP_45}, {0, IDS_TIPS_GAMETIP_46},
    {0, IDS_TIPS_GAMETIP_47}, {0, IDS_TIPS_GAMETIP_48},
    {0, IDS_TIPS_GAMETIP_49}, {0, IDS_TIPS_GAMETIP_50},
};

TIPSTRUCT LocalizationManager::m_TriviaTipA[MAX_TIPS_TRIVIATIP] = {
    {0, IDS_TIPS_TRIVIA_1},  {0, IDS_TIPS_TRIVIA_2},  {0, IDS_TIPS_TRIVIA_3},
    {0, IDS_TIPS_TRIVIA_4},  {0, IDS_TIPS_TRIVIA_5},  {0, IDS_TIPS_TRIVIA_6},
    {0, IDS_TIPS_TRIVIA_7},  {0, IDS_TIPS_TRIVIA_8},  {0, IDS_TIPS_TRIVIA_9},
    {0, IDS_TIPS_TRIVIA_10}, {0, IDS_TIPS_TRIVIA_11}, {0, IDS_TIPS_TRIVIA_12},
    {0, IDS_TIPS_TRIVIA_13}, {0, IDS_TIPS_TRIVIA_14}, {0, IDS_TIPS_TRIVIA_15},
    {0, IDS_TIPS_TRIVIA_16}, {0, IDS_TIPS_TRIVIA_17}, {0, IDS_TIPS_TRIVIA_18},
    {0, IDS_TIPS_TRIVIA_19}, {0, IDS_TIPS_TRIVIA_20},
};

Random* LocalizationManager::TipRandom = new Random();

LocalizationManager::LocalizationManager()
    : m_stringTable(nullptr), m_uiCurrentTip(0) {
    memset(m_TipIDA, 0, sizeof(m_TipIDA));
}

void LocalizationManager::loadStringTable(ArchiveFile* mediaArchive) {
    if (m_stringTable != nullptr) {
        // we need to unload the current std::string table, this is a reload
        delete m_stringTable;
    }
    std::wstring localisationFile = L"languages.loc";
    if (mediaArchive->hasFile(localisationFile)) {
        std::vector<uint8_t> locFile =
            mediaArchive->getFile(localisationFile);
        m_stringTable = new StringTable(locFile.data(), locFile.size());
    } else {
        m_stringTable = nullptr;
        assert(false);
        // AHHHHHHHHH.
    }
}

const wchar_t* LocalizationManager::getString(int iID) const {
    return m_stringTable->getString(iID);
}

int LocalizationManager::TipsSortFunction(const void* a, const void* b) {
    int s1 = ((TIPSTRUCT*)a)->iSortValue;
    int s2 = ((TIPSTRUCT*)b)->iSortValue;

    if (s1 > s2) {
        return 1;
    } else if (s1 == s2) {
        return 0;
    }

    return -1;
}

void LocalizationManager::initialiseTips() {
    memset(m_TipIDA, 0, sizeof(m_TipIDA));

    if (!RenderManager.IsHiDef()) {
        m_GameTipA[0].uiStringID = IDS_TIPS_GAMETIP_0;
    }

#if defined(_CONTENT_PACKAGE)
    for (int i = 1; i < MAX_TIPS_GAMETIP; i++) {
        m_GameTipA[i].iSortValue = TipRandom->nextInt();
    }
    qsort(&m_GameTipA[1], MAX_TIPS_GAMETIP - 1, sizeof(TIPSTRUCT),
          TipsSortFunction);
#endif

    for (int i = 0; i < MAX_TIPS_TRIVIATIP; i++) {
        m_TriviaTipA[i].iSortValue = TipRandom->nextInt();
    }
    qsort(m_TriviaTipA, MAX_TIPS_TRIVIATIP, sizeof(TIPSTRUCT),
          TipsSortFunction);

    int iCurrentGameTip = 0;
    int iCurrentTriviaTip = 0;

    for (int i = 0; i < MAX_TIPS_GAMETIP + MAX_TIPS_TRIVIATIP; i++) {
        if ((i % 3 == 2) && (iCurrentTriviaTip < MAX_TIPS_TRIVIATIP)) {
            m_TipIDA[i] = m_TriviaTipA[iCurrentTriviaTip++].uiStringID;
        } else {
            if (iCurrentGameTip < MAX_TIPS_GAMETIP) {
                m_TipIDA[i] = m_GameTipA[iCurrentGameTip++].uiStringID;
            } else {
                m_TipIDA[i] = m_TriviaTipA[iCurrentTriviaTip++].uiStringID;
            }
        }

        if (m_TipIDA[i] == 0) {
#if !defined(_CONTENT_PACKAGE)
            assert(0);
#endif
        }
    }

    m_uiCurrentTip = 0;
}

int LocalizationManager::getNextTip() {
    static bool bShowSkinDLCTip = true;
    if (app.GetNewDLCAvailable() && app.DisplayNewDLCTip()) {
        return IDS_TIPS_GAMETIP_NEWDLC;
    } else {
        if (bShowSkinDLCTip) {
            bShowSkinDLCTip = false;
            if (app.DLCInstallProcessCompleted()) {
                if (app.m_dlcManager.getPackCount(DLCManager::e_DLCType_Skin) ==
                    0) {
                    return IDS_TIPS_GAMETIP_SKINPACKS;
                }
            } else {
                return IDS_TIPS_GAMETIP_SKINPACKS;
            }
        }
    }

    if (m_uiCurrentTip == MAX_TIPS_GAMETIP + MAX_TIPS_TRIVIATIP)
        m_uiCurrentTip = 0;

    return m_TipIDA[m_uiCurrentTip++];
}

int LocalizationManager::getHTMLColour(eMinecraftColour colour) {
    Minecraft* pMinecraft = Minecraft::GetInstance();
    return pMinecraft->skins->getSelected()->getColourTable()->getColour(
        colour);
}

int LocalizationManager::getHTMLFontSize(EHTMLFontSize size) {
    return s_iHTMLFontSizesA[size];
}

std::wstring LocalizationManager::formatHTMLString(
    int iPad, const std::wstring& desc, int shadowColour /*= 0xFFFFFFFF*/) {
    std::wstring text(desc);

    wchar_t replacements[64];
    text = replaceAll(text, L"{*B*}", L"<br />");
    swprintf(replacements, 64, L"<font color=\"#%08x\">",
             getHTMLColour(eHTMLColor_T1));
    text = replaceAll(text, L"{*T1*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\">",
             getHTMLColour(eHTMLColor_T2));
    text = replaceAll(text, L"{*T2*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\">",
             getHTMLColour(eHTMLColor_T3));
    text = replaceAll(text, L"{*T3*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\">",
             getHTMLColour(eHTMLColor_Black));
    text = replaceAll(text, L"{*ETB*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\">",
             getHTMLColour(eHTMLColor_White));
    text = replaceAll(text, L"{*ETW*}", replacements);
    text = replaceAll(text, L"{*EF*}", L"</font>");

    swprintf(replacements, 64, L"<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_0), shadowColour);
    text = replaceAll(text, L"{*C0*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_1), shadowColour);
    text = replaceAll(text, L"{*C1*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_2), shadowColour);
    text = replaceAll(text, L"{*C2*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_3), shadowColour);
    text = replaceAll(text, L"{*C3*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_4), shadowColour);
    text = replaceAll(text, L"{*C4*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_5), shadowColour);
    text = replaceAll(text, L"{*C5*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_6), shadowColour);
    text = replaceAll(text, L"{*C6*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_7), shadowColour);
    text = replaceAll(text, L"{*C7*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_8), shadowColour);
    text = replaceAll(text, L"{*C8*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_9), shadowColour);
    text = replaceAll(text, L"{*C9*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_a), shadowColour);
    text = replaceAll(text, L"{*CA*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_b), shadowColour);
    text = replaceAll(text, L"{*CB*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_c), shadowColour);
    text = replaceAll(text, L"{*CC*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_d), shadowColour);
    text = replaceAll(text, L"{*CD*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_e), shadowColour);
    text = replaceAll(text, L"{*CE*}", replacements);
    swprintf(replacements, 64, L"<font color=\"#%08x\" shadowcolor=\"#%08x\">",
             getHTMLColour(eHTMLColor_f), shadowColour);
    text = replaceAll(text, L"{*CF*}", replacements);

    // Swap for southpaw.
    if (app.GetGameSettings(iPad, eGameSetting_ControlSouthPaw)) {
        text =
            replaceAll(text, L"{*CONTROLLER_ACTION_MOVE*}",
                       getActionReplacement(iPad, MINECRAFT_ACTION_LOOK_RIGHT));
        text = replaceAll(text, L"{*CONTROLLER_ACTION_LOOK*}",
                          getActionReplacement(iPad, MINECRAFT_ACTION_RIGHT));

        text = replaceAll(text, L"{*CONTROLLER_MENU_NAVIGATE*}",
                          getVKReplacement(VK_PAD_RTHUMB_LEFT));
    } else {
        text = replaceAll(text, L"{*CONTROLLER_ACTION_MOVE*}",
                          getActionReplacement(iPad, MINECRAFT_ACTION_RIGHT));
        text =
            replaceAll(text, L"{*CONTROLLER_ACTION_LOOK*}",
                       getActionReplacement(iPad, MINECRAFT_ACTION_LOOK_RIGHT));

        text = replaceAll(text, L"{*CONTROLLER_MENU_NAVIGATE*}",
                          getVKReplacement(VK_PAD_LTHUMB_LEFT));
    }

    text = replaceAll(text, L"{*CONTROLLER_ACTION_JUMP*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_JUMP));
    text =
        replaceAll(text, L"{*CONTROLLER_ACTION_SNEAK*}",
                   getActionReplacement(iPad, MINECRAFT_ACTION_SNEAK_TOGGLE));
    text = replaceAll(text, L"{*CONTROLLER_ACTION_USE*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_USE));
    text = replaceAll(text, L"{*CONTROLLER_ACTION_ACTION*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_ACTION));
    text = replaceAll(text, L"{*CONTROLLER_ACTION_LEFT_SCROLL*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_LEFT_SCROLL));
    text =
        replaceAll(text, L"{*CONTROLLER_ACTION_RIGHT_SCROLL*}",
                   getActionReplacement(iPad, MINECRAFT_ACTION_RIGHT_SCROLL));
    text = replaceAll(text, L"{*CONTROLLER_ACTION_INVENTORY*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_INVENTORY));
    text = replaceAll(text, L"{*CONTROLLER_ACTION_CRAFTING*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_CRAFTING));
    text = replaceAll(text, L"{*CONTROLLER_ACTION_DROP*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_DROP));
    text = replaceAll(
        text, L"{*CONTROLLER_ACTION_CAMERA*}",
        getActionReplacement(iPad, MINECRAFT_ACTION_RENDER_THIRD_PERSON));
    text = replaceAll(text, L"{*CONTROLLER_ACTION_MENU_PAGEDOWN*}",
                      getActionReplacement(iPad, ACTION_MENU_PAGEDOWN));
    text =
        replaceAll(text, L"{*CONTROLLER_ACTION_DISMOUNT*}",
                   getActionReplacement(iPad, MINECRAFT_ACTION_SNEAK_TOGGLE));
    text = replaceAll(text, L"{*CONTROLLER_VK_A*}", getVKReplacement(VK_PAD_A));
    text = replaceAll(text, L"{*CONTROLLER_VK_B*}", getVKReplacement(VK_PAD_B));
    text = replaceAll(text, L"{*CONTROLLER_VK_X*}", getVKReplacement(VK_PAD_X));
    text = replaceAll(text, L"{*CONTROLLER_VK_Y*}", getVKReplacement(VK_PAD_Y));
    text = replaceAll(text, L"{*CONTROLLER_VK_LB*}",
                      getVKReplacement(VK_PAD_LSHOULDER));
    text = replaceAll(text, L"{*CONTROLLER_VK_RB*}",
                      getVKReplacement(VK_PAD_RSHOULDER));
    text = replaceAll(text, L"{*CONTROLLER_VK_LS*}",
                      getVKReplacement(VK_PAD_LTHUMB_UP));
    text = replaceAll(text, L"{*CONTROLLER_VK_RS*}",
                      getVKReplacement(VK_PAD_RTHUMB_UP));
    text = replaceAll(text, L"{*CONTROLLER_VK_LT*}",
                      getVKReplacement(VK_PAD_LTRIGGER));
    text = replaceAll(text, L"{*CONTROLLER_VK_RT*}",
                      getVKReplacement(VK_PAD_RTRIGGER));
    text = replaceAll(text, L"{*ICON_SHANK_01*}",
                      getIconReplacement(XZP_ICON_SHANK_01));
    text = replaceAll(text, L"{*ICON_SHANK_03*}",
                      getIconReplacement(XZP_ICON_SHANK_03));
    text = replaceAll(text, L"{*CONTROLLER_ACTION_DPAD_UP*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_DPAD_UP));
    text = replaceAll(text, L"{*CONTROLLER_ACTION_DPAD_DOWN*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_DPAD_DOWN));
    text = replaceAll(text, L"{*CONTROLLER_ACTION_DPAD_RIGHT*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_DPAD_RIGHT));
    text = replaceAll(text, L"{*CONTROLLER_ACTION_DPAD_LEFT*}",
                      getActionReplacement(iPad, MINECRAFT_ACTION_DPAD_LEFT));

    std::uint32_t dwLanguage = XGetLanguage();
    switch (dwLanguage) {
        case XC_LANGUAGE_KOREAN:
        case XC_LANGUAGE_JAPANESE:
        case XC_LANGUAGE_TCHINESE:
            text = replaceAll(text, L"&nbsp;", L"");
            break;
    }

    return text;
}

std::wstring LocalizationManager::getActionReplacement(
    int iPad, unsigned char ucAction) {
    unsigned int input = PlatformInput.GetGameJoypadMaps(
        PlatformInput.GetJoypadMapVal(iPad), ucAction);

    std::wstring replacement = L"";

    if (input & _360_JOY_BUTTON_A)
        replacement = L"ButtonA";
    else if (input & _360_JOY_BUTTON_B)
        replacement = L"ButtonB";
    else if (input & _360_JOY_BUTTON_X)
        replacement = L"ButtonX";
    else if (input & _360_JOY_BUTTON_Y)
        replacement = L"ButtonY";
    else if ((input & _360_JOY_BUTTON_LSTICK_UP) ||
             (input & _360_JOY_BUTTON_LSTICK_DOWN) ||
             (input & _360_JOY_BUTTON_LSTICK_LEFT) ||
             (input & _360_JOY_BUTTON_LSTICK_RIGHT)) {
        replacement = L"ButtonLeftStick";
    } else if ((input & _360_JOY_BUTTON_RSTICK_LEFT) ||
               (input & _360_JOY_BUTTON_RSTICK_RIGHT) ||
               (input & _360_JOY_BUTTON_RSTICK_UP) ||
               (input & _360_JOY_BUTTON_RSTICK_DOWN)) {
        replacement = L"ButtonRightStick";
    } else if (input & _360_JOY_BUTTON_DPAD_LEFT)
        replacement = L"ButtonDpadL";
    else if (input & _360_JOY_BUTTON_DPAD_RIGHT)
        replacement = L"ButtonDpadR";
    else if (input & _360_JOY_BUTTON_DPAD_UP)
        replacement = L"ButtonDpadU";
    else if (input & _360_JOY_BUTTON_DPAD_DOWN)
        replacement = L"ButtonDpadD";
    else if (input & _360_JOY_BUTTON_LT)
        replacement = L"ButtonLeftTrigger";
    else if (input & _360_JOY_BUTTON_RT)
        replacement = L"ButtonRightTrigger";
    else if (input & _360_JOY_BUTTON_RB)
        replacement = L"ButtonRightBumper";
    else if (input & _360_JOY_BUTTON_LB)
        replacement = L"ButtonLeftBumper";
    else if (input & _360_JOY_BUTTON_BACK)
        replacement = L"ButtonBack";
    else if (input & _360_JOY_BUTTON_START)
        replacement = L"ButtonStart";
    else if (input & _360_JOY_BUTTON_RTHUMB)
        replacement = L"ButtonRS";
    else if (input & _360_JOY_BUTTON_LTHUMB)
        replacement = L"ButtonLS";

    wchar_t string[128];

#if defined(_WIN64)
    int size = 45;
    if (ui.getScreenWidth() < 1920) size = 30;
#else
    int size = 45;
#endif

    swprintf(string, 128,
             L"<img src=\"%ls\" align=\"middle\" height=\"%d\" width=\"%d\"/>",
             replacement.c_str(), size, size);

    return string;
}

std::wstring LocalizationManager::getVKReplacement(unsigned int uiVKey) {
    std::wstring replacement = L"";
    switch (uiVKey) {
        case VK_PAD_A:
            replacement = L"ButtonA";
            break;
        case VK_PAD_B:
            replacement = L"ButtonB";
            break;
        case VK_PAD_X:
            replacement = L"ButtonX";
            break;
        case VK_PAD_Y:
            replacement = L"ButtonY";
            break;
        case VK_PAD_LSHOULDER:
            replacement = L"ButtonLeftBumper";
            break;
        case VK_PAD_RSHOULDER:
            replacement = L"ButtonRightBumper";
            break;
        case VK_PAD_LTRIGGER:
            replacement = L"ButtonLeftTrigger";
            break;
        case VK_PAD_RTRIGGER:
            replacement = L"ButtonRightTrigger";
            break;
        case VK_PAD_LTHUMB_UP:
        case VK_PAD_LTHUMB_DOWN:
        case VK_PAD_LTHUMB_RIGHT:
        case VK_PAD_LTHUMB_LEFT:
        case VK_PAD_LTHUMB_UPLEFT:
        case VK_PAD_LTHUMB_UPRIGHT:
        case VK_PAD_LTHUMB_DOWNRIGHT:
        case VK_PAD_LTHUMB_DOWNLEFT:
            replacement = L"ButtonLeftStick";
            break;
        case VK_PAD_RTHUMB_UP:
        case VK_PAD_RTHUMB_DOWN:
        case VK_PAD_RTHUMB_RIGHT:
        case VK_PAD_RTHUMB_LEFT:
        case VK_PAD_RTHUMB_UPLEFT:
        case VK_PAD_RTHUMB_UPRIGHT:
        case VK_PAD_RTHUMB_DOWNRIGHT:
        case VK_PAD_RTHUMB_DOWNLEFT:
            replacement = L"ButtonRightStick";
            break;
        default:
            break;
    }
    wchar_t string[128];

#if defined(_WIN64)
    int size = 45;
    if (ui.getScreenWidth() < 1920) size = 30;
#else
    int size = 45;
#endif

    swprintf(string, 128,
             L"<img src=\"%ls\" align=\"middle\" height=\"%d\" width=\"%d\"/>",
             replacement.c_str(), size, size);

    return string;
}

std::wstring LocalizationManager::getIconReplacement(unsigned int uiIcon) {
    wchar_t string[128];

#if defined(_WIN64)
    int size = 33;
    if (ui.getScreenWidth() < 1920) size = 22;
#else
    int size = 33;
#endif

    swprintf(string, 128,
             L"<img src=\"Icon_Shank\" align=\"middle\" height=\"%d\" "
             L"width=\"%d\"/>",
             size, size);
    std::wstring result = L"";
    switch (uiIcon) {
        case XZP_ICON_SHANK_01:
            result = string;
            break;
        case XZP_ICON_SHANK_03:
            result.append(string).append(string).append(string);
            break;
        default:
            break;
    }
    return result;
}

void LocalizationManager::getLocale(
    std::vector<std::wstring>& vecWstrLocales) {
    std::vector<eMCLang> locales;

    const unsigned int systemLanguage = XGetLanguage();

    switch (systemLanguage) {
        case XC_LANGUAGE_ENGLISH:
            switch (XGetLocale()) {
                case XC_LOCALE_AUSTRALIA:
                case XC_LOCALE_CANADA:
                case XC_LOCALE_CZECH_REPUBLIC:
                case XC_LOCALE_GREECE:
                case XC_LOCALE_HONG_KONG:
                case XC_LOCALE_HUNGARY:
                case XC_LOCALE_INDIA:
                case XC_LOCALE_IRELAND:
                case XC_LOCALE_ISRAEL:
                case XC_LOCALE_NEW_ZEALAND:
                case XC_LOCALE_SAUDI_ARABIA:
                case XC_LOCALE_SINGAPORE:
                case XC_LOCALE_SLOVAK_REPUBLIC:
                case XC_LOCALE_SOUTH_AFRICA:
                case XC_LOCALE_UNITED_ARAB_EMIRATES:
                case XC_LOCALE_GREAT_BRITAIN:
                    locales.push_back(eMCLang_enGB);
                    break;
                default:
                    break;
            }
            break;
        case XC_LANGUAGE_JAPANESE:
            locales.push_back(eMCLang_jaJP);
            break;
        case XC_LANGUAGE_GERMAN:
            switch (XGetLocale()) {
                case XC_LOCALE_AUSTRIA:
                    locales.push_back(eMCLang_deAT);
                    break;
                case XC_LOCALE_SWITZERLAND:
                    locales.push_back(eMCLang_deCH);
                    break;
                default:
                    break;
            }
            locales.push_back(eMCLang_deDE);
            break;
        case XC_LANGUAGE_FRENCH:
            switch (XGetLocale()) {
                case XC_LOCALE_BELGIUM:
                    locales.push_back(eMCLang_frBE);
                    break;
                case XC_LOCALE_CANADA:
                    locales.push_back(eMCLang_frCA);
                    break;
                case XC_LOCALE_SWITZERLAND:
                    locales.push_back(eMCLang_frCH);
                    break;
                default:
                    break;
            }
            locales.push_back(eMCLang_frFR);
            break;
        case XC_LANGUAGE_SPANISH:
            switch (XGetLocale()) {
                case XC_LOCALE_MEXICO:
                case XC_LOCALE_ARGENTINA:
                case XC_LOCALE_CHILE:
                case XC_LOCALE_COLOMBIA:
                case XC_LOCALE_UNITED_STATES:
                case XC_LOCALE_LATIN_AMERICA:
                    locales.push_back(eMCLang_laLAS);
                    locales.push_back(eMCLang_esMX);
                    break;
                default:
                    break;
            }
            locales.push_back(eMCLang_esES);
            break;
        case XC_LANGUAGE_ITALIAN:
            locales.push_back(eMCLang_itIT);
            break;
        case XC_LANGUAGE_KOREAN:
            locales.push_back(eMCLang_koKR);
            break;
        case XC_LANGUAGE_TCHINESE:
            switch (XGetLocale()) {
                case XC_LOCALE_HONG_KONG:
                    locales.push_back(eMCLang_zhHK);
                    locales.push_back(eMCLang_zhTW);
                    break;
                case XC_LOCALE_TAIWAN:
                    locales.push_back(eMCLang_zhTW);
                    locales.push_back(eMCLang_zhHK);
                default:
                    break;
            }
            locales.push_back(eMCLang_hant);
            locales.push_back(eMCLang_zhCHT);
            break;
        case XC_LANGUAGE_PORTUGUESE:
            if (XGetLocale() == XC_LOCALE_BRAZIL) {
                locales.push_back(eMCLang_ptBR);
            }
            locales.push_back(eMCLang_ptPT);
            break;
        case XC_LANGUAGE_POLISH:
            locales.push_back(eMCLang_plPL);
            break;
        case XC_LANGUAGE_RUSSIAN:
            locales.push_back(eMCLang_ruRU);
            break;
        case XC_LANGUAGE_SWEDISH:
            locales.push_back(eMCLang_svSV);
            locales.push_back(eMCLang_svSE);
            break;
        case XC_LANGUAGE_TURKISH:
            locales.push_back(eMCLang_trTR);
            break;
        case XC_LANGUAGE_BNORWEGIAN:
            locales.push_back(eMCLang_nbNO);
            locales.push_back(eMCLang_noNO);
            locales.push_back(eMCLang_nnNO);
            break;
        case XC_LANGUAGE_DUTCH:
            switch (XGetLocale()) {
                case XC_LOCALE_BELGIUM:
                    locales.push_back(eMCLang_nlBE);
                    break;
                default:
                    break;
            }
            locales.push_back(eMCLang_nlNL);
            break;
        case XC_LANGUAGE_SCHINESE:
            switch (XGetLocale()) {
                case XC_LOCALE_SINGAPORE:
                    locales.push_back(eMCLang_zhSG);
                    break;
                default:
                    break;
            }
            locales.push_back(eMCLang_hans);
            locales.push_back(eMCLang_csCS);
            locales.push_back(eMCLang_zhCN);
            break;
    }

    locales.push_back(eMCLang_enUS);
    locales.push_back(eMCLang_null);

    for (int i = 0; i < locales.size(); i++) {
        eMCLang lang = locales.at(i);
        vecWstrLocales.push_back(m_localeA[lang]);
    }
}

int LocalizationManager::get_eMCLang(wchar_t* pwchLocale) {
    return m_eMCLangA[pwchLocale];
}

int LocalizationManager::get_xcLang(wchar_t* pwchLocale) {
    return m_xcLangA[pwchLocale];
}

void LocalizationManager::localeAndLanguageInit() {
    m_localeA[eMCLang_zhCHT] = L"zh-CHT";
    m_localeA[eMCLang_csCS] = L"cs-CS";
    m_localeA[eMCLang_laLAS] = L"la-LAS";
    m_localeA[eMCLang_null] = L"en-EN";
    m_localeA[eMCLang_enUS] = L"en-US";
    m_localeA[eMCLang_enGB] = L"en-GB";
    m_localeA[eMCLang_enIE] = L"en-IE";
    m_localeA[eMCLang_enAU] = L"en-AU";
    m_localeA[eMCLang_enNZ] = L"en-NZ";
    m_localeA[eMCLang_enCA] = L"en-CA";
    m_localeA[eMCLang_jaJP] = L"ja-JP";
    m_localeA[eMCLang_deDE] = L"de-DE";
    m_localeA[eMCLang_deAT] = L"de-AT";
    m_localeA[eMCLang_frFR] = L"fr-FR";
    m_localeA[eMCLang_frCA] = L"fr-CA";
    m_localeA[eMCLang_esES] = L"es-ES";
    m_localeA[eMCLang_esMX] = L"es-MX";
    m_localeA[eMCLang_itIT] = L"it-IT";
    m_localeA[eMCLang_koKR] = L"ko-KR";
    m_localeA[eMCLang_ptPT] = L"pt-PT";
    m_localeA[eMCLang_ptBR] = L"pt-BR";
    m_localeA[eMCLang_ruRU] = L"ru-RU";
    m_localeA[eMCLang_nlNL] = L"nl-NL";
    m_localeA[eMCLang_fiFI] = L"fi-FI";
    m_localeA[eMCLang_svSV] = L"sv-SV";
    m_localeA[eMCLang_daDA] = L"da-DA";
    m_localeA[eMCLang_noNO] = L"no-NO";
    m_localeA[eMCLang_plPL] = L"pl-PL";
    m_localeA[eMCLang_trTR] = L"tr-TR";
    m_localeA[eMCLang_elEL] = L"el-EL";
    m_localeA[eMCLang_zhSG] = L"zh-SG";
    m_localeA[eMCLang_zhCN] = L"zh-CN";
    m_localeA[eMCLang_zhHK] = L"zh-HK";
    m_localeA[eMCLang_zhTW] = L"zh-TW";
    m_localeA[eMCLang_nlBE] = L"nl-BE";
    m_localeA[eMCLang_daDK] = L"da-DK";
    m_localeA[eMCLang_frBE] = L"fr-BE";
    m_localeA[eMCLang_frCH] = L"fr-CH";
    m_localeA[eMCLang_deCH] = L"de-CH";
    m_localeA[eMCLang_nbNO] = L"nb-NO";
    m_localeA[eMCLang_enGR] = L"en-GR";
    m_localeA[eMCLang_enHK] = L"en-HK";
    m_localeA[eMCLang_enSA] = L"en-SA";
    m_localeA[eMCLang_enHU] = L"en-HU";
    m_localeA[eMCLang_enIN] = L"en-IN";
    m_localeA[eMCLang_enIL] = L"en-IL";
    m_localeA[eMCLang_enSG] = L"en-SG";
    m_localeA[eMCLang_enSK] = L"en-SK";
    m_localeA[eMCLang_enZA] = L"en-ZA";
    m_localeA[eMCLang_enCZ] = L"en-CZ";
    m_localeA[eMCLang_enAE] = L"en-AE";
    m_localeA[eMCLang_esAR] = L"es-AR";
    m_localeA[eMCLang_esCL] = L"es-CL";
    m_localeA[eMCLang_esCO] = L"es-CO";
    m_localeA[eMCLang_esUS] = L"es-US";
    m_localeA[eMCLang_svSE] = L"sv-SE";
    m_localeA[eMCLang_csCZ] = L"cs-CZ";
    m_localeA[eMCLang_elGR] = L"el-GR";
    m_localeA[eMCLang_nnNO] = L"nn-NO";
    m_localeA[eMCLang_skSK] = L"sk-SK";
    m_localeA[eMCLang_hans] = L"zh-HANS";
    m_localeA[eMCLang_hant] = L"zh-HANT";

    m_eMCLangA[L"zh-CHT"] = eMCLang_zhCHT;
    m_eMCLangA[L"cs-CS"] = eMCLang_csCS;
    m_eMCLangA[L"la-LAS"] = eMCLang_laLAS;
    m_eMCLangA[L"en-EN"] = eMCLang_null;
    m_eMCLangA[L"en-US"] = eMCLang_enUS;
    m_eMCLangA[L"en-GB"] = eMCLang_enGB;
    m_eMCLangA[L"en-IE"] = eMCLang_enIE;
    m_eMCLangA[L"en-AU"] = eMCLang_enAU;
    m_eMCLangA[L"en-NZ"] = eMCLang_enNZ;
    m_eMCLangA[L"en-CA"] = eMCLang_enCA;
    m_eMCLangA[L"ja-JP"] = eMCLang_jaJP;
    m_eMCLangA[L"de-DE"] = eMCLang_deDE;
    m_eMCLangA[L"de-AT"] = eMCLang_deAT;
    m_eMCLangA[L"fr-FR"] = eMCLang_frFR;
    m_eMCLangA[L"fr-CA"] = eMCLang_frCA;
    m_eMCLangA[L"es-ES"] = eMCLang_esES;
    m_eMCLangA[L"es-MX"] = eMCLang_esMX;
    m_eMCLangA[L"it-IT"] = eMCLang_itIT;
    m_eMCLangA[L"ko-KR"] = eMCLang_koKR;
    m_eMCLangA[L"pt-PT"] = eMCLang_ptPT;
    m_eMCLangA[L"pt-BR"] = eMCLang_ptBR;
    m_eMCLangA[L"ru-RU"] = eMCLang_ruRU;
    m_eMCLangA[L"nl-NL"] = eMCLang_nlNL;
    m_eMCLangA[L"fi-FI"] = eMCLang_fiFI;
    m_eMCLangA[L"sv-SV"] = eMCLang_svSV;
    m_eMCLangA[L"da-DA"] = eMCLang_daDA;
    m_eMCLangA[L"no-NO"] = eMCLang_noNO;
    m_eMCLangA[L"pl-PL"] = eMCLang_plPL;
    m_eMCLangA[L"tr-TR"] = eMCLang_trTR;
    m_eMCLangA[L"el-EL"] = eMCLang_elEL;
    m_eMCLangA[L"zh-SG"] = eMCLang_zhSG;
    m_eMCLangA[L"zh-CN"] = eMCLang_zhCN;
    m_eMCLangA[L"zh-HK"] = eMCLang_zhHK;
    m_eMCLangA[L"zh-TW"] = eMCLang_zhTW;
    m_eMCLangA[L"nl-BE"] = eMCLang_nlBE;
    m_eMCLangA[L"da-DK"] = eMCLang_daDK;
    m_eMCLangA[L"fr-BE"] = eMCLang_frBE;
    m_eMCLangA[L"fr-CH"] = eMCLang_frCH;
    m_eMCLangA[L"de-CH"] = eMCLang_deCH;
    m_eMCLangA[L"nb-NO"] = eMCLang_nbNO;
    m_eMCLangA[L"en-GR"] = eMCLang_enGR;
    m_eMCLangA[L"en-HK"] = eMCLang_enHK;
    m_eMCLangA[L"en-SA"] = eMCLang_enSA;
    m_eMCLangA[L"en-HU"] = eMCLang_enHU;
    m_eMCLangA[L"en-IN"] = eMCLang_enIN;
    m_eMCLangA[L"en-IL"] = eMCLang_enIL;
    m_eMCLangA[L"en-SG"] = eMCLang_enSG;
    m_eMCLangA[L"en-SK"] = eMCLang_enSK;
    m_eMCLangA[L"en-ZA"] = eMCLang_enZA;
    m_eMCLangA[L"en-CZ"] = eMCLang_enCZ;
    m_eMCLangA[L"en-AE"] = eMCLang_enAE;
    m_eMCLangA[L"es-AR"] = eMCLang_esAR;
    m_eMCLangA[L"es-CL"] = eMCLang_esCL;
    m_eMCLangA[L"es-CO"] = eMCLang_esCO;
    m_eMCLangA[L"es-US"] = eMCLang_esUS;
    m_eMCLangA[L"sv-SE"] = eMCLang_svSE;
    m_eMCLangA[L"cs-CZ"] = eMCLang_csCZ;
    m_eMCLangA[L"el-GR"] = eMCLang_elGR;
    m_eMCLangA[L"nn-NO"] = eMCLang_nnNO;
    m_eMCLangA[L"sk-SK"] = eMCLang_skSK;
    m_eMCLangA[L"zh-HANS"] = eMCLang_hans;
    m_eMCLangA[L"zh-HANT"] = eMCLang_hant;

    m_xcLangA[L"zh-CHT"] = XC_LOCALE_CHINA;
    m_xcLangA[L"cs-CS"] = XC_LOCALE_CHINA;
    m_xcLangA[L"en-EN"] = XC_LOCALE_UNITED_STATES;
    m_xcLangA[L"en-US"] = XC_LOCALE_UNITED_STATES;
    m_xcLangA[L"en-GB"] = XC_LOCALE_GREAT_BRITAIN;
    m_xcLangA[L"en-IE"] = XC_LOCALE_IRELAND;
    m_xcLangA[L"en-AU"] = XC_LOCALE_AUSTRALIA;
    m_xcLangA[L"en-NZ"] = XC_LOCALE_NEW_ZEALAND;
    m_xcLangA[L"en-CA"] = XC_LOCALE_CANADA;
    m_xcLangA[L"ja-JP"] = XC_LOCALE_JAPAN;
    m_xcLangA[L"de-DE"] = XC_LOCALE_GERMANY;
    m_xcLangA[L"de-AT"] = XC_LOCALE_AUSTRIA;
    m_xcLangA[L"fr-FR"] = XC_LOCALE_FRANCE;
    m_xcLangA[L"fr-CA"] = XC_LOCALE_CANADA;
    m_xcLangA[L"es-ES"] = XC_LOCALE_SPAIN;
    m_xcLangA[L"es-MX"] = XC_LOCALE_MEXICO;
    m_xcLangA[L"it-IT"] = XC_LOCALE_ITALY;
    m_xcLangA[L"ko-KR"] = XC_LOCALE_KOREA;
    m_xcLangA[L"pt-PT"] = XC_LOCALE_PORTUGAL;
    m_xcLangA[L"pt-BR"] = XC_LOCALE_BRAZIL;
    m_xcLangA[L"ru-RU"] = XC_LOCALE_RUSSIAN_FEDERATION;
    m_xcLangA[L"nl-NL"] = XC_LOCALE_NETHERLANDS;
    m_xcLangA[L"fi-FI"] = XC_LOCALE_FINLAND;
    m_xcLangA[L"sv-SV"] = XC_LOCALE_SWEDEN;
    m_xcLangA[L"da-DA"] = XC_LOCALE_DENMARK;
    m_xcLangA[L"no-NO"] = XC_LOCALE_NORWAY;
    m_xcLangA[L"pl-PL"] = XC_LOCALE_POLAND;
    m_xcLangA[L"tr-TR"] = XC_LOCALE_TURKEY;
    m_xcLangA[L"el-EL"] = XC_LOCALE_GREECE;
    m_xcLangA[L"la-LAS"] = XC_LOCALE_LATIN_AMERICA;
    m_xcLangA[L"zh-SG"] = XC_LOCALE_SINGAPORE;
    m_xcLangA[L"Zh-CN"] = XC_LOCALE_CHINA;
    m_xcLangA[L"zh-HK"] = XC_LOCALE_HONG_KONG;
    m_xcLangA[L"zh-TW"] = XC_LOCALE_TAIWAN;
    m_xcLangA[L"nl-BE"] = XC_LOCALE_BELGIUM;
    m_xcLangA[L"da-DK"] = XC_LOCALE_DENMARK;
    m_xcLangA[L"fr-BE"] = XC_LOCALE_BELGIUM;
    m_xcLangA[L"fr-CH"] = XC_LOCALE_SWITZERLAND;
    m_xcLangA[L"de-CH"] = XC_LOCALE_SWITZERLAND;
    m_xcLangA[L"nb-NO"] = XC_LOCALE_NORWAY;
    m_xcLangA[L"en-GR"] = XC_LOCALE_GREECE;
    m_xcLangA[L"en-HK"] = XC_LOCALE_HONG_KONG;
    m_xcLangA[L"en-SA"] = XC_LOCALE_SAUDI_ARABIA;
    m_xcLangA[L"en-HU"] = XC_LOCALE_HUNGARY;
    m_xcLangA[L"en-IN"] = XC_LOCALE_INDIA;
    m_xcLangA[L"en-IL"] = XC_LOCALE_ISRAEL;
    m_xcLangA[L"en-SG"] = XC_LOCALE_SINGAPORE;
    m_xcLangA[L"en-SK"] = XC_LOCALE_SLOVAK_REPUBLIC;
    m_xcLangA[L"en-ZA"] = XC_LOCALE_SOUTH_AFRICA;
    m_xcLangA[L"en-CZ"] = XC_LOCALE_CZECH_REPUBLIC;
    m_xcLangA[L"en-AE"] = XC_LOCALE_UNITED_ARAB_EMIRATES;
    m_xcLangA[L"ja-IP"] = XC_LOCALE_JAPAN;
    m_xcLangA[L"es-AR"] = XC_LOCALE_ARGENTINA;
    m_xcLangA[L"es-CL"] = XC_LOCALE_CHILE;
    m_xcLangA[L"es-CO"] = XC_LOCALE_COLOMBIA;
    m_xcLangA[L"es-US"] = XC_LOCALE_UNITED_STATES;
    m_xcLangA[L"sv-SE"] = XC_LOCALE_SWEDEN;
    m_xcLangA[L"cs-CZ"] = XC_LOCALE_CZECH_REPUBLIC;
    m_xcLangA[L"el-GR"] = XC_LOCALE_GREECE;
    m_xcLangA[L"sk-SK"] = XC_LOCALE_SLOVAK_REPUBLIC;
    m_xcLangA[L"zh-HANS"] = XC_LOCALE_CHINA;
    m_xcLangA[L"zh-HANT"] = XC_LOCALE_CHINA;
}
