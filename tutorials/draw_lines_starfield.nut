// Starfield 3D

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local width = 1280;
local height = 720;
local window = hg.RenderInit("Harfang - Starfield", width, height, hg.RF_VSync | hg.RF_MSAA4X);

local vtx_layout = hg.VertexLayout();
vtx_layout.Begin();
vtx_layout.Add(hg.A_Position, 3, hg.AT_Float);
vtx_layout.Add(hg.A_Color0, 3, hg.AT_Float);
vtx_layout.End();

local shader = hg.LoadProgramFromFile("resources_compiled/shaders/pos_rgb");

local starfield_size = 10.0;
local max_stars = 1000;
local vtx = hg.Vertices(vtx_layout, max_stars * 2);

local stars = [];
for (local i = 0; i < max_stars; ++i) {
	stars.append(hg.RandomVec3(-starfield_size, starfield_size));
}

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(window)) {
	hg.SetViewClear(0, hg.CF_Color | hg.CF_Depth, hg.Color.get_Black(), 1, 0);
	hg.SetViewRect(0, 0, 0, width, height);

	local dt = hg.TickClock();
	local dt_f = hg.time_to_sec_f(dt);

	vtx.Clear();
	for (local i = 0; i < stars.len(); ++i) {
		local star = stars[i];
		star.z -= 2 * dt_f;
		if (star.z < starfield_size) {
			star.z += starfield_size;
		}

		vtx.Begin(2 * i)
			.SetPos(star * hg.Vec3(1 / star.z, 1 / star.z, 0))
			.SetColor0(hg.Color.get_Black())
			.End();
		vtx.Begin(2 * i + 1)
			.SetPos(star * hg.Vec3(1.04 / star.z, 1.04 / star.z, 0))
			.SetColor0(hg.Color.get_White())
			.End();
	}

	hg.DrawLines(0, vtx, shader);

	hg.Frame();
	hg.UpdateWindow(window);

	collectgarbage();
}

hg.RenderShutdown();
hg.DestroyWindow(window);
