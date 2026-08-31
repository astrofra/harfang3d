# Tau Compatibility Layer

Imported: 2026-08-31

This folder is where Harfang-owned compatibility code should live while Tau is
adapted incrementally.

Design rule:

- duplicate only the low-level legacy support that Tau strictly requires;
- do not import NeoGS scenegraph, renderer, editor, or serialization systems;
- prefer tiny adapter stubs over a broad legacy runtime transplant.

Expected contents over time:

- minimal legacy include path shims such as `framework/...` and `physic/...`;
- tiny transform bridge types for the Tau-to-Harfang integration seam;
- Harfang-native replacements for the narrow `nItem` / `nMItem` surface that
  Tau actually touches;
- build-only redirections that let selected Tau files compile without dragging
  the old engine architecture into Harfang.
