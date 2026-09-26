# Legacy archive tool

`legacy_archive pack <input_dir> <archive.nac>` now uses LZMA by default
(level 6). `info`, `list` and `unpack` recognize raw, Zlib and LZMA entries.

```text
legacy_archive pack -c 9 data data.nac
legacy_archive pack --method zlib data compatible.nac
legacy_archive pack -c -1 data uncompressed.nac
legacy_archive unpack data.nac extracted
```

`--method lzma|zlib` selects the codec. `-c` / `--compression` still selects
the level (0..9, or -1 for raw). `--raw PATTERN` overrides compression for
matching paths; `--allow-empty` includes empty files. LZMA automatically
stores entries raw when compression would not save space.

LZMA archives require an updated HARFANG runtime **and launcher**. Older
readers only support raw and Zlib: use `--method zlib` to retain their
compatibility. `--legacy` only selects the old header layout; it does not
select a codec. Zlib packing and existing archives retain their format.

The NAC entry method byte is 0 (raw), 1 (Zlib) or 2 (LZMA). Both compressed
methods store the original size followed by the stored size. An LZMA
payload is five SDK properties bytes followed by a raw LZMA1 stream with
an end marker; the stored size includes the properties. No `.xz` or
`.lzma` container header is embedded. Compression remains per file, so
assets can be read independently. Readers require the exact decoded size,
the end marker and full input consumption. Unknown methods are rejected.

Run the CLI regression tests with Python 3.9+ (standard library only):

```text
python tools/legacy_archive/test_archive.py path/to/legacy_archive.exe
```
