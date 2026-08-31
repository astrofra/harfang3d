// Recursive directory listing

local hg = require("harfang");

local entry_types = {};
entry_types[hg.DE_File] <- "file";
entry_types[hg.DE_Dir] <- "directory";
entry_types[hg.DE_Link] <- "link";

function entry_type_to_string(entry_type) {
	if (entry_type in entry_types) {
		return entry_types[entry_type];
	}
	return "unknown";
}

local entries = hg.ListDirRecursive("resources", hg.DE_All);

foreach (entry in entries) {
	print("- " + entry.name + " is a " + entry_type_to_string(entry.type) + "\n");
}
