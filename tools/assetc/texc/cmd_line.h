#pragma once

#include <cstddef>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

struct CmdLineEntry {
	std::string name, desc;
	bool optional{false};
};

struct CmdLineFormat {
	std::vector<CmdLineEntry> flags;
	std::vector<CmdLineEntry> singles;
	std::vector<CmdLineEntry> positionals;

	std::map<std::string, std::string> aliases;
};

//
struct CmdLineContent {
	std::set<std::string> flags;
	std::map<std::string, std::string> singles;
	std::vector<std::string> positionals;
};

// http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html
bool ParseCmdLine(const std::vector<std::string> &args, const CmdLineFormat &format, CmdLineContent &content);

std::string FormatCmdLineArgs(const CmdLineFormat &fmt);
std::string FormatCmdLineArgsDescription(const CmdLineFormat &fmt);

//
bool GetCmdLineFlagValue(const CmdLineContent &cmd_content, const std::string &name);

bool CmdLineHasSingleValue(const CmdLineContent &cmd_content, const std::string &name);

std::string GetCmdLineSingleValue(const CmdLineContent &cmd_content, const std::string &name, const std::string &default_value);
int GetCmdLineSingleValue(const CmdLineContent &cmd_content, const std::string &name, int default_value);
float GetCmdLineSingleValue(const CmdLineContent &cmd_content, const std::string &name, float default_value);

//
template <typename T> std::string join(T begin_it, T end_it, const std::string &separator) {
	const ptrdiff_t count = std::distance(begin_it, end_it);

	if (count <= 0) {
		return {};
	}

	if (count == 1) {
		return *begin_it;
	}

	--end_it;

	std::string out;
	out.reserve((64 + 2) * count);

	for (T i = begin_it; i != end_it; ++i) {
		out += *i;
		out += separator;
	}

	out += *end_it;
	return out;
}
