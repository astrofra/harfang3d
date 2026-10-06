// Recursive directory listing

import * as hg from 'harfang';

function entry_type_to_string(type) {
	const tp = {};
	tp[hg.DE_File] = 'file';
	tp[hg.DE_Dir] = 'directory';
	tp[hg.DE_Link] = 'link';
	return tp[type];
}

export function main({ path = 'resources_compiled' } = {}) {
	if (!hg.IsDir(path)) throw Error('Directory not found: ' + path);
	const entries = hg.ListDirRecursive(path, hg.DE_All);

	for (let i = 0; i < entries.size(); i++) {
		const entry = entries.at(i);
		console.log(`- ${entry.name} is a ${entry_type_to_string(entry.type)}`);
	}
}
