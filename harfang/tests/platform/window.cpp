// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "platform/window_system.h"

#include "foundation/log.h"
#include "foundation/thread.h"

#ifdef HG_TEST_GLFW
#include "platform/glfw/window_system.h"
#include "platform/input_system.h"
#include "engine/trackball_controller.h"
#include <cmath>
#endif

using namespace hg;

static void test_NewWindow() {
	Window *win = NewWindow(320, 200);
#ifndef __linux__ // Window init may fail, due to X11 authority issues within the CI. Disabling this test on Linux.
	TEST_CHECK(win != nullptr);
#endif
	
	for (int i = 0; i < 100; ++i) {
		UpdateWindow(win);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	DestroyWindow(win);
}

static void test_SetWindowPos() {
	Window *win = NewWindow(320, 200);
#ifndef __linux__ // Window init may fail, due to X11 authority issues within the CI. Disabling this test on Linux.
	TEST_CHECK(win != nullptr);
#endif
	SetWindowPos(win, iVec2(200, 200));

	for (int i = 0; i < 5; ++i) {
		UpdateWindow(win);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	iVec2 pos = GetWindowPos(win);

#ifndef __linux__ // window decorations are handled differently on each desktop on Linux, it's a mess, give up
	TEST_CHECK(pos.x == 200);
	TEST_CHECK(pos.y == 200);
#endif

	DestroyWindow(win);
}

static void test_GetWindowFrameBufferSize() {
	int width = -1, height = -1;
	TEST_CHECK(!GetWindowFrameBufferSize(nullptr, width, height));
	TEST_CHECK(width == 0 && height == 0);
	Window *win = NewWindow(320, 200);
	if (!win)
		return; // A graphical session may be unavailable on CI.
	UpdateWindow(win);
	TEST_CHECK(GetWindowFrameBufferSize(win, width, height));
	TEST_CHECK(width > 0 && height > 0);
	const int initial_width = width, initial_height = height;
	TEST_CHECK(SetWindowClientSize(win, 400, 240));
	for (int i = 0; i < 5; ++i) {
		UpdateWindow(win);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	TEST_CHECK(GetWindowFrameBufferSize(win, width, height));
	TEST_CHECK(width > initial_width && height > initial_height);
#ifdef __APPLE__
	int client_width, client_height;
	TEST_CHECK(GetWindowClientSize(win, client_width, client_height));
	const auto scale = GetWindowContentScale(win);
	TEST_CHECK(width == int(client_width * scale.x));
	TEST_CHECK(height == int(client_height * scale.y));
#endif
	DestroyWindow(win);
}

#ifdef HG_TEST_GLFW
static void test_FractionalScroll() {
	InputInit();
	Window *win = NewWindow(320, 200);
#ifdef __linux__
	if (!win) { // Match the existing window tests on CI without an X11 display.
		InputShutdown();
		return;
	}
#endif
	TEST_ASSERT(win != nullptr);
	auto glfw_win = GetGLFWWindow(win);
	glfwFocusWindow(glfw_win);
	for (int i = 0; i < 20 && GetWindowInFocus() != win; ++i) {
		UpdateWindow(win);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	TEST_ASSERT(GetWindowInFocus() == win);
	// Invoke the real registered GLFW callback, then read through the real Mouse.
	auto scroll = glfwSetScrollCallback(glfw_win, nullptr);
	TEST_ASSERT(scroll != nullptr);
	glfwSetScrollCallback(glfw_win, scroll);
	Mouse mouse;
	TrackballController camera(Vec3::Zero, 0.85f);
	mouse.Update();
	for (int tick = 1; tick <= 16; ++tick) {
		scroll(glfw_win, -0.25, 0.125);
		mouse.Update();
		TEST_CHECK(mouse.Wheel() == 0 && mouse.HWheel() == 0);
		TEST_CHECK(mouse.WheelDelta() == 0.125f && mouse.HWheelDelta() == -0.25f);
		const float previous = camera.distance;
		camera.Update(mouse, 320, 200);
		TEST_CHECK(camera.distance < previous);
		TEST_CHECK(Equal(camera.distance, 0.85f * std::exp(-0.12f * 0.125f * tick)));
	}
	// Sum callbacks once per read; retain signs and legacy integer semantics.
	scroll(glfw_win, 0.5, -0.125);
	scroll(glfw_win, 0.25, -0.25);
	mouse.Update();
	TEST_CHECK(mouse.Wheel() == 0 && mouse.WheelDelta() == -0.375f);
	TEST_CHECK(mouse.HWheel() == 0 && mouse.HWheelDelta() == 0.75f);
	scroll(glfw_win, -2.25, 1.75);
	mouse.Update();
	TEST_CHECK(mouse.Wheel() == 1 && mouse.WheelDelta() == 1.75f);
	TEST_CHECK(mouse.HWheel() == -2 && mouse.HWheelDelta() == -2.25f);
	mouse.Update();
	TEST_CHECK(mouse.WheelDelta() == 0.f && mouse.HWheelDelta() == 0.f);
	DestroyWindow(win);
	InputShutdown();
}
#endif

void test_window() {
	WindowSystemInit();
	test_NewWindow(); 
	test_SetWindowPos();
	test_GetWindowFrameBufferSize();
#ifdef HG_TEST_GLFW
	test_FractionalScroll();
#endif
	WindowSystemShutdown();
}
