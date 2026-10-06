// Check grid topology and normals with the real native vector binding, without a renderer.

import * as hg from 'harfang';
import {
    compute_grid_vertex_position,
    compute_triangles_for_grid_vertex,
    compute_vertex_normals,
} from '../../tutorials/model_builder.js';

function assert(value, message) {
    if (!value) throw Error(message);
}

const origin = new hg.Vec3(2, 0, 3);
const flat = compute_grid_vertex_position(origin, 0.25, 40, 40, true, 0);
const triangles = compute_triangles_for_grid_vertex(40, 40);
assert(flat.length === 1681, '40x40 quads need 41x41 vertices');
assert(triangles.length === 3200, 'Two triangles per quad');
assert(flat[0].x === -3 && flat[0].z === -2, 'Centered grid minimum');
assert(flat[1680].x === 7 && flat[1680].z === 8, 'Centered grid maximum');
const used = new Set();
let area = 0;
for (const triangle of triangles) {
    for (const index of triangle) {
        assert(Number.isInteger(index) && index >= 0 && index < flat.length, 'Vertex index in range');
        used.add(index);
    }
    const [a, b, c] = triangle.map(index => flat[index]);
    const normal = hg.Cross(a.sub(b), c.sub(b));
    assert(normal.y > 0, 'Upward face winding');
    area += hg.Len(normal) / 2;
}
assert(used.size === flat.length, 'Every grid vertex is used');
assert(Math.abs(area - 100) < 1e-5, 'Grid covers a 10x10 surface');
for (const normal of compute_vertex_normals(triangles, flat)) {
    assert(normal.x === 0 && normal.y === 1 && normal.z === 0, 'Flat grid normals point up');
}

const wave = compute_grid_vertex_position(origin, 0.25, 40, 40, true, Math.PI / 2);
assert(wave.some(point => Math.abs(point.y) > 0.5), 'Animation deforms the grid');
for (const normal of compute_vertex_normals(triangles, wave)) {
    assert(Number.isFinite(normal.x + normal.y + normal.z), 'Finite wave normals');
    assert(Math.abs(hg.Len(normal) - 1) < 1e-5, 'Normalized wave normals');
    assert(normal.y > 0, 'Wave normals preserve face orientation');
}
const uncentered = compute_grid_vertex_position(origin, 1, 1, 1, false, 0);
assert(uncentered[0].x === 2 && uncentered[0].z === 3, 'Uncentered origin');
const fallback = compute_vertex_normals([[0, 0, 0]], [new hg.Vec3()])[0];
assert(fallback.x === 0 && fallback.y === 1 && fallback.z === 0, 'Degenerate normal fallback');
console.log('MODEL_BUILDER_MATH_OK');
