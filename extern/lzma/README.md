# LZMA SDK

Unmodified C encoder/decoder subset of Igor Pavlov's public-domain LZMA SDK
26.03 (2026-09-03), from https://www.7-zip.org/a/lzma2603.7z.
See LICENSE.txt for the upstream documentation and license.

Source archive SHA-256:
`86c213f752520ab5325c310f50bef63ec344b56dd1c80b0246d06dc6cec953b2`.

Built in single-thread mode (`Z7_ST`); static linking only pulls the decoder
into the runtime. The encoder is used by `legacy_archive`.
