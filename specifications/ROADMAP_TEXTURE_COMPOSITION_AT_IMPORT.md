# Roadmap: move texture composition from assetc to import

Date: 2026-11-25.
Status: agreed direction, deferred implementation.

## Decision

Material-specific texture composition belongs to the importer or an explicit asset authoring tool. Import should produce ordinary, inspectable image assets representing the translated HARFANG material. `assetc` should compile those images for the runtime.

The glTF importer should stop delegating occlusion/roughness/metallic (ORM) assembly to `profiles.<profile>.preprocess.construct` in texture `.meta` files. Existing recipes need a migration path before support is removed from `assetc`.

This roadmap records future work. It does not implement or schedule the migration.

## Current implementation

The [glTF importer](../tools/gltf_converter/gltf_importer.cpp), in `ExportMaterial`, writes `construct` recipes to:

- Combine a separate occlusion image with a metallic/roughness image when their UV sets match.
- Supply a constant occlusion channel when no occlusion image is present.
- Combine an occlusion image with constant roughness and metallic values.

[`assetc`](../tools/assetc/assetc.cpp), in `PreprocessTexture`, interprets the recipe, loads its dependencies, assembles RGB/RGBA channels, writes an intermediate `.tmp` image into the input tree, and passes that result to texture compilation. Recipe inputs can be integer constants, channels of the current image, another image's corresponding channel, or explicit `{ "path": ..., "channel": ... }` selections.

The material's actual texture content therefore depends on an additional compilation-time recipe. A source image viewed independently may not represent what the material uses after compilation. Material translation also spans two tools, although the importer already knows the material context.

The [importer audit](AUDIT_GLTF_IMPORTER.md) additionally found incorrect constant generation in the occlusion-only branch: casting a factor to `int` before multiplication by 255 truncates intermediate values to zero. Moving composition must address that defect, not reproduce its arithmetic in a new location.

## Target responsibilities

| Component | Responsibility |
| --- | --- |
| Importer | Decode source material semantics, choose a supported mapping, generate any required images, and bind those images to native materials. |
| Imported image assets | Contain the actual composed pixels; remain inspectable, editable and suitable for version control. |
| Texture `.meta` | Describe compilation settings such as compression, color space, mipmaps and target sizing. |
| `assetc` | Compile explicit image inputs into platform/runtime resources, tracking those inputs and compilation settings. |
| Runtime | Load compiled resources through the existing asset pipeline. |

Compression, mipmap generation and format conversion remain compiler responsibilities. ORM packing and other material-specific channel synthesis move upstream. Import-time images are source assets for compilation, not a replacement for the compiled runtime resource tree.

## Phase 1 — establish the composition contract

- [ ] Inventory `construct` producers and consumers, representative project `.meta` files, and profile-specific recipes. Determine whether tools besides the glTF importer depend on it.
- [ ] Define output naming and ownership for generated textures, including their directory relative to the imported scene and project resource root.
- [ ] Define resolution, resampling, channel selection and integer rounding rules. Treat ORM channels as linear data; avoid color-space conversions intended for color textures.
- [ ] Define material-factor handling with the target shader. A factor must be applied exactly once, whether retained as a supported uniform or baked into pixels.
- [ ] Define behavior when source UV sets, texture transforms or sampler requirements differ. Channel packing alone cannot reconcile different sampling coordinates; unsupported combinations need an explicit diagnostic or a separately designed conversion.

**Exit criterion:** a documented mapping for supported ORM inputs, including defaults, factors and unsupported combinations, before implementing image output.

## Phase 2 — generate explicit textures during import

- [ ] Add image composition to the import path, using reusable image utilities where appropriate, without creating another compiler-side recipe interpreter.
- [ ] Generate a dedicated image when composition is required. Never overwrite a shared source texture to satisfy one material's requirements.
- [ ] Bind the generated image to the native material and emit only its compilation settings in `.meta`.
- [ ] Reuse an existing image when it already supplies the required representation without pixel changes. Share generated images only when the complete composition and sampling requirements are equivalent.
- [ ] Preserve import output policies and make reimport deterministic: stable resource names and identical pixels for identical inputs/settings.
- [ ] Distinguish importer-owned generated files from user edits before regeneration. Record provenance if needed for reimport, outside executable compilation recipes.
- [ ] On missing input or composition failure, report the material and input involved; do not publish a new material binding to a missing or incomplete image.
- [ ] Stop emitting new `construct` directives from the glTF importer.

**Exit criterion:** a fresh import followed by `assetc` compilation works with no `construct` metadata and no material-specific image synthesis in the compiler.

## Phase 3 — migrate existing recipes

- [ ] Keep the legacy `assetc` reader temporarily so existing projects continue to compile during the transition.
- [ ] Provide an explicit migration tool with a read-only inventory/preview mode. List affected images, recipes, profiles, dependencies, generated outputs and reference changes.
- [ ] Materialize supported recipes into ordinary images, update the affected material references, retain compilation settings, and remove only the migrated `construct` directives.
- [ ] Cover all existing recipe forms, including RGB/RGBA, constants, channel swizzles, implicit external-image channels and explicit channel selectors.
- [ ] Handle shared images without changing unrelated materials. Stage generated files and reference updates so a failed migration does not leave partial replacements.
- [ ] Preserve original source images and retain enough migration information to review or reverse the changes.
- [ ] Resolve profiles that contain different composition recipes explicitly. Do not flatten distinct profile outputs into one image without checking how references will select them.
- [ ] Report unsupported or ambiguous recipes rather than removing them. Incorrect historical constants cannot be recovered from the recipe alone; recovering the original material factors may require reimporting the source asset.

**Exit criterion:** representative legacy projects can be migrated and compiled without recipes, with reviewed image/reference changes and no silent loss of profile-specific behavior.

## Phase 4 — retire compiler-side composition

- [ ] Confirm the importer no longer emits recipes and all maintained projects/tools have a migration path.
- [ ] Document deprecation and the migration command; choose a removal release after checking actual usage.
- [ ] Remove material-composition code from `assetc`, including its authoring-tree `.tmp` generation for these recipes.
- [ ] Detect remaining `construct` metadata and fail with a migration diagnostic rather than silently compiling the uncomposed source image.

**Exit criterion:** texture compilation consumes explicit images and compilation settings only; legacy recipes cannot silently change material appearance.

## Validation and acceptance

- [ ] Pixel-level fixtures cover separate AO/MR images, missing AO, AO-only plus constants, already packed ORM, RGB/RGBA inputs and explicit channel selections. Include fractional factors such as 0.5 and 0.7.
- [ ] Resolution tests cover mismatches in width only, height only, and both dimensions, with a documented output-size/resampling policy.
- [ ] Shared-resource tests cover two materials using one image with different factors or composition requirements.
- [ ] Reimport tests cover deterministic naming/content, changed source images, user-edited generated images, and skip/overwrite/rename policies.
- [ ] For known-good legacy recipes, compare migrated pixels with the legacy assembled pixels before compression. Report known legacy defects separately from intended migration differences.
- [ ] Compile generated images through `assetc`; verify resource paths, dependencies, incremental rebuilds and preservation of the authored input tree.
- [ ] Compare rendered materials as well as files. Successful copying and compilation alone do not establish material equivalence.
- [ ] Exercise missing inputs, unsupported UV combinations, conflicting profiles and migration interruption without damaging source assets.

## Scope boundaries

The first implementation should cover glTF ORM composition and its legacy migration. Broader PBR issues from the audit—alpha masking, sampler/UV support, extensions and shader factor behavior—remain related work; coordinate their decisions where composition depends on them.

This roadmap does not revive the cancelled project-specific animated-material conversion. It also does not remove `construct` immediately or introduce a general procedural asset system into `assetc`.
