// HARFANG(R). Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#define TEST_NO_MAIN
#include "acutest.h"
#include "platform/osx/gamecontroller.h"

#import <GameController/GameController.h>

using namespace hg;

void test_gamecontroller() {
	@autoreleasepool {
		const auto disconnected = ReadGameControllerProfile(nil);
		TEST_CHECK(!disconnected.connected);
		TEST_CHECK(disconnected.button.none());
		TEST_CHECK(disconnected.axes[GA_LeftTrigger] == -1.f);
		TEST_CHECK(disconnected.axes[GA_RightTrigger] == -1.f);

		if (@available(macOS 10.15, *)) {
			GCController *controller = [GCController controllerWithExtendedGamepad];
			GCExtendedGamepad *profile = controller.extendedGamepad;
			TEST_ASSERT(profile != nil);
			const auto idle = ReadGameControllerProfile(profile);
			TEST_CHECK(idle.connected);
			TEST_CHECK(idle.button.none());
			TEST_CHECK(idle.axes[GA_LeftTrigger] == -1.f);

			// A native Circle/B press must produce a fresh state without mutating
			// the previous frame, since Lua navigation detects rising edges.
			[profile.buttonB setValue:1.f];
			const auto pressed = ReadGameControllerProfile(profile);
			TEST_CHECK(!idle.button[GB_ButtonB]);
			TEST_CHECK(pressed.button[GB_ButtonB]);
			TEST_CHECK(pressed.button.count() == 1);
			[profile.buttonB setValue:0.f];
			TEST_CHECK(!ReadGameControllerProfile(profile).button[GB_ButtonB]);

			[profile.leftThumbstick setValueForXAxis:0.5f yAxis:0.75f];
			[profile.rightThumbstick setValueForXAxis:-0.5f yAxis:-0.75f];
			[profile.leftTrigger setValue:0.25f];
			[profile.rightTrigger setValue:1.f];
			[profile.buttonX setValue:1.f];
			[profile.dpad setValueForXAxis:-1.f yAxis:1.f];
			const auto moved = ReadGameControllerProfile(profile);
			TEST_CHECK(moved.axes[GA_LeftX] == 0.5f);
			TEST_CHECK(moved.axes[GA_LeftY] == -0.75f);
			TEST_CHECK(moved.axes[GA_RightX] == -0.5f);
			TEST_CHECK(moved.axes[GA_RightY] == 0.75f);
			TEST_CHECK(moved.axes[GA_LeftTrigger] == -0.5f);
			TEST_CHECK(moved.axes[GA_RightTrigger] == 1.f);
			TEST_CHECK(moved.button[GB_ButtonX]);
			TEST_CHECK(moved.button[GB_DPadLeft]);
			TEST_CHECK(moved.button[GB_DPadUp]);
			TEST_CHECK(!moved.button[GB_DPadRight]);
			TEST_CHECK(!moved.button[GB_DPadDown]);
		}
	}
}
