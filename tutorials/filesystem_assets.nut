// Access to a file by mounting a folder as an assets source

local hg = require("harfang");

hg.WindowSystemInit();

local res_x = 256;
local res_y = 256;
local win = hg.RenderInit("Harfang - Load from Assets", res_x, res_y, hg.RF_VSync);

// Mount folder as an assets source and load texture from the assets system.
hg.AddAssetsFolder("resources_compiled");

local load_result = hg.LoadTextureFromAssets("pictures/owl.jpg", 0);
local tex = load_result[0];
local tex_info = load_result[1];

if (hg.IsValid(tex)) {
	print("Texture dimensions: " + tex_info.width.tostring() + "x" + tex_info.height.tostring() + "\n");
} else {
	print("Failed to load texture\n");
}

hg.RenderShutdown();
