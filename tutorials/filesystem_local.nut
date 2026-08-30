// Access to a file through the local filesystem

local hg = require("harfang");

local file = hg.Open("resources/pictures/owl.jpg");

if (hg.IsValid(file)) {
	print("File is " + hg.GetSize(file).tostring() + " bytes long\n");
} else {
	print("Failed to open file\n");
}

hg.Close(file);
