// HARFANG(R). Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#pragma once

#include "platform/input_system.h"

#ifdef __OBJC__
@class GCExtendedGamepad;
#endif

namespace hg {

// Keep GLFW readers for legacy HID devices, but prefer Apple's live controller
// state for the default reader. Apple-managed Bluetooth devices can expose a
// recognizable HID device whose values never change through GLFW/IOKit.
void InitGameControllerInput(GamepadReader fallback);
void ShutdownGameControllerInput();

#ifdef __OBJC__
GamepadState ReadGameControllerProfile(GCExtendedGamepad *profile);
#endif

} // namespace hg
