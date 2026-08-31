# Tau Legacy Import For Harfang

Imported: 2026-08-31

This folder is the starting point for a Tau-based scene-physics alternative in
Harfang.

The intent is not to port an old engine wholesale.

The chosen compromise is:

- keep the original Tau physics source visible under `source/physic/`,
- adapt it progressively behind Harfang-owned compatibility shims under
  `compat/`,
- and avoid importing legacy scenegraph, renderer, or editor systems.

Current status:

- the legacy Tau `physic/` source snapshot has been imported from
  `S:\works\engine-neogs\vendor\tau\source\physic`;
- `tau_legacy` is currently a scaffold target used to host the imported source
  tree inside the Harfang build;
- a Harfang-owned transform adapter now exists under `compat/` and is wired to
  `SceneTauPhysics` as the future `nItem` / `nMItem` seam;
- imported Tau source files are present for incremental adaptation but are not
  compiled yet.

The next steps are:

1. add the minimum compatibility headers and utility types required to compile a
   cuboid-only subset;
2. patch the known safety issues identified in the Tau feasibility study;
3. progressively switch selected Tau files from `HEADER_FILE_ONLY` to compiled
   sources;
4. wire the adapted subset into `SceneTauPhysics`.

Reference documents:

- `specifications/SPECS_TAU_PHYSICS_BACKEND_FEASIBILITY.md`
- `specifications/SPECS_TAU_CUBOID_BACKEND_INTEGRATION_PLAN.md`
- `specifications/SPECS_PHYSICS_QA_AUTOMATION_FEASIBILITY.md`
