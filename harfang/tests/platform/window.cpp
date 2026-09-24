// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "platform/window_system.h"

#include "foundation/log.h"
#include "foundation/thread.h"

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

void test_window() {
	WindowSystemInit();
	test_NewWindow(); 
	test_SetWindowPos();
	test_GetWindowFrameBufferSize();
	WindowSystemShutdown();
}
