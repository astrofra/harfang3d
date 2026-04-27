#define _CRT_SECURE_NO_WARNINGS

/*
    Standalone command-line tool for legacy nArchive packages.
    Based on the Legacy cooker/archive behavior originally authored by
    Emmanuel Julien (https://github.com/ejulien/) and adapted for HARFANG
    from the legacy-cooker reference implementation.

    Provides four subcommands: info, list, unpack, pack.
*/

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#define GS_PATH_SEP '\\'
#define gs_mkdir(path) _mkdir(path)
#define gs_stat _stat
#define GS_STAT_STRUCT struct _stat
#else
#include <dirent.h>
#include <unistd.h>
#define GS_PATH_SEP '/'
#define gs_mkdir(path) mkdir(path, 0777)
#define gs_stat stat
#define GS_STAT_STRUCT struct stat
#endif

#include <miniz.h>

#define GS_MAGIC_ENHANCED 0x4E415244u
#define GS_MAGIC_LEGACY 0x4E415243u
#define GS_METHOD_RAW 0u
#define GS_METHOD_ZLIB 1u
#define GS_MAX_ALIAS_LENGTH 511u

typedef struct GsEntry {
    char *alias;
    uint8_t method;
    uint32_t length;
    uint32_t compressed_length;
    uint32_t data_offset;
} GsEntry;

typedef struct GsArchive {
    char *path;
    const char *revision;
    uint32_t offset_padding;
    uint32_t size_padding;
    uint32_t file_size;
    GsEntry *entries;
    size_t entry_count;
    size_t entry_capacity;
} GsArchive;

typedef struct StringList {
    char **items;
    size_t count;
    size_t capacity;
} StringList;

typedef struct PackOptions {
    int compression_level;
    uint32_t offset_padding;
    uint32_t size_padding;
    int legacy;
    int overwrite;
    int allow_empty;
    StringList excludes;
    StringList raw_patterns;
} PackOptions;

typedef struct UnpackOptions {
    int overwrite;
    StringList includes;
} UnpackOptions;

static void set_error(char *err, size_t err_size, const char *fmt, ...) {
    va_list args;
    if (!err || err_size == 0) {
        return;
    }
    va_start(args, fmt);
    vsnprintf(err, err_size, fmt, args);
    va_end(args);
}

static char *gs_strdup(const char *text) {
    size_t len;
    char *copy;
    if (!text) {
        text = "";
    }
    len = strlen(text);
    copy = (char *)malloc(len + 1);
    if (!copy) {
        return NULL;
    }
    memcpy(copy, text, len + 1);
    return copy;
}

static int string_list_append(StringList *list, const char *text) {
    char **new_items;
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity ? list->capacity * 2 : 16;
        new_items = (char **)realloc(list->items, new_capacity * sizeof(char *));
        if (!new_items) {
            return 0;
        }
        list->items = new_items;
        list->capacity = new_capacity;
    }
    list->items[list->count] = gs_strdup(text);
    if (!list->items[list->count]) {
        return 0;
    }
    list->count++;
    return 1;
}

static void string_list_free(StringList *list) {
    size_t i;
    for (i = 0; i < list->count; ++i) {
        free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static int is_path_sep(char c) {
    return c == '/' || c == '\\';
}

static char *join_path(const char *left, const char *right) {
    size_t llen = strlen(left);
    size_t rlen = strlen(right);
    int needs_sep = llen > 0 && !is_path_sep(left[llen - 1]);
    char *out = (char *)malloc(llen + (needs_sep ? 1 : 0) + rlen + 1);
    if (!out) {
        return NULL;
    }
    memcpy(out, left, llen);
    if (needs_sep) {
        out[llen++] = GS_PATH_SEP;
    }
    memcpy(out + llen, right, rlen + 1);
    return out;
}

static void normalize_alias(char *path) {
    while (*path) {
        if (*path == '\\') {
            *path = '/';
        }
        ++path;
    }
}

static int wildcard_match(const char *pattern, const char *text) {
    const char *star = NULL;
    const char *retry = NULL;

    while (*text) {
        if (*pattern == '?' || *pattern == *text) {
            ++pattern;
            ++text;
        } else if (*pattern == '*') {
            star = pattern++;
            retry = text;
        } else if (star) {
            pattern = star + 1;
            text = ++retry;
        } else {
            return 0;
        }
    }

    while (*pattern == '*') {
        ++pattern;
    }
    return *pattern == 0;
}

static int matches_any(const StringList *patterns, const char *text) {
    size_t i;
    for (i = 0; i < patterns->count; ++i) {
        if (wildcard_match(patterns->items[i], text)) {
            return 1;
        }
    }
    return 0;
}

static uint32_t align_position(uint32_t position, uint32_t padding) {
    uint32_t error;
    if (!padding) {
        return position;
    }
    error = position % padding;
    return error ? position + padding - error : position;
}

static int seek_to_u32(FILE *file, uint32_t position) {
    return fseek(file, (long)position, SEEK_SET) == 0;
}

static int align_read(FILE *file, uint32_t padding) {
    long pos = ftell(file);
    uint32_t target;
    if (pos < 0) {
        return 0;
    }
    target = align_position((uint32_t)pos, padding);
    if (target != (uint32_t)pos) {
        return seek_to_u32(file, target);
    }
    return 1;
}

static int align_write(FILE *file, uint32_t padding) {
    long pos = ftell(file);
    uint32_t target;
    uint32_t gap;
    if (pos < 0) {
        return 0;
    }
    target = align_position((uint32_t)pos, padding);
    gap = target - (uint32_t)pos;
    while (gap--) {
        if (fputc(0, file) == EOF) {
            return 0;
        }
    }
    return 1;
}

static int read_u32(FILE *file, uint32_t *out) {
    unsigned char b[4];
    if (fread(b, 1, 4, file) != 4) {
        return 0;
    }
    *out = ((uint32_t)b[0]) |
           ((uint32_t)b[1] << 8) |
           ((uint32_t)b[2] << 16) |
           ((uint32_t)b[3] << 24);
    return 1;
}

static int write_u32(FILE *file, uint32_t value) {
    unsigned char b[4];
    b[0] = (unsigned char)(value & 0xffu);
    b[1] = (unsigned char)((value >> 8) & 0xffu);
    b[2] = (unsigned char)((value >> 16) & 0xffu);
    b[3] = (unsigned char)((value >> 24) & 0xffu);
    return fwrite(b, 1, 4, file) == 4;
}

static int file_exists(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return 0;
    }
    fclose(file);
    return 1;
}

static char *absolute_path(const char *path) {
#ifdef _WIN32
    DWORD needed = GetFullPathNameA(path, 0, NULL, NULL);
    char *buffer;
    if (!needed) {
        return gs_strdup(path);
    }
    buffer = (char *)malloc((size_t)needed + 1);
    if (!buffer) {
        return NULL;
    }
    if (!GetFullPathNameA(path, needed, buffer, NULL)) {
        free(buffer);
        return gs_strdup(path);
    }
    return buffer;
#else
    char *resolved = realpath(path, NULL);
    if (resolved) {
        return resolved;
    }
    if (path[0] == '/') {
        return gs_strdup(path);
    } else {
        char cwd[4096];
        char *joined;
        if (!getcwd(cwd, sizeof(cwd))) {
            return gs_strdup(path);
        }
        joined = join_path(cwd, path);
        return joined;
    }
#endif
}

static void normalize_compare_path(char *path) {
    char *start = path;
    size_t len;
    while (*path) {
        if (*path == '\\') {
            *path = '/';
        }
#ifdef _WIN32
        *path = (char)tolower((unsigned char)*path);
#endif
        ++path;
    }
    len = strlen(start);
    while (len > 1 && start[len - 1] == '/') {
        start[--len] = 0;
    }
}

static int same_filesystem_path(const char *left, const char *right) {
    char *left_abs = absolute_path(left);
    char *right_abs = absolute_path(right);
    int same;
    if (!left_abs || !right_abs) {
        free(left_abs);
        free(right_abs);
        return 0;
    }
    normalize_compare_path(left_abs);
    normalize_compare_path(right_abs);
    same = strcmp(left_abs, right_abs) == 0;
    free(left_abs);
    free(right_abs);
    return same;
}

static int path_is_dir(const char *path) {
    GS_STAT_STRUCT st;
    if (gs_stat(path, &st) != 0) {
        return 0;
    }
#ifdef _WIN32
    return (st.st_mode & _S_IFDIR) != 0;
#else
    return S_ISDIR(st.st_mode);
#endif
}

static int get_file_size(FILE *file, uint32_t *out_size) {
    long end;
    long current = ftell(file);
    if (current < 0 || fseek(file, 0, SEEK_END) != 0) {
        return 0;
    }
    end = ftell(file);
    if (end < 0 || end > 0xFFFFFFFFL || fseek(file, current, SEEK_SET) != 0) {
        return 0;
    }
    *out_size = (uint32_t)end;
    return 1;
}

static int archive_add_entry(GsArchive *archive, GsEntry entry) {
    GsEntry *new_entries;
    if (archive->entry_count == archive->entry_capacity) {
        size_t new_capacity = archive->entry_capacity ? archive->entry_capacity * 2 : 128;
        new_entries = (GsEntry *)realloc(archive->entries, new_capacity * sizeof(GsEntry));
        if (!new_entries) {
            return 0;
        }
        archive->entries = new_entries;
        archive->entry_capacity = new_capacity;
    }
    archive->entries[archive->entry_count++] = entry;
    return 1;
}

static void archive_free(GsArchive *archive) {
    size_t i;
    for (i = 0; i < archive->entry_count; ++i) {
        free(archive->entries[i].alias);
    }
    free(archive->entries);
    free(archive->path);
    memset(archive, 0, sizeof(*archive));
}

static uint32_t entry_stored_length(const GsEntry *entry) {
    return entry->method == GS_METHOD_ZLIB ? entry->compressed_length : entry->length;
}

static const char *entry_method_name(const GsEntry *entry) {
    return entry->method == GS_METHOD_ZLIB ? "Zlib" : "Raw";
}

static int scan_archive(const char *path, GsArchive *archive, char *err, size_t err_size) {
    FILE *file;
    uint32_t magic;

    memset(archive, 0, sizeof(*archive));
    archive->path = gs_strdup(path);
    if (!archive->path) {
        set_error(err, err_size, "out of memory");
        return 0;
    }

    file = fopen(path, "rb");
    if (!file) {
        set_error(err, err_size, "could not open archive '%s': %s", path, strerror(errno));
        return 0;
    }
    if (!get_file_size(file, &archive->file_size)) {
        fclose(file);
        set_error(err, err_size, "could not determine archive size");
        return 0;
    }
    if (!read_u32(file, &magic)) {
        fclose(file);
        set_error(err, err_size, "could not read archive magic");
        return 0;
    }

    if (magic == GS_MAGIC_ENHANCED) {
        archive->revision = "EnhancedLegacy";
        if (!read_u32(file, &archive->offset_padding) || !read_u32(file, &archive->size_padding)) {
            fclose(file);
            set_error(err, err_size, "truncated enhanced archive header");
            return 0;
        }
    } else if (magic == GS_MAGIC_LEGACY) {
        archive->revision = "Legacy";
        archive->offset_padding = 0;
        archive->size_padding = 0;
    } else {
        fclose(file);
        set_error(err, err_size, "invalid Legacy archive magic 0x%08x", magic);
        return 0;
    }

    while ((uint32_t)ftell(file) < archive->file_size) {
        long unaligned_pos;
        unsigned char marker[4];
        uint32_t alias_length;
        char *alias;
        int method_raw;
        uint32_t length;
        uint32_t compressed_length;
        uint32_t data_offset;
        GsEntry entry;

        unaligned_pos = ftell(file);
        if (unaligned_pos < 0) {
            fclose(file);
            set_error(err, err_size, "could not query archive cursor");
            return 0;
        }
        if (fread(marker, 1, 4, file) == 4) {
            if (marker[0] == 0xff && marker[1] == 0xff && marker[2] == 0xff && marker[3] == 0xff) {
                break;
            }
        }
        if (fseek(file, unaligned_pos, SEEK_SET) != 0) {
            fclose(file);
            set_error(err, err_size, "could not seek archive");
            return 0;
        }

        if (!align_read(file, archive->offset_padding)) {
            fclose(file);
            set_error(err, err_size, "could not align archive cursor");
            return 0;
        }
        if ((uint32_t)ftell(file) >= archive->file_size) {
            break;
        }

        if (!read_u32(file, &alias_length)) {
            fclose(file);
            set_error(err, err_size, "truncated entry alias length");
            return 0;
        }
        if (alias_length > GS_MAX_ALIAS_LENGTH) {
            break;
        }

        if (!align_read(file, archive->offset_padding)) {
            fclose(file);
            set_error(err, err_size, "could not align before alias");
            return 0;
        }
        alias = (char *)malloc((size_t)alias_length + 1);
        if (!alias) {
            fclose(file);
            set_error(err, err_size, "out of memory");
            return 0;
        }
        if (fread(alias, 1, alias_length, file) != alias_length) {
            free(alias);
            fclose(file);
            set_error(err, err_size, "truncated entry alias");
            return 0;
        }
        alias[alias_length] = 0;
        normalize_alias(alias);

        if (!align_read(file, archive->offset_padding)) {
            free(alias);
            fclose(file);
            set_error(err, err_size, "could not align before method");
            return 0;
        }
        method_raw = fgetc(file);
        if (method_raw == EOF) {
            free(alias);
            fclose(file);
            set_error(err, err_size, "truncated method for entry '%s'", alias);
            return 0;
        }

        if (!align_read(file, archive->offset_padding) || !read_u32(file, &length)) {
            free(alias);
            fclose(file);
            set_error(err, err_size, "truncated length for entry '%s'", alias);
            return 0;
        }

        if ((method_raw & 1) == GS_METHOD_ZLIB) {
            if (!align_read(file, archive->offset_padding) || !read_u32(file, &compressed_length)) {
                free(alias);
                fclose(file);
                set_error(err, err_size, "truncated compressed length for entry '%s'", alias);
                return 0;
            }
        } else {
            compressed_length = length;
        }

        if (!align_read(file, archive->offset_padding)) {
            free(alias);
            fclose(file);
            set_error(err, err_size, "could not align before payload for entry '%s'", alias);
            return 0;
        }
        data_offset = (uint32_t)ftell(file);
        if (data_offset > archive->file_size || compressed_length > archive->file_size - data_offset) {
            free(alias);
            fclose(file);
            set_error(err, err_size, "entry payload extends past end of archive");
            return 0;
        }

        entry.alias = alias;
        entry.method = (uint8_t)(method_raw & 1);
        entry.length = length;
        entry.compressed_length = compressed_length;
        entry.data_offset = data_offset;
        if (!archive_add_entry(archive, entry)) {
            free(alias);
            fclose(file);
            set_error(err, err_size, "out of memory");
            return 0;
        }

        if (fseek(file, (long)compressed_length, SEEK_CUR) != 0) {
            fclose(file);
            set_error(err, err_size, "could not skip payload");
            return 0;
        }
    }

    fclose(file);
    return 1;
}

static int read_entry_data(FILE *file, const GsEntry *entry, unsigned char **out, uint32_t *out_len, char *err, size_t err_size) {
    uint32_t stored_length = entry_stored_length(entry);
    unsigned char *payload = NULL;
    unsigned char *data = NULL;

    if (fseek(file, (long)entry->data_offset, SEEK_SET) != 0) {
        set_error(err, err_size, "could not seek to entry '%s'", entry->alias);
        return 0;
    }

    payload = (unsigned char *)malloc(stored_length ? stored_length : 1);
    if (!payload) {
        set_error(err, err_size, "out of memory");
        return 0;
    }
    if (stored_length && fread(payload, 1, stored_length, file) != stored_length) {
        free(payload);
        set_error(err, err_size, "could not read entry '%s'", entry->alias);
        return 0;
    }

    if (entry->method == GS_METHOD_ZLIB) {
        uLongf decoded_length = (uLongf)entry->length;
        data = (unsigned char *)malloc(entry->length ? entry->length : 1);
        if (!data) {
            free(payload);
            set_error(err, err_size, "out of memory");
            return 0;
        }
        if (uncompress((Bytef *)data, &decoded_length, (const Bytef *)payload, (uLong)stored_length) != Z_OK ||
            decoded_length != entry->length) {
            free(payload);
            free(data);
            set_error(err, err_size, "could not decompress entry '%s'", entry->alias);
            return 0;
        }
        free(payload);
        *out = data;
        *out_len = entry->length;
        return 1;
    }

    *out = payload;
    *out_len = entry->length;
    return 1;
}

static int mkdir_if_needed(const char *path) {
    if (!path || !*path) {
        return 1;
    }
    if (gs_mkdir(path) == 0) {
        return 1;
    }
    return errno == EEXIST;
}

static int ensure_parent_dirs(const char *path) {
    char *tmp = gs_strdup(path);
    size_t i;
    if (!tmp) {
        return 0;
    }

    for (i = 0; tmp[i]; ++i) {
        char saved;
        if (!is_path_sep(tmp[i])) {
            continue;
        }
        if (i == 0 || (i == 2 && tmp[1] == ':')) {
            continue;
        }
        saved = tmp[i];
        tmp[i] = 0;
        if (!mkdir_if_needed(tmp)) {
            free(tmp);
            return 0;
        }
        tmp[i] = saved;
    }

    free(tmp);
    return 1;
}

static int build_output_path(const char *output_dir, const char *alias, char **out, char *err, size_t err_size) {
    char *alias_copy;
    char *p;
    size_t out_len;
    size_t alias_len;
    char *target;
    size_t i;

    if (!alias || !*alias || is_path_sep(alias[0]) || (strlen(alias) >= 2 && alias[1] == ':') || strchr(alias, ':')) {
        set_error(err, err_size, "unsafe archive path '%s'", alias ? alias : "");
        return 0;
    }

    alias_copy = gs_strdup(alias);
    if (!alias_copy) {
        set_error(err, err_size, "out of memory");
        return 0;
    }
    normalize_alias(alias_copy);

    p = alias_copy;
    while (*p) {
        char *start = p;
        size_t len;
        while (*p && *p != '/') {
            ++p;
        }
        len = (size_t)(p - start);
        if (len == 0 || (len == 1 && start[0] == '.') || (len == 2 && start[0] == '.' && start[1] == '.')) {
            free(alias_copy);
            set_error(err, err_size, "unsafe archive path '%s'", alias);
            return 0;
        }
        if (*p == '/') {
            ++p;
        }
    }

    out_len = strlen(output_dir);
    alias_len = strlen(alias_copy);
    target = (char *)malloc(out_len + 1 + alias_len + 1);
    if (!target) {
        free(alias_copy);
        set_error(err, err_size, "out of memory");
        return 0;
    }
    memcpy(target, output_dir, out_len);
    if (out_len && !is_path_sep(target[out_len - 1])) {
        target[out_len++] = GS_PATH_SEP;
    }
    memcpy(target + out_len, alias_copy, alias_len + 1);
    for (i = out_len; target[i]; ++i) {
        if (target[i] == '/') {
            target[i] = GS_PATH_SEP;
        }
    }
    free(alias_copy);
    *out = target;
    return 1;
}

static int unpack_archive_cmd(const char *archive_path, const char *output_dir, const UnpackOptions *opts, char *err, size_t err_size) {
    GsArchive archive;
    FILE *file;
    size_t i;
    size_t written = 0;

    if (!scan_archive(archive_path, &archive, err, err_size)) {
        return 0;
    }
    if (!mkdir_if_needed(output_dir)) {
        archive_free(&archive);
        set_error(err, err_size, "could not create output directory '%s'", output_dir);
        return 0;
    }

    file = fopen(archive_path, "rb");
    if (!file) {
        archive_free(&archive);
        set_error(err, err_size, "could not reopen archive '%s'", archive_path);
        return 0;
    }

    for (i = 0; i < archive.entry_count; ++i) {
        const GsEntry *entry = &archive.entries[i];
        unsigned char *data = NULL;
        uint32_t data_len = 0;
        char *target = NULL;
        FILE *out_file;

        if (opts->includes.count && !matches_any(&opts->includes, entry->alias)) {
            continue;
        }
        if (!build_output_path(output_dir, entry->alias, &target, err, err_size)) {
            fclose(file);
            archive_free(&archive);
            return 0;
        }
        if (!opts->overwrite && file_exists(target)) {
            set_error(err, err_size, "refusing to overwrite existing file '%s'", target);
            free(target);
            fclose(file);
            archive_free(&archive);
            return 0;
        }
        if (!ensure_parent_dirs(target)) {
            set_error(err, err_size, "could not create parent directories for '%s'", target);
            free(target);
            fclose(file);
            archive_free(&archive);
            return 0;
        }
        if (!read_entry_data(file, entry, &data, &data_len, err, err_size)) {
            free(target);
            fclose(file);
            archive_free(&archive);
            return 0;
        }

        out_file = fopen(target, "wb");
        if (!out_file) {
            set_error(err, err_size, "could not write '%s': %s", target, strerror(errno));
            free(data);
            free(target);
            fclose(file);
            archive_free(&archive);
            return 0;
        }
        if (data_len && fwrite(data, 1, data_len, out_file) != data_len) {
            set_error(err, err_size, "short write for '%s'", target);
            fclose(out_file);
            free(data);
            free(target);
            fclose(file);
            archive_free(&archive);
            return 0;
        }
        fclose(out_file);
        free(data);
        free(target);
        ++written;
    }

    fclose(file);
    printf("Unpacked %u file(s) to %s\n", (unsigned)written, output_dir);
    archive_free(&archive);
    return 1;
}

static int read_whole_file(const char *path, unsigned char **out, uint32_t *out_len, char *err, size_t err_size) {
    FILE *file = fopen(path, "rb");
    uint32_t size;
    unsigned char *data;
    if (!file) {
        set_error(err, err_size, "could not open '%s': %s", path, strerror(errno));
        return 0;
    }
    if (!get_file_size(file, &size)) {
        fclose(file);
        set_error(err, err_size, "could not determine size of '%s'", path);
        return 0;
    }
    data = (unsigned char *)malloc(size ? size : 1);
    if (!data) {
        fclose(file);
        set_error(err, err_size, "out of memory");
        return 0;
    }
    if (size && fread(data, 1, size, file) != size) {
        free(data);
        fclose(file);
        set_error(err, err_size, "could not read '%s'", path);
        return 0;
    }
    fclose(file);
    *out = data;
    *out_len = size;
    return 1;
}

static int string_compare_alias(const void *left, const void *right) {
    const char *a = *(const char *const *)left;
    const char *b = *(const char *const *)right;
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) {
            return ca - cb;
        }
        ++a;
        ++b;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

static int collect_files_recursive(const char *root, const char *rel, StringList *files, const StringList *excludes, char *err, size_t err_size) {
    char *dir_path = rel && *rel ? join_path(root, rel) : gs_strdup(root);
    if (!dir_path) {
        set_error(err, err_size, "out of memory");
        return 0;
    }

#ifdef _WIN32
    {
        char *pattern = join_path(dir_path, "*");
        WIN32_FIND_DATAA fd;
        HANDLE handle;
        if (!pattern) {
            free(dir_path);
            set_error(err, err_size, "out of memory");
            return 0;
        }
        handle = FindFirstFileA(pattern, &fd);
        free(pattern);
        if (handle == INVALID_HANDLE_VALUE) {
            free(dir_path);
            set_error(err, err_size, "could not read directory '%s'", dir_path);
            return 0;
        }
        do {
            char *child_rel;
            size_t rel_len;
            if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) {
                continue;
            }
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                continue;
            }
            rel_len = rel && *rel ? strlen(rel) : 0;
            child_rel = (char *)malloc(rel_len + (rel_len ? 1 : 0) + strlen(fd.cFileName) + 1);
            if (!child_rel) {
                FindClose(handle);
                free(dir_path);
                set_error(err, err_size, "out of memory");
                return 0;
            }
            if (rel_len) {
                memcpy(child_rel, rel, rel_len);
                child_rel[rel_len++] = '/';
            }
            strcpy(child_rel + rel_len, fd.cFileName);
            normalize_alias(child_rel);

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (!collect_files_recursive(root, child_rel, files, excludes, err, err_size)) {
                    free(child_rel);
                    FindClose(handle);
                    free(dir_path);
                    return 0;
                }
            } else if (!matches_any(excludes, child_rel)) {
                if (!string_list_append(files, child_rel)) {
                    free(child_rel);
                    FindClose(handle);
                    free(dir_path);
                    set_error(err, err_size, "out of memory");
                    return 0;
                }
            }
            free(child_rel);
        } while (FindNextFileA(handle, &fd));
        FindClose(handle);
    }
#else
    {
        DIR *dir = opendir(dir_path);
        struct dirent *entry;
        if (!dir) {
            free(dir_path);
            set_error(err, err_size, "could not read directory '%s'", dir_path);
            return 0;
        }
        while ((entry = readdir(dir)) != NULL) {
            char *child_rel;
            char *child_path;
            size_t rel_len;
            GS_STAT_STRUCT st;
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            rel_len = rel && *rel ? strlen(rel) : 0;
            child_rel = (char *)malloc(rel_len + (rel_len ? 1 : 0) + strlen(entry->d_name) + 1);
            if (!child_rel) {
                closedir(dir);
                free(dir_path);
                set_error(err, err_size, "out of memory");
                return 0;
            }
            if (rel_len) {
                memcpy(child_rel, rel, rel_len);
                child_rel[rel_len++] = '/';
            }
            strcpy(child_rel + rel_len, entry->d_name);
            child_path = join_path(root, child_rel);
            if (!child_path) {
                free(child_rel);
                closedir(dir);
                free(dir_path);
                set_error(err, err_size, "out of memory");
                return 0;
            }
            if (lstat(child_path, &st) != 0) {
                free(child_path);
                free(child_rel);
                continue;
            }
            if (S_ISLNK(st.st_mode)) {
                free(child_path);
                free(child_rel);
                continue;
            }
            if (S_ISDIR(st.st_mode)) {
                if (!collect_files_recursive(root, child_rel, files, excludes, err, err_size)) {
                    free(child_path);
                    free(child_rel);
                    closedir(dir);
                    free(dir_path);
                    return 0;
                }
            } else if (S_ISREG(st.st_mode) && !matches_any(excludes, child_rel)) {
                if (!string_list_append(files, child_rel)) {
                    free(child_path);
                    free(child_rel);
                    closedir(dir);
                    free(dir_path);
                    set_error(err, err_size, "out of memory");
                    return 0;
                }
            }
            free(child_path);
            free(child_rel);
        }
        closedir(dir);
    }
#endif

    free(dir_path);
    return 1;
}

static int write_entry(FILE *out, const char *alias, const unsigned char *data, uint32_t len, int compression_level, uint32_t offset_padding, GsEntry *entry, char *err, size_t err_size) {
    unsigned char *payload = NULL;
    uint32_t payload_len = len;
    uint8_t method = GS_METHOD_RAW;
    size_t alias_len = strlen(alias);

    if (alias_len > GS_MAX_ALIAS_LENGTH) {
        set_error(err, err_size, "archive alias exceeds %u bytes: %s", GS_MAX_ALIAS_LENGTH, alias);
        return 0;
    }

    if (compression_level >= 0) {
        uLongf bound = compressBound((uLong)len);
        payload = (unsigned char *)malloc(bound ? (size_t)bound : 1);
        if (!payload) {
            set_error(err, err_size, "out of memory");
            return 0;
        }
        if (compress2((Bytef *)payload, &bound, (const Bytef *)data, (uLong)len, compression_level) != Z_OK) {
            free(payload);
            set_error(err, err_size, "zlib compression failed for '%s'", alias);
            return 0;
        }
        payload_len = (uint32_t)bound;
        method = GS_METHOD_ZLIB;
    } else {
        payload = (unsigned char *)data;
        payload_len = len;
        method = GS_METHOD_RAW;
    }

    if (!align_write(out, offset_padding) || !write_u32(out, (uint32_t)alias_len) ||
        !align_write(out, offset_padding) || fwrite(alias, 1, alias_len, out) != alias_len ||
        !align_write(out, offset_padding) || fputc(method, out) == EOF ||
        !align_write(out, offset_padding) || !write_u32(out, len)) {
        if (compression_level >= 0) {
            free(payload);
        }
        set_error(err, err_size, "could not write archive entry '%s'", alias);
        return 0;
    }

    if (method == GS_METHOD_ZLIB) {
        if (!align_write(out, offset_padding) || !write_u32(out, payload_len)) {
            free(payload);
            set_error(err, err_size, "could not write compressed length for '%s'", alias);
            return 0;
        }
    }

    if (!align_write(out, offset_padding)) {
        if (compression_level >= 0) {
            free(payload);
        }
        set_error(err, err_size, "could not align payload for '%s'", alias);
        return 0;
    }

    entry->alias = gs_strdup(alias);
    entry->method = method;
    entry->length = len;
    entry->compressed_length = payload_len;
    entry->data_offset = (uint32_t)ftell(out);

    if (!entry->alias || (payload_len && fwrite(payload, 1, payload_len, out) != payload_len)) {
        free(entry->alias);
        entry->alias = NULL;
        if (compression_level >= 0) {
            free(payload);
        }
        set_error(err, err_size, "could not write payload for '%s'", alias);
        return 0;
    }

    if (compression_level >= 0) {
        free(payload);
    }
    return 1;
}

static int pack_archive_cmd(const char *input_dir, const char *archive_path, const PackOptions *opts, char *err, size_t err_size) {
    FILE *out;
    StringList files = {0};
    GsArchive written = {0};
    uint32_t effective_padding;
    size_t i;
    unsigned skipped_empty = 0;
    unsigned skipped_output_archive = 0;

    if (!path_is_dir(input_dir)) {
        set_error(err, err_size, "input is not a directory: %s", input_dir);
        return 0;
    }
    if (opts->compression_level < -1 || opts->compression_level > 9) {
        set_error(err, err_size, "compression level must be -1 or 0..9");
        return 0;
    }
    if (file_exists(archive_path) && !opts->overwrite) {
        set_error(err, err_size, "refusing to overwrite existing archive '%s'", archive_path);
        return 0;
    }
    if (!ensure_parent_dirs(archive_path)) {
        set_error(err, err_size, "could not create parent directories for '%s'", archive_path);
        return 0;
    }
    if (!collect_files_recursive(input_dir, "", &files, &opts->excludes, err, err_size)) {
        string_list_free(&files);
        return 0;
    }
    qsort(files.items, files.count, sizeof(char *), string_compare_alias);

    out = fopen(archive_path, "wb");
    if (!out) {
        string_list_free(&files);
        set_error(err, err_size, "could not create archive '%s': %s", archive_path, strerror(errno));
        return 0;
    }

    if (opts->legacy) {
        if (!write_u32(out, GS_MAGIC_LEGACY)) {
            fclose(out);
            string_list_free(&files);
            set_error(err, err_size, "could not write archive header");
            return 0;
        }
        effective_padding = 0;
    } else {
        if (!write_u32(out, GS_MAGIC_ENHANCED) || !write_u32(out, opts->offset_padding) || !write_u32(out, opts->size_padding)) {
            fclose(out);
            string_list_free(&files);
            set_error(err, err_size, "could not write archive header");
            return 0;
        }
        effective_padding = opts->offset_padding;
    }

    for (i = 0; i < files.count; ++i) {
        char *full_path = join_path(input_dir, files.items[i]);
        unsigned char *data = NULL;
        uint32_t data_len = 0;
        int entry_level;
        GsEntry entry;

        if (!full_path) {
            fclose(out);
            string_list_free(&files);
            archive_free(&written);
            set_error(err, err_size, "out of memory");
            return 0;
        }
        if (same_filesystem_path(full_path, archive_path)) {
            free(full_path);
            skipped_output_archive++;
            continue;
        }
        if (!read_whole_file(full_path, &data, &data_len, err, err_size)) {
            free(full_path);
            fclose(out);
            string_list_free(&files);
            archive_free(&written);
            return 0;
        }
        free(full_path);

        if (!data_len && !opts->allow_empty) {
            free(data);
            skipped_empty++;
            continue;
        }

        entry_level = matches_any(&opts->raw_patterns, files.items[i]) ? -1 : opts->compression_level;
        memset(&entry, 0, sizeof(entry));
        if (!write_entry(out, files.items[i], data, data_len, entry_level, effective_padding, &entry, err, err_size) ||
            !archive_add_entry(&written, entry)) {
            free(entry.alias);
            free(data);
            fclose(out);
            string_list_free(&files);
            archive_free(&written);
            return 0;
        }
        free(data);
    }

    if (!write_u32(out, 0xFFFFFFFFu)) {
        fclose(out);
        string_list_free(&files);
        archive_free(&written);
        set_error(err, err_size, "could not write archive EOF marker");
        return 0;
    }

    if (!opts->legacy && opts->size_padding) {
        long size = ftell(out);
        uint32_t pad_count;
        if (size < 0) {
            fclose(out);
            string_list_free(&files);
            archive_free(&written);
            set_error(err, err_size, "could not query archive size");
            return 0;
        }
        pad_count = opts->size_padding - ((uint32_t)size % opts->size_padding);
        while (pad_count--) {
            if (fputc(0xff, out) == EOF) {
                fclose(out);
                string_list_free(&files);
                archive_free(&written);
                set_error(err, err_size, "could not write archive size padding");
                return 0;
            }
        }
    }

    fclose(out);
    printf("Packed %u file(s) into %s\n", (unsigned)written.entry_count, archive_path);
    if (skipped_empty) {
        printf("Skipped %u empty file(s). Use --allow-empty to store them.\n", skipped_empty);
    }
    if (skipped_output_archive) {
        printf("Skipped %u output archive file(s).\n", skipped_output_archive);
    }

    string_list_free(&files);
    archive_free(&written);
    return 1;
}

static void print_size(uint32_t value) {
    double amount = (double)value;
    const char *units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    while (amount >= 1024.0 && unit < 3) {
        amount /= 1024.0;
        ++unit;
    }
    if (unit == 0) {
        printf("%u B", value);
    } else {
        printf("%.2f %s", amount, units[unit]);
    }
}

static int info_cmd(const char *archive_path, char *err, size_t err_size) {
    GsArchive archive;
    uint32_t original = 0;
    uint32_t stored = 0;
    size_t i;
    if (!scan_archive(archive_path, &archive, err, err_size)) {
        return 0;
    }
    for (i = 0; i < archive.entry_count; ++i) {
        original += archive.entries[i].length;
        stored += entry_stored_length(&archive.entries[i]);
    }
    printf("Archive: %s\n", archive.path);
    printf("Revision: %s\n", archive.revision);
    printf("Offset padding: %u\n", archive.offset_padding);
    printf("Size padding: %u\n", archive.size_padding);
    printf("Entries: %u\n", (unsigned)archive.entry_count);
    printf("Original data: ");
    print_size(original);
    printf("\nStored data: ");
    print_size(stored);
    printf("\nFile size: ");
    print_size(archive.file_size);
    printf("\n");
    archive_free(&archive);
    return 1;
}

static void print_json_string(const char *text) {
    putchar('"');
    while (*text) {
        unsigned char c = (unsigned char)*text++;
        switch (c) {
            case '\\': printf("\\\\"); break;
            case '"': printf("\\\""); break;
            case '\b': printf("\\b"); break;
            case '\f': printf("\\f"); break;
            case '\n': printf("\\n"); break;
            case '\r': printf("\\r"); break;
            case '\t': printf("\\t"); break;
            default:
                if (c < 32) {
                    printf("\\u%04x", c);
                } else {
                    putchar((int)c);
                }
                break;
        }
    }
    putchar('"');
}

static int list_cmd(const char *archive_path, int names_only, int as_json, const StringList *includes, char *err, size_t err_size) {
    GsArchive archive;
    size_t i;
    int first = 1;
    if (!scan_archive(archive_path, &archive, err, err_size)) {
        return 0;
    }

    if (as_json) {
        printf("{\n  \"archive\": ");
        print_json_string(archive.path);
        printf(",\n  \"revision\": ");
        print_json_string(archive.revision);
        printf(",\n  \"offset_padding\": %u,\n  \"size_padding\": %u,\n  \"file_size\": %u,\n  \"entries\": [\n",
               archive.offset_padding, archive.size_padding, archive.file_size);
        for (i = 0; i < archive.entry_count; ++i) {
            const GsEntry *entry = &archive.entries[i];
            if (includes->count && !matches_any(includes, entry->alias)) {
                continue;
            }
            if (!first) {
                printf(",\n");
            }
            first = 0;
            printf("    {\"alias\": ");
            print_json_string(entry->alias);
            printf(", \"method\": ");
            print_json_string(entry_method_name(entry));
            printf(", \"offset\": %u, \"length\": %u, \"compressed_length\": %u, \"stored_length\": %u}",
                   entry->data_offset, entry->length, entry->compressed_length, entry_stored_length(entry));
        }
        printf("\n  ]\n}\n");
        archive_free(&archive);
        return 1;
    }

    if (!names_only) {
        printf("%-6s %12s %12s %8s %12s %s\n", "method", "size", "stored", "ratio", "offset", "path");
    }
    for (i = 0; i < archive.entry_count; ++i) {
        const GsEntry *entry = &archive.entries[i];
        if (includes->count && !matches_any(includes, entry->alias)) {
            continue;
        }
        if (names_only) {
            printf("%s\n", entry->alias);
        } else {
            double ratio = entry->length ? (100.0 * (double)entry_stored_length(entry) / (double)entry->length) : 0.0;
            printf("%-6s %12u %12u %7.1f%% %12u %s\n",
                   entry_method_name(entry), entry->length, entry_stored_length(entry), ratio, entry->data_offset, entry->alias);
        }
    }

    archive_free(&archive);
    return 1;
}

static void usage(void) {
    printf("usage: legacy_archive <command> [options]\n\n");
    printf("Commands:\n");
    printf("  info <archive>\n");
    printf("  list [--json] [-n|--names-only] [--include PATTERN] <archive>\n");
    printf("  unpack [-f|--overwrite] [--include PATTERN] <archive> <output_dir>\n");
    printf("  pack [-f|--overwrite] [-c LEVEL] [--offset-padding N] [--size-padding N]\n");
    printf("       [--legacy] [--exclude PATTERN] [--raw PATTERN] [--allow-empty]\n");
    printf("       <input_dir> <archive>\n");
}

static int require_value(int argc, char **argv, int *i, const char *option, char *err, size_t err_size) {
    if (*i + 1 >= argc) {
        set_error(err, err_size, "missing value for %s", option);
        return 0;
    }
    ++(*i);
    (void)argv;
    return 1;
}

int main(int argc, char **argv) {
    char err[1024] = {0};
    const char *cmd;

    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        usage();
        return argc < 2 ? 1 : 0;
    }

    cmd = argv[1];
    if (strcmp(cmd, "info") == 0) {
        if (argc != 3) {
            usage();
            return 1;
        }
        if (!info_cmd(argv[2], err, sizeof(err))) {
            fprintf(stderr, "error: %s\n", err);
            return 1;
        }
        return 0;
    }

    if (strcmp(cmd, "list") == 0) {
        int names_only = 0;
        int as_json = 0;
        const char *archive_path = NULL;
        StringList includes = {0};
        int i;
        int ok;
        for (i = 2; i < argc; ++i) {
            if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--names-only") == 0) {
                names_only = 1;
            } else if (strcmp(argv[i], "--json") == 0) {
                as_json = 1;
            } else if (strcmp(argv[i], "--include") == 0) {
                if (!require_value(argc, argv, &i, "--include", err, sizeof(err)) || !string_list_append(&includes, argv[i])) {
                    fprintf(stderr, "error: %s\n", err[0] ? err : "out of memory");
                    string_list_free(&includes);
                    return 1;
                }
            } else if (!archive_path) {
                archive_path = argv[i];
            } else {
                usage();
                string_list_free(&includes);
                return 1;
            }
        }
        if (!archive_path) {
            usage();
            string_list_free(&includes);
            return 1;
        }
        ok = list_cmd(archive_path, names_only, as_json, &includes, err, sizeof(err));
        string_list_free(&includes);
        if (!ok) {
            fprintf(stderr, "error: %s\n", err);
            return 1;
        }
        return 0;
    }

    if (strcmp(cmd, "unpack") == 0) {
        UnpackOptions opts;
        const char *archive_path = NULL;
        const char *output_dir = NULL;
        int i;
        int ok;
        memset(&opts, 0, sizeof(opts));
        for (i = 2; i < argc; ++i) {
            if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--overwrite") == 0) {
                opts.overwrite = 1;
            } else if (strcmp(argv[i], "--include") == 0) {
                if (!require_value(argc, argv, &i, "--include", err, sizeof(err)) || !string_list_append(&opts.includes, argv[i])) {
                    fprintf(stderr, "error: %s\n", err[0] ? err : "out of memory");
                    string_list_free(&opts.includes);
                    return 1;
                }
            } else if (!archive_path) {
                archive_path = argv[i];
            } else if (!output_dir) {
                output_dir = argv[i];
            } else {
                usage();
                string_list_free(&opts.includes);
                return 1;
            }
        }
        if (!archive_path || !output_dir) {
            usage();
            string_list_free(&opts.includes);
            return 1;
        }
        ok = unpack_archive_cmd(archive_path, output_dir, &opts, err, sizeof(err));
        string_list_free(&opts.includes);
        if (!ok) {
            fprintf(stderr, "error: %s\n", err);
            return 1;
        }
        return 0;
    }

    if (strcmp(cmd, "pack") == 0) {
        PackOptions opts;
        const char *input_dir = NULL;
        const char *archive_path = NULL;
        int i;
        int ok;
        memset(&opts, 0, sizeof(opts));
        opts.compression_level = 6;
        opts.offset_padding = 4;
        opts.size_padding = 0;
        for (i = 2; i < argc; ++i) {
            if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--overwrite") == 0) {
                opts.overwrite = 1;
            } else if (strcmp(argv[i], "--legacy") == 0) {
                opts.legacy = 1;
            } else if (strcmp(argv[i], "--allow-empty") == 0) {
                opts.allow_empty = 1;
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--compression") == 0) {
                if (!require_value(argc, argv, &i, argv[i], err, sizeof(err))) {
                    fprintf(stderr, "error: %s\n", err);
                    string_list_free(&opts.excludes);
                    string_list_free(&opts.raw_patterns);
                    return 1;
                }
                opts.compression_level = atoi(argv[i]);
            } else if (strcmp(argv[i], "--offset-padding") == 0) {
                if (!require_value(argc, argv, &i, "--offset-padding", err, sizeof(err))) {
                    fprintf(stderr, "error: %s\n", err);
                    string_list_free(&opts.excludes);
                    string_list_free(&opts.raw_patterns);
                    return 1;
                }
                opts.offset_padding = (uint32_t)strtoul(argv[i], NULL, 10);
            } else if (strcmp(argv[i], "--size-padding") == 0) {
                if (!require_value(argc, argv, &i, "--size-padding", err, sizeof(err))) {
                    fprintf(stderr, "error: %s\n", err);
                    string_list_free(&opts.excludes);
                    string_list_free(&opts.raw_patterns);
                    return 1;
                }
                opts.size_padding = (uint32_t)strtoul(argv[i], NULL, 10);
            } else if (strcmp(argv[i], "--exclude") == 0) {
                if (!require_value(argc, argv, &i, "--exclude", err, sizeof(err)) || !string_list_append(&opts.excludes, argv[i])) {
                    fprintf(stderr, "error: %s\n", err[0] ? err : "out of memory");
                    string_list_free(&opts.excludes);
                    string_list_free(&opts.raw_patterns);
                    return 1;
                }
            } else if (strcmp(argv[i], "--raw") == 0) {
                if (!require_value(argc, argv, &i, "--raw", err, sizeof(err)) || !string_list_append(&opts.raw_patterns, argv[i])) {
                    fprintf(stderr, "error: %s\n", err[0] ? err : "out of memory");
                    string_list_free(&opts.excludes);
                    string_list_free(&opts.raw_patterns);
                    return 1;
                }
            } else if (!input_dir) {
                input_dir = argv[i];
            } else if (!archive_path) {
                archive_path = argv[i];
            } else {
                usage();
                string_list_free(&opts.excludes);
                string_list_free(&opts.raw_patterns);
                return 1;
            }
        }
        if (!input_dir || !archive_path) {
            usage();
            string_list_free(&opts.excludes);
            string_list_free(&opts.raw_patterns);
            return 1;
        }
        ok = pack_archive_cmd(input_dir, archive_path, &opts, err, sizeof(err));
        string_list_free(&opts.excludes);
        string_list_free(&opts.raw_patterns);
        if (!ok) {
            fprintf(stderr, "error: %s\n", err);
            return 1;
        }
        return 0;
    }

    usage();
    return 1;
}
