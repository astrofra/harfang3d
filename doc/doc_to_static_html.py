import argparse
import html
import json
import os
import posixpath
import re
import shutil
import sys
import xml.etree.ElementTree as ETree
from pathlib import Path

try:
	import markdown
except ImportError as exc:
	print("The 'markdown' Python package is required. Install repository requirements.txt first.", file=sys.stderr)
	raise exc

import doc_utils.api_tools as api_tools
import doc_utils.doc_tools as doc_tools


LINK_RE = re.compile(r"(?<![A-Za-z0-9_])\[([A-Za-z_][A-Za-z0-9_\.]*)\](?!\()")
IMAGE_RE = re.compile(r"(!\[[^\]]*\]\()/images/([^\)]+)(\))")
HTML_ID_RE = re.compile(r"[^a-z0-9_]+")
HREF_RE = re.compile(r'href="([^"]+)"')
ID_RE = re.compile(r'id="([^"]+)"')
TAG_RE = re.compile(r"<[^>]+>")


BUILTIN_TYPES = {
	"void": {"cpython": "None", "lua": "nil"},
	"bool": {"cpython": "bool", "lua": "boolean"},
	"char": {"cpython": "int", "lua": "integer"},
	"short": {"cpython": "int", "lua": "integer"},
	"int": {"cpython": "int", "lua": "integer"},
	"long": {"cpython": "int", "lua": "integer"},
	"uchar": {"cpython": "int", "lua": "integer"},
	"ushort": {"cpython": "int", "lua": "integer"},
	"uint": {"cpython": "int", "lua": "integer"},
	"uint8_t": {"cpython": "int", "lua": "integer"},
	"uint16_t": {"cpython": "int", "lua": "integer"},
	"uint32_t": {"cpython": "int", "lua": "integer"},
	"int8_t": {"cpython": "int", "lua": "integer"},
	"int16_t": {"cpython": "int", "lua": "integer"},
	"int32_t": {"cpython": "int", "lua": "integer"},
	"Int8": {"cpython": "int", "lua": "integer"},
	"Int16": {"cpython": "int", "lua": "integer"},
	"Int32": {"cpython": "int", "lua": "integer"},
	"Int64": {"cpython": "int", "lua": "integer"},
	"Char16": {"cpython": "int", "lua": "integer"},
	"Char32": {"cpython": "int", "lua": "integer"},
	"UInt8": {"cpython": "int", "lua": "integer"},
	"UInt16": {"cpython": "int", "lua": "integer"},
	"UInt32": {"cpython": "int", "lua": "integer"},
	"UInt64": {"cpython": "int", "lua": "integer"},
	"IntPtr": {"cpython": "int", "lua": "integer"},
	"size_t": {"cpython": "int", "lua": "integer"},
	"ptrdiff_t": {"cpython": "int", "lua": "integer"},
	"float": {"cpython": "float", "lua": "number"},
	"double": {"cpython": "float", "lua": "number"},
	"string": {"cpython": "str", "lua": "string"},
}


EXTRA_CSS = """

* {
	box-sizing: border-box;
}

body.static-doc {
	margin: 0;
	padding: 0;
	color: #1d1f23;
	background: #ffffff;
}

.doc-layout {
	display: grid;
	grid-template-columns: minmax(220px, 300px) minmax(0, 1fr);
	min-height: 100vh;
}

.doc-sidebar {
	position: sticky;
	top: 0;
	height: 100vh;
	overflow: auto;
	padding: 18px;
	border-right: 1px solid #d8dde5;
	background: #f7f9fb;
}

.doc-brand {
	display: block;
	margin-bottom: 18px;
	color: #11151a;
	font-size: 20px;
	font-weight: 700;
	text-decoration: none;
}

.doc-version {
	display: block;
	color: #687385;
	font-size: 12px;
	font-weight: 500;
}

.doc-search {
	width: 100%;
	margin: 0 0 8px;
	padding: 8px 10px;
	border: 1px solid #bec7d2;
	border-radius: 6px;
	background: #ffffff;
	font: inherit;
}

.doc-search-results {
	display: none;
	margin: 0 0 18px;
	padding: 8px;
	border: 1px solid #d8dde5;
	border-radius: 6px;
	background: #ffffff;
}

.doc-search-results.active {
	display: block;
}

.doc-search-result {
	display: block;
	padding: 7px 6px;
	border-radius: 4px;
	color: #1f5f9f;
	text-decoration: none;
}

.doc-search-result:hover {
	background: #edf3f8;
	text-decoration: none;
}

.doc-search-result small {
	display: block;
	color: #687385;
}

.doc-nav-title {
	margin: 20px 0 8px;
	color: #4d5968;
	font-size: 12px;
	font-weight: 700;
	text-transform: uppercase;
}

.doc-nav {
	margin: 0;
	padding: 0;
	list-style: none;
}

.doc-nav li {
	margin: 0;
}

.doc-nav-spacer {
	height: 10px;
}

.doc-nav a {
	display: block;
	padding: 5px 7px;
	border-radius: 6px;
	color: #273241;
	text-decoration: none;
}

.doc-nav a:hover,
.doc-nav a.active {
	background: #e5ecf4;
	color: #0c4f8a;
	text-decoration: none;
}

.doc-content {
	width: min(1120px, 100%);
	padding: 24px 34px 64px;
}

.doc-footer {
	margin-top: 48px;
	padding-top: 18px;
	border-top: 1px solid #d8dde5;
	color: #687385;
	font-size: 13px;
}

.api-entry {
	margin: 0 0 34px;
	padding-top: 8px;
	border-top: 1px solid #d8dde5;
}

.api-kind {
	color: #687385;
	font-size: 14px;
	font-weight: 500;
}

.api-table {
	width: 100%;
	margin: 14px 0 20px;
	border-collapse: collapse;
}

.api-table th,
.api-table td {
	padding: 7px 9px;
	border-bottom: 1px solid #e4e8ee;
	vertical-align: top;
}

.api-table th {
	text-align: left;
	background: #f3f6f9;
}

.api-proto {
	margin: 4px 0;
	font-family: Consolas, "Liberation Mono", Menlo, monospace;
	font-size: 13px;
}

.api-doc {
	margin: 10px 0 16px;
}

.api-members {
	margin: 8px 0 16px;
}

@media (max-width: 860px) {
	.doc-layout {
		display: block;
	}

	.doc-sidebar {
		position: static;
		height: auto;
		border-right: 0;
		border-bottom: 1px solid #d8dde5;
	}

	.doc-content {
		padding: 18px 18px 48px;
	}
}
"""


SEARCH_JS = """
(function () {
	function escapeHtml(value) {
		return String(value).replace(/[&<>"']/g, function (ch) {
			return ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[ch];
		});
	}

	function installSearch() {
		var input = document.getElementById("doc-search");
		var results = document.getElementById("doc-search-results");
		if (!input || !results) {
			return;
		}

		var root = document.body.getAttribute("data-root") || "";
		var index = window.HG_SEARCH_INDEX || [];

		input.addEventListener("input", function () {
			var query = input.value.trim().toLowerCase();
			if (query.length < 2) {
				results.className = "doc-search-results";
				results.innerHTML = "";
				return;
			}

			var matches = index.filter(function (item) {
				return (item.title + " " + item.kind + " " + item.text).toLowerCase().indexOf(query) !== -1;
			}).slice(0, 20);

			if (!matches.length) {
				results.className = "doc-search-results active";
				results.innerHTML = "<small>No result.</small>";
				return;
			}

			results.className = "doc-search-results active";
			results.innerHTML = matches.map(function (item) {
				return '<a class="doc-search-result" href="' + root + item.href + '">' +
					escapeHtml(item.title) + '<small>' + escapeHtml(item.kind) + '</small></a>';
			}).join("");
		});
	}

	if (document.readyState === "loading") {
		document.addEventListener("DOMContentLoaded", installSearch);
	} else {
		installSearch();
	}
})();
"""


class StaticDocGenerator:
	def __init__(self, args):
		self.args = args
		self.api_root = ETree.parse(args.api).getroot()
		self.out_dir = Path(args.out)
		self.doc_dir = Path(args.doc)
		self.img_dir = Path(args.img)
		self.css_path = Path(args.css)
		self.languages = ["cpython", "lua"]
		self.manual_order = []
		self.manual_spacers = set()
		self.manual_titles = {}
		self.routes = {}
		self.entry_routes = {}
		self.entry_collisions = set()
		self.unresolved_links = []
		self.missing_images = []
		self.generated_pages = {}
		self.search_items = []

	def run(self):
		api_tools.load_api(self.args.api)
		doc_tools.load_doc_folder(self.args.doc)

		self.prepare_output()
		self.load_manual_order()
		self.build_routes()
		self.copy_assets()

		self.write_index_page()
		self.write_manual_pages()
		self.write_api_pages()
		self.write_search_js()
		self.write_reports()
		self.print_summary()

		if self.args.strict and (self.unresolved_links or self.missing_images):
			raise SystemExit(1)

	def prepare_output(self):
		if self.out_dir.exists():
			shutil.rmtree(self.out_dir)
		self.out_dir.mkdir(parents=True, exist_ok=True)

	def load_manual_order(self):
		tree_desc = self.doc_dir / "tree_desc.txt"
		last_uid = None
		for raw in tree_desc.read_text(encoding="utf-8").splitlines():
			uid = raw.strip()
			if uid:
				self.manual_order.append(uid)
				last_uid = uid
			elif last_uid:
				self.manual_spacers.add(last_uid)

		for uid in self.manual_order:
			text = doc_tools.man.get(uid, "")
			title, _ = self.extract_title(text, uid)
			self.manual_titles[uid] = title

	def build_routes(self):
		for uid in self.manual_order:
			self.routes[uid] = {
				"path": "manual/%s.html" % self.slug(uid[4:]),
				"anchor": "",
				"title": self.manual_titles.get(uid, uid),
				"kind": "manual",
			}

		for lang in self.languages:
			for uid, tags in api_tools.api.items():
				tag = tags[0]
				if tag.tag == "class":
					self.routes[(lang, uid)] = self.api_route(lang, "classes", uid, tag.get("name"), "class")
				elif tag.tag == "function":
					parent = api_tools.api_parent_map.get(tag)
					if parent is not None and parent.tag == "class":
						self.routes[(lang, uid)] = self.api_route(lang, "classes", uid, self.function_display_name(tag), "method")
					else:
						self.routes[(lang, uid)] = self.api_route(lang, "functions", uid, tag.get("name"), "function")
				elif tag.tag in ("enum", "constants"):
					self.routes[(lang, uid)] = self.api_route(lang, "constants", uid, tag.get("name"), tag.tag)

		for lang in self.languages:
			for uid, tags in api_tools.api.items():
				tag = tags[0]
				if tag.tag not in ("enum", "constants"):
					continue
				for entry in tag:
					name = entry.get("name")
					if not name:
						continue
					route = {
						"path": "api/%s/constants.html" % lang,
						"anchor": self.entry_anchor(uid, name),
						"title": name,
						"kind": "constant",
					}
					key = (lang, name)
					if key in self.entry_routes:
						self.entry_collisions.add(name)
					else:
						self.entry_routes[key] = route

	def api_route(self, lang, page, uid, title, kind):
		return {
			"path": "api/%s/%s.html" % (lang, page),
			"anchor": self.anchor(uid),
			"title": title or uid,
			"kind": kind,
		}

	def copy_assets(self):
		css = self.css_path.read_text(encoding="utf-8") if self.css_path.exists() else ""
		self.write_text_file(self.out_dir / "doc.css", css + EXTRA_CSS)

		image_out = self.out_dir / "images" / "docs" / self.args.version
		image_out.mkdir(parents=True, exist_ok=True)
		if self.img_dir.exists():
			for item in self.img_dir.iterdir():
				if item.is_file():
					shutil.copy2(item, image_out / item.name)

	def write_index_page(self):
		uid = "man.Overview" if "man.Overview" in doc_tools.man else self.manual_order[0]
		title, text = self.extract_title(doc_tools.man.get(uid, ""), uid)
		content = self.render_markdown(uid, text, "index.html", "cpython")
		html_page = self.page_shell("index.html", title, content)
		self.write_page("index.html", html_page)

	def write_manual_pages(self):
		for uid in self.manual_order:
			title, text = self.extract_title(doc_tools.man.get(uid, ""), uid)
			page_path = self.routes[uid]["path"]
			content = self.render_markdown(uid, text, page_path, "cpython")
			html_page = self.page_shell(page_path, title, content)
			self.write_page(page_path, html_page)
			self.add_search_item(title, page_path, "manual", text)

	def write_api_pages(self):
		for lang in self.languages:
			self.write_api_classes_page(lang)
			self.write_api_functions_page(lang)
			self.write_api_constants_page(lang)

	def write_api_classes_page(self, lang):
		page_path = "api/%s/classes.html" % lang
		classes = self.sorted_uids_for_tag("class")
		parts = ["<h1>API Classes <span class=\"api-kind\">%s</span></h1>" % html.escape(self.language_title(lang))]

		for uid in classes:
			tag = api_tools.api[uid][0]
			name = tag.get("name", uid)
			parts.append('<section class="api-entry" id="%s">' % self.anchor(uid))
			parts.append("<h2>%s <span class=\"api-kind\">Class</span></h2>" % html.escape(name))
			parts.append(self.render_symbol_doc(uid, page_path, lang))
			parts.append(self.render_class_info(uid, tag, page_path, lang))
			parts.append(self.render_class_variables(tag, page_path, lang))
			parts.append(self.render_class_functions(tag, page_path, lang))
			parts.append("</section>")
			self.add_search_item(name, "%s#%s" % (page_path, self.anchor(uid)), "class", doc_tools.doc.get(uid, ""))

		self.write_page(page_path, self.page_shell(page_path, "API Classes - %s" % self.language_title(lang), "\n".join(parts)))

	def write_api_functions_page(self, lang):
		page_path = "api/%s/functions.html" % lang
		functions = self.global_function_groups()
		parts = ["<h1>API Functions <span class=\"api-kind\">%s</span></h1>" % html.escape(self.language_title(lang))]

		for uid, tags in functions:
			name = tags[0].get("name", uid)
			parts.append('<section class="api-entry" id="%s">' % self.anchor(uid))
			parts.append("<h2>%s <span class=\"api-kind\">Function</span></h2>" % html.escape(name))
			parts.append(self.render_prototypes(tags, page_path, lang))
			parts.append(self.render_symbol_doc(uid, page_path, lang))
			parts.append("</section>")
			self.add_search_item(name, "%s#%s" % (page_path, self.anchor(uid)), "function", doc_tools.doc.get(uid, ""))

		self.write_page(page_path, self.page_shell(page_path, "API Functions - %s" % self.language_title(lang), "\n".join(parts)))

	def write_api_constants_page(self, lang):
		page_path = "api/%s/constants.html" % lang
		uids = self.sorted_uids_for_tag("enum") + self.sorted_uids_for_tag("constants")
		parts = ["<h1>API Constants <span class=\"api-kind\">%s</span></h1>" % html.escape(self.language_title(lang))]

		for uid in uids:
			tag = api_tools.api[uid][0]
			name = tag.get("name", uid)
			kind = "Enumeration" if tag.tag == "enum" else "Constants"
			parts.append('<section class="api-entry" id="%s">' % self.anchor(uid))
			parts.append("<h2>%s <span class=\"api-kind\">%s</span></h2>" % (html.escape(name), kind))
			parts.append(self.render_symbol_doc(uid, page_path, lang))
			parts.append("<ul class=\"api-members\">")
			for entry in tag:
				entry_name = entry.get("name")
				if entry_name:
					parts.append('<li id="%s"><code>%s</code></li>' % (self.entry_anchor(uid, entry_name), html.escape(entry_name)))
			parts.append("</ul>")
			parts.append("</section>")
			self.add_search_item(name, "%s#%s" % (page_path, self.anchor(uid)), kind.lower(), doc_tools.doc.get(uid, ""))

		self.write_page(page_path, self.page_shell(page_path, "API Constants - %s" % self.language_title(lang), "\n".join(parts)))

	def write_search_js(self):
		payload = "window.HG_SEARCH_INDEX = "
		payload += json.dumps(self.search_items, ensure_ascii=False, indent=2)
		payload += ";\n"
		payload += SEARCH_JS
		self.write_text_file(self.out_dir / "search.js", payload)

	def write_reports(self):
		reports = self.out_dir / "reports"
		reports.mkdir(parents=True, exist_ok=True)

		self.write_lines(reports / "unresolved_links.txt", [
			"%s: [%s]" % (page, uid) for page, uid in sorted(set(self.unresolved_links))
		])
		self.write_lines(reports / "missing_images.txt", [
			"%s: %s" % (page, image) for page, image in sorted(set(self.missing_images))
		])
		self.write_lines(reports / "stale_symbol_docs.txt", self.stale_symbol_docs())
		self.write_lines(reports / "missing_symbol_docs.txt", self.missing_symbol_docs())
		self.write_lines(reports / "ambiguous_constant_entries.txt", sorted(self.entry_collisions))
		self.write_lines(reports / "broken_local_links.txt", self.find_broken_local_links())

	def print_summary(self):
		api_uids = {uid for uid in api_tools.api.keys()}
		symbol_docs = {path.stem for path in self.doc_dir.glob("*.md") if not path.stem.startswith("man.")}
		print("Static documentation generated:")
		print("  output: %s" % self.out_dir)
		print("  pages: %d" % len(self.generated_pages))
		print("  manual pages: %d" % len(self.manual_order))
		print("  API UIDs: %d" % len(api_uids))
		print("  symbol Markdown files: %d" % len(symbol_docs))
		print("  unresolved links: %d" % len(set(self.unresolved_links)))
		print("  missing images: %d" % len(set(self.missing_images)))
		print("  reports: %s" % (self.out_dir / "reports"))

	def render_class_info(self, uid, tag, page_path, lang):
		rows = []
		inherits = [item.get("uid") for item in tag.findall("inherits") if item.get("uid")]
		if inherits:
			rows.append((
				"Inherits",
				", ".join(self.link_or_code(base_uid, page_path, lang) for base_uid in inherits),
			))
		if tag.get("base_type", "0") == "1":
			rows.append(("Instantiation", "This base type cannot be instantiated directly."))

		if not rows:
			return ""

		out = ['<table class="api-table">']
		for name, value in rows:
			out.append("<tr><th>%s</th><td>%s</td></tr>" % (html.escape(name), value))
		out.append("</table>")
		return "\n".join(out)

	def render_class_variables(self, tag, page_path, lang):
		vars_ = [child for child in tag if child.tag == "variable"]
		if not vars_:
			return ""

		out = ["<h3>Variables</h3>", '<table class="api-table"><tr><th>Name</th><th>Type</th></tr>']
		for var in sorted(vars_, key=lambda item: item.get("name", "")):
			type_html = self.link_or_code(var.get("type", ""), page_path, lang)
			if var.get("static") == "1":
				type_html = "static " + type_html
			out.append("<tr><td><code>%s</code></td><td>%s</td></tr>" % (html.escape(var.get("name", "")), type_html))
		out.append("</table>")
		return "\n".join(out)

	def render_class_functions(self, tag, page_path, lang):
		groups = {}
		for child in tag:
			if child.tag == "function":
				groups.setdefault(child.get("uid"), []).append(child)

		if not groups:
			return ""

		out = ["<h3>Functions</h3>", '<table class="api-table"><tr><th>Name</th><th>Prototype</th><th>Documentation</th></tr>']
		for uid in sorted(groups.keys(), key=lambda item: groups[item][0].get("name", "")):
			name = groups[uid][0].get("name", uid)
			doc = self.render_symbol_doc(uid, page_path, lang, compact=True)
			out.append(
				'<tr id="%s"><td><code>%s</code></td><td>%s</td><td>%s</td></tr>' %
				(self.anchor(uid), html.escape(name), self.render_prototypes(groups[uid], page_path, lang), doc)
			)
		out.append("</table>")
		return "\n".join(out)

	def render_prototypes(self, tags, page_path, lang):
		out = []
		for tag in tags:
			out.append('<div class="api-proto">%s</div>' % self.format_prototype(tag, page_path, lang))
		return "\n".join(out)

	def format_prototype(self, tag, page_path, lang):
		name = tag.get("name", "")
		inputs = []
		outputs = []
		for parm in tag:
			if parm.tag != "parm":
				continue
			parm_name = parm.get("name", "")
			if parm_name.startswith("OUTPUT") or parm_name.startswith("INOUT"):
				outputs.append(parm)
			else:
				inputs.append(parm)

		ret_uid = tag.get("returns_constants_group") or tag.get("returns", "void")
		returns = []
		if ret_uid != "void":
			returns.append(self.link_or_code(ret_uid, page_path, lang))
		for parm in outputs:
			returns.append(self.link_or_code(parm.get("constants_group") or parm.get("type", ""), page_path, lang))
		if not returns:
			returns.append(self.format_builtin("void", lang))

		if lang == "cpython":
			args = []
			for parm in inputs:
				parm_name = parm.get("name", "arg")
				parm_type = self.link_or_code(parm.get("constants_group") or parm.get("type", ""), page_path, lang)
				args.append("%s: %s" % (html.escape(parm_name), parm_type))
			return "%s(%s) -&gt; %s" % (html.escape(name), ", ".join(args), ", ".join(returns))

		args = []
		for parm in inputs:
			parm_name = parm.get("name", "arg")
			parm_type = self.link_or_code(parm.get("constants_group") or parm.get("type", ""), page_path, lang)
			args.append("%s %s" % (parm_type, html.escape(parm_name)))
		return "%s %s(%s)" % (", ".join(returns), html.escape(name), ", ".join(args))

	def render_symbol_doc(self, uid, page_path, lang, compact=False):
		text = doc_tools.doc.get(uid, "")
		if not text:
			return "" if compact else '<div class="api-doc"></div>'
		lines = [line for line in text.splitlines() if not line.startswith(".proto")]
		text = "\n".join(lines).strip()
		if not text:
			return "" if compact else '<div class="api-doc"></div>'
		return '<div class="api-doc">%s</div>' % self.render_markdown(uid, text, page_path, lang)

	def render_markdown(self, uid, text, page_path, lang):
		text = text.replace("${HG_VERSION}", self.args.version)
		text = IMAGE_RE.sub(lambda m: self.rewrite_image(page_path, m), text)
		text = LINK_RE.sub(lambda m: self.rewrite_link(uid, page_path, lang, m), text)
		return markdown.markdown(text, extensions=["markdown.extensions.extra", "markdown.extensions.codehilite", "markdown.extensions.toc"], output_format="html5")

	def rewrite_image(self, page_path, match):
		image_path = match.group(2).replace("${HG_VERSION}", self.args.version)
		output_path = "images/%s" % image_path
		if not (self.out_dir / output_path).exists():
			self.missing_images.append((page_path, output_path))
		return "%s%s%s" % (match.group(1), self.relative_href(page_path, output_path), match.group(3))

	def rewrite_link(self, uid, page_path, lang, match):
		raw = match.group(1)
		if raw == "TOC":
			return match.group(0)
		if raw in BUILTIN_TYPES:
			return self.format_builtin(raw, lang)

		if raw in doc_tools.man:
			return self.link_to_route(page_path, self.routes[raw], self.manual_titles.get(raw, raw))

		route = self.routes.get((lang, raw)) or self.routes.get(("cpython", raw))
		if route:
			return self.link_to_route(page_path, route, self.display_title(lang, raw, route))

		entry_route = self.entry_routes.get((lang, raw)) or self.entry_routes.get(("cpython", raw))
		if entry_route:
			return self.link_to_route(page_path, entry_route, raw)

		self.unresolved_links.append((page_path, raw))
		return match.group(0)

	def link_or_code(self, uid, page_path, lang):
		if uid in BUILTIN_TYPES:
			return self.format_builtin(uid, lang)
		route = self.routes.get((lang, uid)) or self.entry_routes.get((lang, uid))
		if route:
			return self.link_to_route(page_path, route, route["title"])
		return "<code>%s</code>" % html.escape(uid)

	def link_to_route(self, page_path, route, label):
		href = self.relative_href(page_path, route["path"], route.get("anchor", ""))
		return '<a href="%s">%s</a>' % (html.escape(href, quote=True), html.escape(label))

	def display_title(self, lang, uid, route):
		tags = api_tools.api.get(uid)
		if not tags:
			return route["title"]
		tag = tags[0]
		if tag.tag == "function":
			parent = api_tools.api_parent_map.get(tag)
			if parent is not None and parent.tag == "class":
				return "%s.%s" % (parent.get("name"), tag.get("name"))
		return route["title"]

	def relative_href(self, page_path, target_path, anchor=""):
		if page_path == target_path:
			return "#%s" % anchor if anchor else posixpath.basename(target_path)
		base = posixpath.dirname(page_path)
		rel = posixpath.relpath(target_path, base if base else ".")
		if rel == ".":
			rel = posixpath.basename(target_path)
		return "%s#%s" % (rel, anchor) if anchor else rel

	def root_prefix(self, page_path):
		depth = page_path.count("/")
		return "../" * depth

	def page_shell(self, page_path, title, content):
		root = self.root_prefix(page_path)
		nav = self.render_nav(page_path)
		return """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title} - {project}</title>
<link rel="stylesheet" href="{css}">
</head>
<body class="static-doc" data-root="{root}">
<div class="doc-layout">
<aside class="doc-sidebar">
<a class="doc-brand" href="{home}">{project}<span class="doc-version">{version}</span></a>
<input id="doc-search" class="doc-search" type="search" placeholder="Search documentation">
<div id="doc-search-results" class="doc-search-results"></div>
{nav}
</aside>
<main class="doc-content">
{content}
<div class="doc-footer">{project} {version} static documentation.</div>
</main>
</div>
<script src="{search}"></script>
</body>
</html>
""".format(
			title=html.escape(title),
			project=html.escape(self.args.project_name),
			version=html.escape(self.args.version),
			root=html.escape(root, quote=True),
			css=html.escape(self.relative_href(page_path, "doc.css"), quote=True),
			search=html.escape(self.relative_href(page_path, "search.js"), quote=True),
			home=html.escape(self.relative_href(page_path, "index.html"), quote=True),
			nav=nav,
			content=content,
		)

	def render_nav(self, current_path):
		parts = ['<div class="doc-nav-title">Manual</div>', '<ul class="doc-nav">']
		for uid in self.manual_order:
			route = self.routes[uid]
			active = " active" if route["path"] == current_path else ""
			parts.append('<li><a class="%s" href="%s">%s</a></li>' % (
				active.strip(),
				html.escape(self.relative_href(current_path, route["path"]), quote=True),
				html.escape(route["title"]),
			))
			if uid in self.manual_spacers:
				parts.append('<li class="doc-nav-spacer"></li>')
		parts.append("</ul>")

		for lang in self.languages:
			parts.append('<div class="doc-nav-title">API %s</div>' % html.escape(self.language_title(lang)))
			parts.append('<ul class="doc-nav">')
			for label, path in [
				("Classes", "api/%s/classes.html" % lang),
				("Functions", "api/%s/functions.html" % lang),
				("Constants", "api/%s/constants.html" % lang),
			]:
				active = " active" if path == current_path else ""
				parts.append('<li><a class="%s" href="%s">%s</a></li>' % (
					active.strip(),
					html.escape(self.relative_href(current_path, path), quote=True),
					label,
				))
			parts.append("</ul>")
		return "\n".join(parts)

	def write_page(self, page_path, content):
		path = self.out_dir / page_path
		path.parent.mkdir(parents=True, exist_ok=True)
		self.write_text_file(path, content)
		self.generated_pages[page_path] = content

	def add_search_item(self, title, href, kind, text):
		clean_text = self.clean_text(text)
		self.search_items.append({
			"title": title,
			"href": href,
			"kind": kind,
			"text": clean_text[:240],
		})

	def sorted_uids_for_tag(self, tag_name):
		uids = [uid for uid, tags in api_tools.api.items() if tags[0].tag == tag_name and tags[0].get("global", "1") == "1"]
		return sorted(uids, key=lambda uid: api_tools.api[uid][0].get("name", uid).lower())

	def global_function_groups(self):
		groups = {}
		for uid, tags in api_tools.api.items():
			if tags[0].tag != "function":
				continue
			parent = api_tools.api_parent_map.get(tags[0])
			if parent is not None and parent.tag == "class":
				continue
			groups.setdefault(uid, []).extend(tags)
		return sorted(groups.items(), key=lambda item: item[1][0].get("name", item[0]).lower())

	def function_display_name(self, tag):
		parent = api_tools.api_parent_map.get(tag)
		if parent is not None and parent.tag == "class":
			return "%s.%s" % (parent.get("name"), tag.get("name"))
		return tag.get("name")

	def language_title(self, lang):
		return "Python" if lang == "cpython" else "Lua"

	def format_builtin(self, uid, lang):
		label = BUILTIN_TYPES.get(uid, {}).get(lang, uid)
		return "<code>%s</code>" % html.escape(label)

	def extract_title(self, text, uid):
		lines = text.splitlines()
		if lines and lines[0].startswith(".title "):
			title = lines[0][7:].strip()
			return title, "\n".join(lines[1:]).lstrip()
		return uid, text

	def anchor(self, value):
		value = HTML_ID_RE.sub("-", value.lower()).strip("-")
		return value or "entry"

	def entry_anchor(self, uid, entry_name):
		return "%s-%s" % (self.anchor(uid), self.anchor(entry_name))

	def slug(self, value):
		return self.anchor(value)

	def clean_text(self, text):
		text = LINK_RE.sub(lambda m: m.group(1), text)
		text = TAG_RE.sub(" ", text)
		text = re.sub(r"\s+", " ", text)
		return text.strip()

	def stale_symbol_docs(self):
		api_uids = set(api_tools.api.keys())
		symbol_docs = {path.stem for path in self.doc_dir.glob("*.md") if not path.stem.startswith("man.")}
		return sorted(symbol_docs - api_uids)

	def missing_symbol_docs(self):
		api_uids = set(api_tools.api.keys())
		symbol_docs = {path.stem for path in self.doc_dir.glob("*.md") if not path.stem.startswith("man.")}
		return sorted(api_uids - symbol_docs)

	def find_broken_local_links(self):
		broken = []
		anchors_by_page = {}
		for page_path, content in self.generated_pages.items():
			anchors_by_page[page_path] = set(ID_RE.findall(content))

		for page_path, content in self.generated_pages.items():
			for href in HREF_RE.findall(content):
				if href.startswith(("http://", "https://", "mailto:", "javascript:")):
					continue
				if href.startswith("#"):
					target_page = page_path
					anchor = href[1:]
				else:
					href_path, _, anchor = href.partition("#")
					base = posixpath.dirname(page_path)
					target_page = posixpath.normpath(posixpath.join(base, href_path)).replace("\\", "/")

				if target_page not in self.generated_pages and not (self.out_dir / target_page).exists():
					broken.append("%s: %s" % (page_path, href))
				elif anchor and anchor not in anchors_by_page.get(target_page, set()):
					broken.append("%s: %s" % (page_path, href))
		return sorted(set(broken))

	def write_lines(self, path, lines):
		self.write_text_file(path, "\n".join(lines) + ("\n" if lines else ""))

	def write_text_file(self, path, content):
		with open(path, "w", encoding="utf-8", newline="\n") as file:
			file.write(content)


def parse_args():
	parser = argparse.ArgumentParser(description="Generate static Harfang HTML documentation.")
	parser.add_argument("--api", required=True, help="FabGen API XML path")
	parser.add_argument("--doc", required=True, help="Documentation Markdown folder")
	parser.add_argument("--img", required=True, help="Documentation image folder")
	parser.add_argument("--css", required=True, help="Base CSS file")
	parser.add_argument("--out", required=True, help="Output folder")
	parser.add_argument("--version", required=True, help="Documentation version")
	parser.add_argument("--project-name", default="Harfang", help="Project display name")
	parser.add_argument("--strict", action="store_true", help="Fail on unresolved links or missing images")
	return parser.parse_args()


if __name__ == "__main__":
	StaticDocGenerator(parse_args()).run()
