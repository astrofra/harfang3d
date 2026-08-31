// Load a picture from a file.

local hg = require("harfang");

local pic = hg.Picture();

if (hg.LoadPicture(pic, "resources/pictures/owl.jpg")) {
	print("Picture dimensions: " + pic.GetWidth() + "x" + pic.GetHeight() + "\n");
} else {
	print("Failed to load picture!\n");
}
