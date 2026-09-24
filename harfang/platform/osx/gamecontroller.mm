// HARFANG(R). Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include "platform/osx/gamecontroller.h"

#import <Foundation/Foundation.h>
#import <GameController/GameController.h>

#include <string>

namespace hg {

static constexpr int slot_count = 16;
static NSMutableArray *controller_slots;
static GamepadReader fallback_reader;
static bool previous_background_events;

GamepadState ReadGameControllerProfile(GCExtendedGamepad *profile) {
	GamepadState state{};
	state.axes[GA_LeftTrigger] = state.axes[GA_RightTrigger] = -1.f;
	if (!profile)
		return state;

	state.connected = true;
	state.axes[GA_LeftX] = profile.leftThumbstick.xAxis.value;
	state.axes[GA_LeftY] = -profile.leftThumbstick.yAxis.value;
	state.axes[GA_RightX] = profile.rightThumbstick.xAxis.value;
	state.axes[GA_RightY] = -profile.rightThumbstick.yAxis.value;
	// Preserve GLFW's down-positive Y axes and [-1, 1] trigger range.
	state.axes[GA_LeftTrigger] = profile.leftTrigger.value * 2.f - 1.f;
	state.axes[GA_RightTrigger] = profile.rightTrigger.value * 2.f - 1.f;
	state.button[GB_ButtonA] = profile.buttonA.isPressed;
	state.button[GB_ButtonB] = profile.buttonB.isPressed;
	state.button[GB_ButtonX] = profile.buttonX.isPressed;
	state.button[GB_ButtonY] = profile.buttonY.isPressed;
	state.button[GB_LeftBumper] = profile.leftShoulder.isPressed;
	state.button[GB_RightBumper] = profile.rightShoulder.isPressed;
	state.button[GB_DPadUp] = profile.dpad.up.isPressed;
	state.button[GB_DPadRight] = profile.dpad.right.isPressed;
	state.button[GB_DPadDown] = profile.dpad.down.isPressed;
	state.button[GB_DPadLeft] = profile.dpad.left.isPressed;
	if (@available(macOS 10.14.1, *)) {
		state.button[GB_LeftThumb] = profile.leftThumbstickButton.isPressed;
		state.button[GB_RightThumb] = profile.rightThumbstickButton.isPressed;
	}
	if (@available(macOS 10.15, *)) {
		state.button[GB_Back] = profile.buttonOptions.isPressed;
		state.button[GB_Start] = profile.buttonMenu.isPressed;
	}
	if (@available(macOS 11.0, *))
		state.button[GB_Guide] = profile.buttonHome.isPressed;
	return state;
}

// Called on the input/window thread, like the GLFW readers. Keep device slots
// stable when another controller connects or disconnects.
static void RefreshControllerSlots() {
	NSArray<GCController *> *connected = GCController.controllers;
	for (int i = 0; i < slot_count; ++i) {
		id controller = controller_slots[i];
		if (controller != NSNull.null && ![connected containsObject:controller])
			controller_slots[i] = NSNull.null;
	}
	for (GCController *controller in connected) {
		if (!controller.extendedGamepad || [controller_slots containsObject:controller])
			continue;
		const NSUInteger empty = [controller_slots indexOfObject:NSNull.null];
		if (empty != NSNotFound)
			controller_slots[empty] = controller;
	}
}

static GamepadState ReadControllerSlot(int slot) {
	id controller = controller_slots[slot];
	if (controller == NSNull.null)
		return ReadGameControllerProfile(nil);
	return ReadGameControllerProfile([(GCController *)controller extendedGamepad]);
}

template <int Slot> static GamepadState ReadNativeGamepad() {
	@autoreleasepool {
		RefreshControllerSlots();
		return ReadControllerSlot(Slot);
	}
}

static GamepadState ReadDefaultGamepad() {
	@autoreleasepool {
		RefreshControllerSlots();
		for (int i = 0; i < slot_count; ++i) {
			GamepadState state = ReadControllerSlot(i);
			if (state.connected)
				return state;
		}
		return fallback_reader ? fallback_reader() : GamepadState{};
	}
}

static const GamepadReader native_readers[] = {
	ReadNativeGamepad<0>, ReadNativeGamepad<1>, ReadNativeGamepad<2>, ReadNativeGamepad<3>,
	ReadNativeGamepad<4>, ReadNativeGamepad<5>, ReadNativeGamepad<6>, ReadNativeGamepad<7>,
	ReadNativeGamepad<8>, ReadNativeGamepad<9>, ReadNativeGamepad<10>, ReadNativeGamepad<11>,
	ReadNativeGamepad<12>, ReadNativeGamepad<13>, ReadNativeGamepad<14>, ReadNativeGamepad<15>
};

void InitGameControllerInput(GamepadReader fallback) {
	if (controller_slots)
		return;
	@autoreleasepool {
		controller_slots = [NSMutableArray arrayWithCapacity:slot_count];
		for (int i = 0; i < slot_count; ++i) {
			[controller_slots addObject:NSNull.null];
			AddGamepadReader(("gamecontroller_slot_" + std::to_string(i)).c_str(), native_readers[i]);
		}
		fallback_reader = fallback;
		// GLFW gamepad polling works independently of window focus. Keep that
		// behavior for the native reader and restore Apple's setting on shutdown.
		if (@available(macOS 11.3, *)) {
			previous_background_events = GCController.shouldMonitorBackgroundEvents;
			GCController.shouldMonitorBackgroundEvents = YES;
		}
		AddGamepadReader("default", ReadDefaultGamepad);
		RefreshControllerSlots();
	}
}

void ShutdownGameControllerInput() {
	if (!controller_slots)
		return;
	for (int i = 0; i < slot_count; ++i)
		RemoveGamepadReader(("gamecontroller_slot_" + std::to_string(i)).c_str());
	if (fallback_reader)
		AddGamepadReader("default", fallback_reader);
	else
		RemoveGamepadReader("default");
	if (@available(macOS 11.3, *))
		GCController.shouldMonitorBackgroundEvents = previous_background_events;
	controller_slots = nil;
	fallback_reader = nullptr;
}

} // namespace hg
