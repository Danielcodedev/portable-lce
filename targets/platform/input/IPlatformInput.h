#pragma once

#include <functional>

#include "PlatformTypes.h"

#define MAP_STYLE_0 0
#define MAP_STYLE_1 1
#define MAP_STYLE_2 2

#define _360_JOY_BUTTON_A 0x00000001
#define _360_JOY_BUTTON_B 0x00000002
#define _360_JOY_BUTTON_X 0x00000004
#define _360_JOY_BUTTON_Y 0x00000008

#define _360_JOY_BUTTON_START 0x00000010
#define _360_JOY_BUTTON_BACK 0x00000020
#define _360_JOY_BUTTON_RB 0x00000040
#define _360_JOY_BUTTON_LB 0x00000080

#define _360_JOY_BUTTON_RTHUMB 0x00000100
#define _360_JOY_BUTTON_LTHUMB 0x00000200
#define _360_JOY_BUTTON_DPAD_UP 0x00000400
#define _360_JOY_BUTTON_DPAD_DOWN 0x00000800

#define _360_JOY_BUTTON_DPAD_LEFT 0x00001000
#define _360_JOY_BUTTON_DPAD_RIGHT 0x00002000
// fake digital versions of analog values
#define _360_JOY_BUTTON_LSTICK_RIGHT 0x00004000
#define _360_JOY_BUTTON_LSTICK_LEFT 0x00008000

#define _360_JOY_BUTTON_RSTICK_DOWN 0x00010000
#define _360_JOY_BUTTON_RSTICK_UP 0x00020000
#define _360_JOY_BUTTON_RSTICK_RIGHT 0x00040000
#define _360_JOY_BUTTON_RSTICK_LEFT 0x00080000

#define _360_JOY_BUTTON_LSTICK_DOWN 0x00100000
#define _360_JOY_BUTTON_LSTICK_UP 0x00200000
#define _360_JOY_BUTTON_RT 0x00400000
#define _360_JOY_BUTTON_LT 0x00800000

// Stick axis maps - to allow changes for SouthPaw in-game axis mapping
#define AXIS_MAP_LX 0
#define AXIS_MAP_LY 1
#define AXIS_MAP_RX 2
#define AXIS_MAP_RY 3

// Trigger map - to allow for swap triggers in-game
#define TRIGGER_MAP_0 0
#define TRIGGER_MAP_1 1

class IPlatformInput {
public:
    enum EKeyboardMode {
        EKeyboardMode_Default,
        EKeyboardMode_Numeric,
        EKeyboardMode_Password,
        EKeyboardMode_Alphabet,
        EKeyboardMode_Full,
        EKeyboardMode_Alphabet_Extended,
        EKeyboardMode_IP_Address,
        EKeyboardMode_Phone
    };

    virtual ~IPlatformInput() = default;

    virtual void Initialise(int iInputStateC, unsigned char ucMapC,
                            unsigned char ucActionC,
                            unsigned char ucMenuActionC) = 0;
    virtual void Tick() = 0;
    virtual void SetDeadzoneAndMovementRange(
        unsigned int uiDeadzone, unsigned int uiMovementRangeMax) = 0;

    // Joypad mapping
    virtual void SetGameJoypadMaps(unsigned char ucMap, unsigned char ucAction,
                                   unsigned int uiActionVal) = 0;
    [[nodiscard]] virtual unsigned int GetGameJoypadMaps(
        unsigned char ucMap, unsigned char ucAction) = 0;
    virtual void SetJoypadMapVal(int iPad, unsigned char ucMap) = 0;
    [[nodiscard]] virtual unsigned char GetJoypadMapVal(int iPad) = 0;
    virtual void SetJoypadSensitivity(int iPad, float fSensitivity) = 0;

    // Input queries
    [[nodiscard]] virtual unsigned int GetValue(int iPad,
                                                unsigned char ucAction,
                                                bool bRepeat = false) = 0;
    [[nodiscard]] virtual bool ButtonPressed(int iPad,
                                             unsigned char ucAction = 255) = 0;
    [[nodiscard]] virtual bool ButtonReleased(int iPad,
                                              unsigned char ucAction) = 0;
    [[nodiscard]] virtual bool ButtonDown(int iPad,
                                          unsigned char ucAction = 255) = 0;

    // Axis remapping
    virtual void SetJoypadStickAxisMap(int iPad, unsigned int uiFrom,
                                       unsigned int uiTo) = 0;
    virtual void SetJoypadStickTriggerMap(int iPad, unsigned int uiFrom,
                                          unsigned int uiTo) = 0;
    virtual void SetKeyRepeatRate(float fRepeatDelaySecs,
                                  float fRepeatRateSecs) = 0;
    virtual void SetDebugSequence(const char* chSequenceA,
                                  std::function<int()> callback) = 0;
    [[nodiscard]] virtual float GetIdleSeconds(int iPad) = 0;
    [[nodiscard]] virtual bool IsPadConnected(int iPad) = 0;

    // Analog sticks and triggers (remapped for Southpaw etc.)
    [[nodiscard]] virtual float GetJoypadStick_LX(
        int iPad, bool bCheckMenuDisplay = true) = 0;
    [[nodiscard]] virtual float GetJoypadStick_LY(
        int iPad, bool bCheckMenuDisplay = true) = 0;
    [[nodiscard]] virtual float GetJoypadStick_RX(
        int iPad, bool bCheckMenuDisplay = true) = 0;
    [[nodiscard]] virtual float GetJoypadStick_RY(
        int iPad, bool bCheckMenuDisplay = true) = 0;
    [[nodiscard]] virtual unsigned char GetJoypadLTrigger(
        int iPad, bool bCheckMenuDisplay = true) = 0;
    [[nodiscard]] virtual unsigned char GetJoypadRTrigger(
        int iPad, bool bCheckMenuDisplay = true) = 0;

    // Menu and misc
    virtual void SetMenuDisplayed(int iPad, bool bVal) = 0;
    [[nodiscard]] virtual bool GetMenuDisplayed(int iPad) = 0;
    [[nodiscard]] virtual int GetHotbarSlotPressed(int iPad) = 0;
    [[nodiscard]] virtual int GetScrollDelta() = 0;

    // Keyboard
    virtual EKeyboardResult RequestKeyboard(const wchar_t* Title,
                                            const wchar_t* Text, int iPad,
                                            unsigned int uiMaxChars,
                                            std::function<int(bool)> callback,
                                            EKeyboardMode eMode) = 0;
    [[nodiscard]] virtual const char* GetText() = 0;

    // String verification (TCR 92)
    virtual bool VerifyStrings(
        wchar_t** pwStringA, int iStringC,
        std::function<int(STRING_VERIFY_RESPONSE*)> callback) = 0;
    virtual void CancelQueuedVerifyStrings(
        std::function<int(STRING_VERIFY_RESPONSE*)> callback) = 0;
    virtual void CancelAllVerifyInProgress() = 0;

    // Mouse
    [[nodiscard]] virtual int GetMouseX() = 0;
    [[nodiscard]] virtual int GetMouseY() = 0;

    // Primary pad (which controller is player 1)
    [[nodiscard]] virtual int GetPrimaryPad() = 0;
    virtual void SetPrimaryPad(int iPad) = 0;
};
