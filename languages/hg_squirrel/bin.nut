local bin = {};

bin.exec <- function(tool_name, ...) {
	local command = tool_name;
	for (local i = 0; i < vargv.len(); ++i)
		command += " " + vargv[i].tostring();
	return system(command);
};

return bin;
