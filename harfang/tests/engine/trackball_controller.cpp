// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "engine/trackball_controller.h"
#include "platform/input_system.h"

#include <cmath>

using namespace hg;

static MouseState trackball_mouse_state{};
static MouseState ReadTrackballMouse() { return trackball_mouse_state; }

void test_trackball_controller() {
	const float epsilon = 1.e-5f;
	{
		TrackballController camera(Vec3::Zero, 3.4f);
		camera.Update(true, false, 150, 100, 50, 0, 0, 200, 200);
		const Mat4 world = camera.ComputeWorld();
		TEST_CHECK(AlmostEqual(GetZ(world), Vec3(0.5f, 0, Sqrt(0.75f)), epsilon));
		TEST_CHECK(AlmostEqual(GetT(world), Vec3(-1.7f, 0, -3.4f * Sqrt(0.75f)), epsilon));
		camera.Update(true, false, 100, 100, -50, 0, 0, 200, 200);
		TEST_CHECK(AlmostEqual(GetT(camera.ComputeWorld()), Vec3(0, 0, -3.4f), epsilon));

		// Opposite points and empty viewports must not produce invalid orientations.
		camera.Update(true, false, 200, 100, 200, 0, 0, 200, 200);
		TEST_CHECK(AlmostEqual(GetZ(camera.ComputeWorld()), Vec3::Front, epsilon));
		camera.Update(true, true, 150, 100, 50, 0, 1000, 0, 200);
		TEST_CHECK(AlmostEqual(GetT(camera.ComputeWorld()), Vec3(0, 0, -3.4f), epsilon));
		camera.Update(true, true, 150, 100, 50, 0, 1000, 200, 0);
		TEST_CHECK(AlmostEqual(GetT(camera.ComputeWorld()), Vec3(0, 0, -3.4f), epsilon));

		camera.Update(false, false, 0, 0, 0, 0, 1000, 200, 200);
		TEST_CHECK(Equal(camera.distance, camera.min_distance));
		camera.Update(false, false, 0, 0, 0, 0, -1000, 200, 200);
		TEST_CHECK(Equal(camera.distance, camera.max_distance));
		camera.Reset();
		TEST_CHECK(Equal(camera.distance, 3.4f));

		// Wheel is applied before pan, which uses the vertical FOV and viewport height.
		camera.Update(false, true, 0, 0, 10, -5, 1, 200, 200, Pi / 2.f);
		const float distance = 3.4f * std::exp(-0.12f);
		TEST_CHECK(AlmostEqual(camera.target, Vec3(-10, 5, 0) * (2.f * distance / 200.f), epsilon));
		camera.Reset();
		TEST_CHECK(AlmostEqual(camera.target, Vec3::Zero, epsilon));
	}
	{
		// Projection outside the sphere and long sequences must preserve an orthonormal basis, including roll.
		TrackballController camera;
		camera.Update(true, false, 300, 200, 25, 30, 0, 200, 200);
		TEST_CHECK(Equal(Len(camera.orientation), 1.f));
		for (int i = 0; i < 10000; ++i)
			camera.Update(true, false, 140, 130, 3, -2, 0, 200, 200);
		const Mat4 world = camera.ComputeWorld();
		const Vec3 right = GetX(world), up = GetY(world), forward = GetZ(world);
		TEST_CHECK(AlmostEqual(Cross(right, up), forward, epsilon));
		TEST_CHECK(Equal(Len(right), 1.f) && Equal(Len(up), 1.f) && Equal(Len(forward), 1.f));
		TEST_CHECK(Abs(Dot(right, up)) < epsilon && Abs(Dot(right, forward)) < epsilon && Abs(Dot(up, forward)) < epsilon);
	}
	{
		// Reset remembers the supplied pose, even when the range is configured after construction.
		const Vec3 target(2, 3, 4);
		const Quaternion orientation = QuaternionFromEuler(0.18f, 0.4f, 0.f, RO_YXZ);
		TrackballController camera(target, 100.f, orientation * 2.f);
		camera.max_distance = 1000.f;
		camera.Update(true, true, 140, 130, 3, -2, 2, 200, 200);
		camera.Reset();
		TEST_CHECK(Equal(camera.distance, 100.f));
		TEST_CHECK(AlmostEqual(camera.target, target, epsilon));
		TEST_CHECK(AlmostEqual(GetZ(camera.ComputeWorld()), Rotate(orientation, Vec3::Front), epsilon));
		TrackballController copy(camera);
		copy.target = Vec3::Zero;
		TEST_CHECK(AlmostEqual(camera.target, target, epsilon));
		copy.Reset();
		TEST_CHECK(AlmostEqual(copy.target, target, epsilon));
	}
	{
		// High-resolution wheel ticks must move on every frame, not only above 1.
		AddMouseReader("trackball-fractional", ReadTrackballMouse);
		Mouse mouse("trackball-fractional");
		TrackballController camera(Vec3::Zero, 0.85f);
		camera.max_distance = 2.f;
		trackball_mouse_state = {};
		trackball_mouse_state.wheel_delta = 0.125f;
		trackball_mouse_state.hwheel_delta = -0.25f;
		for (int tick = 1; tick <= 16; ++tick) {
			const float previous = camera.distance;
			mouse.Update();
			TEST_CHECK(mouse.Wheel() == 0 && mouse.HWheel() == 0);
			TEST_CHECK(mouse.WheelDelta() == 0.125f && mouse.HWheelDelta() == -0.25f);
			camera.Update(mouse, 200, 200);
			TEST_CHECK(camera.distance < previous);
			TEST_CHECK(Equal(camera.distance, 0.85f * std::exp(-0.12f * 0.125f * tick)));
		}
		trackball_mouse_state.wheel_delta = -1000.f;
		mouse.Update();
		camera.Update(mouse, 200, 200);
		TEST_CHECK(camera.distance == 2.f);
		trackball_mouse_state = {};
		trackball_mouse_state.wheel = 1;
		trackball_mouse_state.hwheel = -2;
		mouse.Update();
		TEST_CHECK(mouse.Wheel() == 1 && mouse.WheelDelta() == 1.f);
		TEST_CHECK(mouse.HWheel() == -2 && mouse.HWheelDelta() == -2.f);
		camera.Update(mouse, 200, 200);
		TEST_CHECK(Equal(camera.distance, 2.f * std::exp(-0.12f)));
		RemoveMouseReader("trackball-fractional");
	}
	{
		// Synthetic mouse input exercises first-press and focus reacquisition without window/input initialization.
		AddMouseReader("trackball-test", ReadTrackballMouse);
		Mouse mouse("trackball-test");
		TrackballController camera;
		trackball_mouse_state = {};
		trackball_mouse_state.y = 100;
		mouse.Update();
		trackball_mouse_state.x = 100;
		trackball_mouse_state.button[MB_0] = true;
		trackball_mouse_state.wheel = 1;
		mouse.Update();
		camera.Update(mouse, 200, 200);
		TEST_CHECK(AlmostEqual(GetZ(camera.ComputeWorld()), Vec3::Front, epsilon));
		TEST_CHECK(Equal(camera.distance, 3.4f * std::exp(-0.12f)));

		trackball_mouse_state.x = 150;
		trackball_mouse_state.wheel = 0;
		mouse.Update();
		camera.Update(mouse, 200, 200);
		const Mat4 dragged = camera.ComputeWorld();
		TEST_CHECK(AlmostEqual(GetZ(dragged), Vec3(0.5f, 0, Sqrt(0.75f)), epsilon));

		trackball_mouse_state.x = 180;
		trackball_mouse_state.wheel = 1;
		mouse.Update();
		camera.Update(mouse, 200, 200, Pi / 3.f, false);
		TEST_CHECK(AlmostEqual(GetZ(camera.ComputeWorld()), GetZ(dragged), epsilon));
		TEST_CHECK(Equal(camera.distance, 3.4f * std::exp(-0.24f)));

		trackball_mouse_state.x = 190;
		trackball_mouse_state.button[MB_1] = true;
		trackball_mouse_state.wheel = 0;
		mouse.Update();
		camera.Update(mouse, 200, 200);
		TEST_CHECK(AlmostEqual(GetZ(camera.ComputeWorld()), GetZ(dragged), epsilon));
		TEST_CHECK(AlmostEqual(camera.target, Vec3::Zero, epsilon));
		RemoveMouseReader("trackball-test");
	}
}
