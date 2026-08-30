// Load a JPG picture and save it as PNG.

local hg = require("harfang");

local pic = hg.Picture();

local ok = hg.LoadJPG(pic, "resources/pictures/owl.jpg");
if (!ok) {
	throw "Failed to load picture!";
}

ok = hg.SavePNG(pic, "owl.png");
if (!ok) {
	throw "Failed to save picture!";
}

print("Conversion complete\n");
