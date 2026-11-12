// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "platform/platform.h"
#include "foundation/path_tools.h"

#import <Cocoa/Cocoa.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <functional>
#include <stdexcept>

namespace hg {

// AppKit panels must run on the main thread, including requests from a script.
static bool OnMainThread(const std::function<bool()> &action) {
	if ([NSThread isMainThread]) return action();
	__block bool result = false;
	dispatch_sync(dispatch_get_main_queue(), ^{ result = action(); });
	return result;
}

bool InitPlatform() {
	return OnMainThread([] {
		@autoreleasepool {
			[NSApplication sharedApplication];
			return true;
		}
	});
}

std::string GetPlatformLocale() {
	@autoreleasepool {
		return [[[NSLocale currentLocale] localeIdentifier] UTF8String];
	}
}

static void SetFilters(NSSavePanel *panel, const std::vector<FileFilter> &filters) {
	NSMutableArray<UTType *> *types = [NSMutableArray array];
	for (const auto &filter : filters) {
		NSString *patterns = [NSString stringWithUTF8String:filter.pattern.c_str()];
		for (NSString *pattern in [patterns componentsSeparatedByString:@";"]) {
			NSString *extension = [pattern stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
			if ([extension isEqualToString:@"*"] || [extension isEqualToString:@"*.*"]) return;
			if ([extension hasPrefix:@"*."]) extension = [extension substringFromIndex:2];
			else if ([extension hasPrefix:@"."]) extension = [extension substringFromIndex:1];
			if (extension.length == 0) continue;
			UTType *type = [UTType typeWithFilenameExtension:extension];
			if (type && ![types containsObject:type]) [types addObject:type];
		}
	}
	panel.allowedContentTypes = types;
	panel.allowsOtherFileTypes = types.count == 0;
}

static void SetInitialLocation(NSSavePanel *panel, const std::string &initial_dir, const std::string &output, bool save) {
	NSString *suggested = [[NSString stringWithUTF8String:output.c_str()] stringByExpandingTildeInPath];
	NSString *directory = [[NSString stringWithUTF8String:initial_dir.c_str()] stringByExpandingTildeInPath];
	BOOL is_directory = NO;
	if (suggested.length && [[NSFileManager defaultManager] fileExistsAtPath:suggested isDirectory:&is_directory] && is_directory) {
		if (!directory.length) directory = suggested;
	} else if (suggested.length) {
		if (save) panel.nameFieldStringValue = suggested.lastPathComponent;
		if (!directory.length) directory = suggested.stringByDeletingLastPathComponent;
	}
	if (directory.length) panel.directoryURL = [NSURL fileURLWithPath:directory isDirectory:YES];
}

static bool CompletePanel(NSSavePanel *panel, std::string &output) {
	NSWindow *owner = NSApp.keyWindow ?: NSApp.mainWindow;
	[NSApp activateIgnoringOtherApps:YES];
	const NSModalResponse response = [panel runModal];
	// GLFW polls keyboard state on the key window. Restore it after the native
	// panel closes, including cancellation, so shortcuts resume immediately.
	if (owner && owner != panel && owner.isVisible) [owner makeKeyAndOrderFront:nil];
	if (response != NSModalResponseOK || !panel.URL.isFileURL) return false;
	output = CleanPath([panel.URL.path UTF8String]);
	return true;
}

bool OpenFolderDialog(const std::string &title, std::string &output, const std::string &initial_dir) {
	return OnMainThread([&] {
		@autoreleasepool {
			[NSApplication sharedApplication];
			NSOpenPanel *panel = [NSOpenPanel openPanel];
			panel.title = [NSString stringWithUTF8String:title.c_str()];
			panel.canChooseFiles = NO;
			panel.canChooseDirectories = YES;
			panel.canCreateDirectories = YES;
			panel.allowsMultipleSelection = NO;
			SetInitialLocation(panel, initial_dir, output, false);
			return CompletePanel(panel, output);
		}
	});
}

bool OpenFileDialog(const std::string &title, const std::vector<FileFilter> &filters, std::string &output, const std::string &initial_dir) {
	return OnMainThread([&] {
		@autoreleasepool {
			[NSApplication sharedApplication];
			NSOpenPanel *panel = [NSOpenPanel openPanel];
			panel.title = [NSString stringWithUTF8String:title.c_str()];
			panel.canChooseFiles = YES;
			panel.canChooseDirectories = NO;
			panel.allowsMultipleSelection = NO;
			SetFilters(panel, filters);
			SetInitialLocation(panel, initial_dir, output, false);
			return CompletePanel(panel, output);
		}
	});
}

bool SaveFileDialog(const std::string &title, const std::vector<FileFilter> &filters, std::string &output, const std::string &initial_dir) {
	return OnMainThread([&] {
		@autoreleasepool {
			[NSApplication sharedApplication];
			NSSavePanel *panel = [NSSavePanel savePanel];
			panel.title = [NSString stringWithUTF8String:title.c_str()];
			panel.canCreateDirectories = YES;
			panel.extensionHidden = NO;
			SetFilters(panel, filters);
			SetInitialLocation(panel, initial_dir, output, true);
			return CompletePanel(panel, output);
		}
	});
}

void DebugBreak() { throw std::runtime_error("HARFANG DebugBreak"); }

} // namespace hg
