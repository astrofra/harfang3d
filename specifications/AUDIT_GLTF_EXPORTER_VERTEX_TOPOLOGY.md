# HARFANG to glTF exporter: vertex sharing and topology audit

Date: 2026-11-25  
Audited revision: `4349498659e043fc517bc626b10a06a2b01a6edb`  
Primary file: `tools/gltf_converter/gltf_exporter.cpp`  
Scope: native geometry loading, triangulation, vertex attributes, index generation, and related geometry correctness risks. This is not a comprehensive audit of materials, animation, cameras, or scene transforms. No exporter implementation was changed.

## Verdict

**The suspected defect is confirmed.** `ExportGeometry` appends a new vertex for every triangle corner. It emits index buffers, but never reuses a vertex index between triangles. The result is effectively a triangle soup with redundant indices, including duplicate vertices inside a triangulated quad.

The duplication is **not limited to four copies**. For valid polygons whose material slots are exported, the generated vertex count is exactly `3 × triangle_count`. The ratio to native vertex count depends on connectivity, polygon sizes, and attribute discontinuities.

**The native format already contains the connectivity needed to improve this.** The exporter loads it and then discards vertex identity during expansion. The preferred fix is to preserve native vertex identity while splitting only where exported attributes differ. A geometric proximity weld should not be the default repair.

This does not imply that a glTF mesh should always contain as few vertices as `geo.vtx`: UV seams and hard normals legitimately require additional render vertices. Nor does vertex deduplication alone recover the original polygon topology in a standard glTF file.

## Evidence in the implementation

References below are repository-relative; line numbers refer to the audited revision.

| Location | Observation | Implication |
| --- | --- | --- |
| [gltf_exporter.cpp](../tools/gltf_converter/gltf_exporter.cpp), lines 876–899 | `ExportObject` loads `source_output_path / model_name` using `LoadGeometryFromFile`. | This path reads editable native geometry, rather than reconstructing topology from compiled GPU buffers. |
| Same file, lines 521–541 | `PreparePolygonVertex` reads position through `geo.binding`, and normals/UVs through the polygon-corner offset. | Native vertex identity and corner attributes are both available at export time. |
| Same file, lines 552–570 | Every generated triangle performs three `geo_vtx.push_back(...)` calls; each index is the current vector size. | No lookup, remapping, or reuse occurs, even for identical attributes. |
| Same file, lines 589–611, 638–660, 741–757, 784–800 | Position, normal, and available UV0/UV1 arrays are serialized from the expanded vector. | Duplication affects every emitted attribute stream. |
| Same file, lines 813–869 | Material primitives reference shared attribute accessors and receive separate `uint32_t` index arrays. | Material primitives do not each serialize another full copy of the attribute arrays. The duplication originates earlier. |
| [geometry.cpp](../harfang/engine/geometry.cpp), lines 619–630 | The native runtime conversion calls `ModelBuilder::AddVertex` before emitting triangle indices. | The native conversion has a reuse step that the glTF exporter bypasses. |
| [model_builder.cpp](../harfang/engine/model_builder.cpp), lines 53–89 | `AddVertex` uses a hash lookup and vertex equality to reuse existing vertices within a list. | There is an existing implementation pattern, though it is not a drop-in topology-preserving export policy. |

The decisive exporter sequence is:

```cpp
geo_mat_indices[i_mat].push_back(geo_vtx.size());
geo_vtx.push_back(PreparePolygonVertex(geo, i_bind, 0));
geo_mat_indices[i_mat].push_back(geo_vtx.size());
geo_vtx.push_back(PreparePolygonVertex(geo, i_bind, i_vtp + 1));
geo_mat_indices[i_mat].push_back(geo_vtx.size());
geo_vtx.push_back(PreparePolygonVertex(geo, i_bind, i_vtp));
```

For a single quad with bindings `[0, 1, 2, 3]`, the triangle corner identities are `[0, 2, 1, 0, 3, 2]`. The current output stores six vertices and indices `[0, 1, 2, 3, 4, 5]`. Four stored vertices with indices `[0, 2, 1, 0, 3, 2]` would suffice, preserving exactly the same attributes and triangle winding.

## What the native format preserves

[Geometry](../harfang/engine/geometry.h), lines 24–55, separates:

- `vtx`: native positions and their vertex identities.
- `pol`: polygon corner counts and material slots.
- `binding`: a flattened polygon-corner-to-native-vertex mapping.
- `normal`, `color`, `tangent`, and eight `uv` sets: polygon-corner attributes.
- `skin` and `bind_pose`: deformation data, currently omitted by this exporter.

For polygon `p`, let `base` be the sum of the preceding polygon corner counts. Its corner `k` has native vertex ID `binding[base + k]`, position `vtx[binding[base + k]]`, and, when present, normal `normal[base + k]` and UV `uv[channel][base + k]`.

`LoadGeometry` reads these arrays directly, and `SaveGeometry` writes them directly ([geometry.cpp](../harfang/engine/geometry.cpp), lines 371–473). Consequently, neither reverse engineering a spatial weld nor changing the native file format is required to reuse vertices in the exporter.

The source can itself contain duplicate positions under different IDs. Those may represent disconnected surfaces or existing import splits. Native IDs describe the topology actually stored; they do not guarantee the recovery of topology from an earlier authoring application.

There is also a documentation inconsistency relevant to future skin support: `geometry.h` labels `skin` as per-polygon-vertex, but the runtime conversion indexes it with `i_vtx` (lines 500–506), and the glTF importer populates it using vertex offsets (lines 1057–1082). Resolve this discrepancy before implementing skin export; do not copy the header comment into a new indexing algorithm.

## Necessary splits versus avoidable duplication

A glTF primitive uses one index stream to select corresponding elements of all vertex attribute arrays. Distinct normals or UVs at a shared position therefore require distinct render vertices. This is a constraint of the representation, not evidence of an unwelded source mesh. See the [Khronos glTF 2.0 mesh specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes).

Three operations must be distinguished:

| Operation | Safe default? | Result |
| --- | --- | --- |
| Reuse vertices with the same native ID and identical exported attributes | Yes | Removes avoidable expansion while retaining source identity and necessary seams. |
| Merge identical complete exported attribute tuples regardless of native ID | Optional render optimization | May merge deliberately disconnected but coincident source vertices. |
| Merge by position alone, especially with a distance tolerance | No | Can destroy hard normals, UV seams, thin gaps, disconnected shells, or deformation boundaries. |

Material boundaries require separate primitives, but not necessarily separate vertex records: the current shared-accessor arrangement can reuse the same records across primitives when their attributes agree.

Even after the fix, a hard-edged cube can correctly have eight native positions and 24 render vertices. Standard triangle primitives do not preserve original quad boundaries or explicitly record which seam-split vertices originated from the same native vertex. If exact editable topology must survive the round trip, retain the `.geo` or define documented metadata/sidecar mappings for native IDs, polygon boundaries, corner identities, and any omitted attributes. Generic glTF consumers would need explicit support to reconstruct that information.

## Quantifying the expansion

For valid exported polygons with corner counts `n_p`:

```text
T = sum(n_p - 2)
current generated vertices = 3T
avoidable duplicates = 3T - U
```

Here `U` is the number of distinct `(native vertex ID, exported attribute tuple)` keys over referenced corners. Unused native positions should not be included in the optimized count.

For a large smooth closed triangulated genus-zero mesh without seams, `T = 2V - 4`, so the current ratio approaches six generated vertices per native vertex. This is an illustrative topology calculation, not a universal bound: individual high-valence vertices can occur many more times.

### Measurements from local native assets

The audit parsed three existing `.geo` files from `interactive-book/app/assets/common/primitives/`, relative to the workspace root. It replayed the exporter's material traversal and triangle fan, then counted exact attribute tuples. All three assets contain normals. The key included native vertex ID, position, normal, and present UV0/UV1 values, matching the active exporter streams.

**These are native-file measurements and algorithm-replay results, not measurements of files produced by running the complete exporter.** They assume all used material slots are present on the exporting object. No fresh C++ build, full scene export, glTF validator run, or visual comparison was performed.

| Asset | Native positions | Polygon corners | Polygons | Triangles | Current generated vertices | Required records under proposed key | Current / required |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `plane.geo` | 25 | 96 | 32 | 32 | 96 | 25 | 3.84× |
| `cube.geo` | 24 | 96 | 26 | 44 | 132 | 96 | 1.375× |
| `sphere.geo` | 642 | 3,840 | 1,280 | 1,280 | 3,840 | 673 | 5.71× |

The local `cube.geo` is not the idealized six-quad cube discussed above. Its attributes require 96 distinct records under exact comparison; reducing it to 24 solely because there are 24 native positions would be incorrect. The sphere similarly requires more records than native positions. On these three assets, dropping native ID from the comparison key did not reduce the distinct-record counts further.

With position + normal + UV0 stored as floats, the attribute payload is 32 bytes per vertex. As a size illustration, 3,840 expanded records occupy 122,880 bytes versus 21,536 bytes for 673 records, a reduction of 101,344 bytes (82.5%) in that attribute payload. This excludes indices, additional streams, JSON/base64 overhead, textures, and compression; it is not a whole-file size claim. Triangle index count remains `3T`.

Redundant records increase attribute payload and CPU preparation work and eliminate index-based vertex reuse between triangles. Actual GPU and export-time improvements require benchmarks; they should not be inferred directly from the vertex-count ratio.

## Recommended implementation

Implement an export-specific remap inside `ExportGeometry`, before serializing accessors.

1. Validate native binding ranges and attribute lengths, polygon corner counts, and the relationship between used material slots and object materials. Reject unsupported or malformed input with an actionable diagnostic.
2. Decide the set of attributes actually being exported. Resolve missing normals explicitly before constructing keys.
3. Traverse polygons and produce the existing triangle corner order `(0, k + 1, k)`. Keep the current Z reflection and winding reversal together.
4. For each corner, build its final exported attribute values and an explicit key containing its native vertex ID and all emitted attributes.
5. Look up that key in a geometry-wide map. Append attributes only for a new key; otherwise reuse the stored index. Append that index to the appropriate material list.
6. Serialize compact shared attribute accessors, then nonempty material primitives. Preserve an explicit mapping from material slot to primitive; vector position is no longer sufficient if empty slots are skipped.
7. Select an appropriate index component width after remapping. Initially retaining 32-bit indices keeps the correction focused; smaller indices can follow with boundary tests.

Conceptual algorithm:

```text
for each polygon:
    for each triangle corner in the existing fan order:
        corner = polygon_offset + local_corner
        source_id = geo.binding[corner]
        values = convert_exported_attributes(source_id, corner)
        key = (source_id, values)
        index = find_or_insert(key, compact_vertex_arrays)
        material_indices[polygon.material].append(index)
```

Use deterministic first-occurrence numbering and explicit field comparisons. Do not hash raw `hg::Vertex` memory: it includes attributes that are not currently exported, potentially uninitialized fields, and possible padding. Hashing and equality must agree, including the treatment of signed zero. Reject non-finite values; do not introduce approximate merging implicitly.

The runtime `ModelBuilder` is useful evidence that vertex reuse is already intended elsewhere, but directly reusing it has tradeoffs: it compares all its fields, works within material lists, and does not include native vertex identity in equality. An export-specific key is clearer and can match exactly the serialized data. If colors, tangents, skinning, or morph attributes are added later, the key must expand with them.

A geometry-wide remap takes expected linear time in the generated corner count and memory proportional to distinct records plus indices. A single polygon pass also avoids the current repeated full polygon traversal for each material.

## Related findings to address alongside the repair

These findings are from source inspection; their failure cases were not executed during this audit.

| Priority | Finding and evidence | Recommended handling |
| --- | --- | --- |
| High | **Missing normals can export indeterminate values.** `PreparePolygonVertex` declares `hg::Vertex vtx;` and only assigns its normal when `geo.normal` is populated (lines 522–527), while normal serialization is unconditional (623–670). `Vec3` has a defaulted constructor and uninitialized scalar members in [vector3.h](../harfang/foundation/vector3.h), lines 25 and 119. | Initialize staging data and explicitly generate suitable normals or omit the stream. Zero initialization alone does not produce valid surface normals. |
| High | **Material-count mismatch can lose faces or access beyond cached primitive storage.** Geometry expansion uses native `mat_count`, but primitive generation uses `nb_materials` (547, 552, 813). The cache is keyed only by geometry path, although the cached primitive count depends on the first object's material count; a later object assigns `mesh.primitives[i]` without a size check (887–911). | Validate used slots against each object, make geometry caching independent of object material count, and bind materials through explicit slot mappings. |
| High | **Empty material slots can reach unsafe serialization.** The exporter emits a primitive for every object material, including unused slots. `serialize` forms `&from[0]` and `&to[offset]` even for empty vectors (169–174); empty index accessors also retain sentinel bounds (843–858). A geometry with positions but no exportable triangles can reach the same problem. | Skip empty primitives, handle empty geometry deliberately, and make serialization safe for zero-length input. |
| Medium | **The exporter does not validate loaded geometry before indexing arrays.** The loader returns the arrays, and `ExportObject` checks only whether positions exist (889–893). | Invoke native validation and supplement it with export-specific checks, including polygon sizes, finite attributes, and material consistency. |
| Medium | **Attribute preservation is incomplete.** The active output includes position, normal, UV0, and UV1. Tangent serialization is commented out (672–724); colors and UV2–UV7 are read into staging vertices but never attached to primitives; skin data is not read by this `PreparePolygonVertex`. | Document these limitations and implement explicit attribute support separately. Deduplication cannot recover data that was never serialized. |
| Medium | **Fan triangulation assumes suitable polygons.** The loop at 558–564 does not handle general concave polygons robustly. | Establish whether supported source assets are guaranteed convex/pretriangulated; otherwise add robust triangulation with corner-attribute tracking. |

The disabled tangent block should not simply be uncommented: it iterates native tangent entries instead of the exported vertex order, keeps the original Z component, and hardcodes handedness. Any implementation must emit remapped tangent frames with correct coordinate conversion and handedness.

## Acceptance criteria for a future fix

| Fixture / check | Required behavior |
| --- | --- |
| Single quad, continuous attributes | Four records, six indices, unchanged winding and appearance. |
| Two triangles sharing an edge, matching attributes | Four records, shared indices along the edge. |
| Smooth tetrahedron, matching corner attributes | Four records rather than twelve. |
| Six-quad hard-edged cube with eight native positions | 24 records and 36 indices when each face has continuous attributes; preserve hard edges. |
| UV seam or normal discontinuity | Split the affected native vertex only where the emitted tuple differs. |
| Coincident disconnected source vertices | Keep different native IDs distinct under the default policy. |
| Multiple materials, unused slots, and shared geometry with different object material counts | Preserve faces and material assignments; no empty primitives or out-of-bounds indexing. |
| Missing normals, empty geometry, invalid bindings or attribute lengths | Defined handling and clear diagnostics, without indeterminate data or invalid accessors. |
| Local plane / cube / sphere above | Match the measured 25 / 96 / 673 record counts when using the same exact attributes and identity policy. |
| All output indices and streams | Indices in range, consistent accessor counts, finite values, correct bounds, identical expanded per-corner attributes before and after deduplication. |
| Serialization and rendering | Validate both `.gltf` and `.glb` with the Khronos validator and compare rendered output, including seams, hard edges, reflection/winding, and material boundaries. |

Compare the *expanded triangle attribute sequence* of the old and new representations, not merely the counts. This detects accidental changes to UVs, normals, corner order, and material assignments while allowing the index representation to improve.

## Reproduction appendix

Run the following read-only Python script from the workspace root containing both `harfang3d/` and `interactive-book/`. It parses the geometry arrays up through the UV sets using the binary layout in `LoadGeometry`, and replays the current triangle expansion. It is an audit probe for these valid local files, not a general-purpose untrusted-file parser or a full exporter test.

```python
from pathlib import Path
import hashlib
import struct

for name in ("plane", "cube", "sphere"):
    path = Path("interactive-book/app/assets/common/primitives") / f"{name}.geo"
    data = path.read_bytes()
    offset = 9  # uint32 magic, uint8 marker, uint32 version

    def read_vector(fmt):
        global offset
        count = struct.unpack_from("<I", data, offset)[0]
        offset += 4
        size = struct.calcsize("<" + fmt)
        values = [struct.unpack_from("<" + fmt, data, offset + i * size)
                  for i in range(count)]
        offset += count * size
        return values

    positions = read_vector("3f")
    polygons = read_vector("2B")
    bindings = [item[0] for item in read_vector("I")]
    normals = read_vector("3f")
    colors = read_vector("4f")
    tangents = read_vector("6f")
    uv = [read_vector("2f") for _ in range(8)]
    assert sum(n for n, _ in polygons) == len(bindings)
    assert len(normals) == len(bindings)
    keys = []
    for material in sorted({m for _, m in polygons}):
        base = 0
        for count, slot in polygons:
            if slot == material:
                for k in range(1, count - 1):
                    for corner in (base, base + k + 1, base + k):
                        vertex = bindings[corner]
                        keys.append((vertex, positions[vertex], normals[corner],
                                     *(u[corner] for u in uv[:2] if u)))
            base += count
    print(name, hashlib.sha256(data).hexdigest())
    print("native", len(positions), "corners", len(bindings),
          "polygons", len(polygons), "triangles", len(keys) // 3,
          "expanded", len(keys), "identity_and_attributes", len(set(keys)),
          "attributes_only", len({key[1:] for key in keys}))
```

Input SHA-256 fingerprints:

```text
plane.geo   203566904f893923fe0a7835b6e3715523f2b05e8fe1ed79ae054c6a90f6c9d5
cube.geo    4d26acfd50f72b6b8426272c7604f276b454baf0ece4e3f4e0109280ba6f5b6f
sphere.geo  ed22543e674ae86c23eab4014f95e24a1a0132f93e2c97649017f73ebabf5a11
```
