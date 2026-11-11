#define TEST_NO_MAIN
#include "acutest.h"

#include "engine/dear_imgui.h"
#include "platform/input_system.h"

using namespace hg;

void test_dear_imgui_mouse_scale() {
	ImGuiContext *previous = ImGui::GetCurrentContext();
	for (const Vec2 scale : {Vec2(1, 1), Vec2(2, 2), Vec2(1.25f, 1.5f)}) {
		DearImguiContext ctx;
		ctx.m_imgui = ImGui::CreateContext();
		auto &io = ImGui::GetIO();
		io.IniFilename = nullptr;
		io.Fonts->AddFontDefault();
		io.Fonts->Build();
		KeyboardState keyboard{};
		MouseState mouse{};
		mouse.x = 160;
		mouse.y = 100;
		// Exercise resized/nonuniform viewports too; there is no fixed 2x factor.
		for (int height : {600, 900}) {
			const ImVec2 target(float(mouse.x) * scale.x, float(height) - float(mouse.y) * scale.y);
			for (int frame = 0; frame != 4; ++frame) {
				mouse.button[MB_0] = frame == 2;
				mouse.wheel_delta = 0.125f;
				mouse.hwheel_delta = -0.25f;
				if (scale.x == 1 && scale.y == 1)
					ImGuiBeginFrame(ctx, 1280, height, time_from_ms(16), mouse, keyboard);
				else
					ImGuiBeginFrame(ctx, 1280, height, time_from_ms(16), mouse, keyboard, scale);
				TEST_CHECK(io.MousePos.x == target.x && io.MousePos.y == target.y);
				TEST_CHECK(io.MouseWheel == 0.125f && io.MouseWheelH == -0.25f);
				ImGui::SetNextWindowPos(ImVec2(target.x - 40, target.y - 40));
				ImGui::SetNextWindowSize(ImVec2(160, 100));
				ImGui::Begin("Mouse scale", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
				ImGui::SetCursorScreenPos(ImVec2(target.x - 20, target.y - 10));
				const bool clicked = ImGui::Button("Target", ImVec2(80, 30));
				if (frame >= 1)
					TEST_CHECK(ImGui::IsItemHovered());
				TEST_CHECK(clicked == (frame == 3));
				ImGui::End();
				ImGui::EndFrame();
			}
		}
		ImGui::DestroyContext(ctx.m_imgui);
	}
	ImGui::SetCurrentContext(previous);
}
