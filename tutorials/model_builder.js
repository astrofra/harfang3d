// Model builder usage

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function create_grid_model_with_model_builder(vtx_layout, origin_pos, quad_size, range_x, range_z, center_on_origin, time) {
    /*
    vtx_layout : VertexLayout from harfang. Need the position and the normal.
    origin_pos : position where the model will be created
    quad_size : size of a quad (the grid is composed of multiple quads with the same size)
    range_x : number of quads on the x axis
    range_z : number of quads on the z axis
    center_on_origin : True if the model need to be centered on the origin_pos
    time : current clock in seconds to add movement in the mesh (waves movement)
    */

    // Get the grid vertex position
    const vertex_positions = compute_grid_vertex_position(origin_pos, quad_size, range_x, range_z, center_on_origin, time);

    // Get the grid triangles
    const grid_triangles = compute_triangles_for_grid_vertex(range_x, range_z);

    // Get the grid vertex normals
    const vertex_normals = compute_vertex_normals(grid_triangles, vertex_positions);

    // Create a model builder
    const mdl_builder = new hg.ModelBuilder();

    // Add all vertex to the model builder
    const vertex_indices = [];
    for (let i = 0; i < vertex_positions.length; i++) {
        const v = new hg.Vertex();
        v.pos = vertex_positions[i];
        v.normal = vertex_normals[i];
        const v_id = mdl_builder.AddVertex(v);
        vertex_indices.push(v_id);
    }

    // Add the triangles to the model builder
    for (let i = 0; i < grid_triangles.length; i++) {
        const [i0, i1, i2] = grid_triangles[i];
        mdl_builder.AddTriangle(vertex_indices[i0], vertex_indices[i1], vertex_indices[i2]);
    }

    mdl_builder.EndList(0);

    // Create the model from the model builder
    const mdl = mdl_builder.MakeModel(vtx_layout);

    return mdl;
}


export function compute_grid_vertex_position(origin_pos, quad_size, range_x, range_z, center_on_origin, time) {
    // Create offset if the model need to be center on the origin_pos
    let offset = new hg.Vec3(0, 0, 0);
    if (center_on_origin) {
        offset = new hg.Vec3(range_x * quad_size / 2, 0, range_z * quad_size / 2);
    }

    // Create the positions list
    const positions = [];

    // For all the vertex of the grid (add 1 to the range to get the last vertex on each axes, because the range corresponding to the number of quad to draw)
    for (let iz = 0; iz <= range_z; iz++) {
        for (let ix = 0; ix <= range_x; ix++) {
            // Create it's position
            const pos_x = origin_pos.x + ix * quad_size - offset.x;
            const pos_z = origin_pos.z + iz * quad_size - offset.z;
            const pos_y = Math.sin(pos_x) * Math.sin(pos_z) * Math.sin(time); // use time to add some movement
            positions.push(new hg.Vec3(pos_x, pos_y, pos_z));
        }
    }

    return positions;
}


export function compute_triangles_for_grid_vertex(range_x, range_z) {
    // Create the triangles list
    const triangles = [];

    // Use to get the vertex id in the positions list according to the x and z position in the grid
    function get_vertex_id(ix, iz) {
        return iz * (range_x + 1) + ix; // JavaScript arrays start at zero
    }

    // For all the quad of the grid
    for (let iz = 0; iz < range_z; iz++) {
        for (let ix = 0; ix < range_x; ix++) {
            // Get the 4 vertex that compose the quad
            const a = get_vertex_id(ix, iz); // bottom left
            const b = get_vertex_id(ix, iz + 1); // top left
            const c = get_vertex_id(ix + 1, iz + 1); // top right
            const d = get_vertex_id(ix + 1, iz); // bottom right

            triangles.push([d, c, b]);
            triangles.push([b, a, d]);
        }
    }

    return triangles;
}


export function compute_vertex_normals(triangles, positions) {
    // Create normal list with 1 vec3 per vertex position
    const normals = [];
    for (let i = 0; i < positions.length; i++) {
        normals[i] = new hg.Vec3(0, 0, 0);
    }

    // For all the triangle
    for (let it = 0; it < triangles.length; it++) {
        const [v0, v1, v2] = triangles[it];

        // Get the corresponding vertex position
        const p0 = positions[v0], p1 = positions[v1], p2 = positions[v2];
        const face_normal = hg.Cross(p0.sub(p1), p2.sub(p1));

        // Guard against degenerate triangles (zero area)
        if (!(face_normal.x === 0 && face_normal.y === 0 && face_normal.z === 0)) {
            // Accumulate (area-weighted) face normal to each vertex
            normals[v0] = normals[v0].add(face_normal);
            normals[v1] = normals[v1].add(face_normal);
            normals[v2] = normals[v2].add(face_normal);
        }
    }

    // Normalize all vertex normals
    for (let i = 0; i < normals.length; i++) {
        if (hg.Len(normals[i]) > 0) {
            normals[i] = hg.Normalize(normals[i]);
        } else {
            normals[i] = new hg.Vec3(0, 1, 0);
        }
    }

    return normals;
}


export function main(options) {
    // Init render and resources
    return runWindow('Harfang - Model builder', () => {
        const pipeline = hg.CreateForwardPipeline();
        const res = new hg.PipelineResources();

        // Create materials
        const prg_ref = hg.LoadPipelineProgramRefFromAssets('core/shader/pbr.hps', res, hg.GetForwardPipelineInfo());
        const plane_material = hg.CreateMaterial(prg_ref, 'uBaseOpacityColor', new hg.Vec4(0.5, 0.5, 0.5), 'uOcclusionRoughnessMetalnessColor', new hg.Vec4(1, 1, 0.25));

        // Setup scene
        const scene = new hg.Scene();
        if (!hg.LoadSceneFromAssets('probe_scene/pbr.scn', scene, res, hg.GetForwardPipelineInfo())) {
            throw Error('Failed to load probe_scene/pbr.scn');
        }

        const cam = hg.CreateCamera(scene, hg.TransformationMat4(new hg.Vec3(0, 6, -12), new hg.Vec3(hg.DegreeToRadian(30), 0, 0)), 0.01, 1000);
        scene.SetCurrentCamera(cam);

        // const light = hg.CreatePointLight(scene, hg.TranslationMat4(new hg.Vec3(0, 5, 0)), 0);
        const light_mtx = hg.TransformationMat4(new hg.Vec3(8, 5, 0), new hg.Vec3(hg.DegreeToRadian(50), hg.DegreeToRadian(-90), hg.DegreeToRadian(-90)));
        const inner_angle = hg.DegreeToRadian(30);
        const outer_angle = hg.DegreeToRadian(45);
        const light_color = new hg.Color(1, 1, 1, 1);
        const light = hg.CreateSpotLight(scene, light_mtx, 0, inner_angle, outer_angle, light_color, 1, light_color, 1, 1, hg.LST_Map, 0.0);

        // Create plane model
        const vtx_layout = hg.VertexLayoutPosFloatNormUInt8();
        // const vtx_layout = hg.VertexLayoutPosFloatColorFloat();

        const grid_start_pos = new hg.Vec3(0, 0, 0);
        const quad_size = 0.25;
        // Create the grid model from a model builder
        const grid_mdl = create_grid_model_with_model_builder(vtx_layout, grid_start_pos, quad_size, 40, 40, true, 1);
        // Add the grid model to the PipelineResources and get the corresponding model ref
        const grid_mdl_ref = res.AddModel('grid', grid_mdl);
        // Create a node from the model ref
        const grid_node = hg.CreateObject(scene, hg.TransformationMat4(grid_start_pos, new hg.Vec3(0, 0, 0)), grid_mdl_ref, [plane_material]);

        // main loop
        let current_time = 0;

        return {
            draw(dt, res_x, res_y) {
                const dts = hg.time_to_sec_f(dt);
                current_time += dts; // runWindow supplies dt without advancing the global TickClock

                // Rotate the grid node
                // const grid_rot = grid_node.GetTransform().GetRot();
                // const rot = grid_rot.add(new hg.Vec3(0, 0.5 * dts, 0));
                // grid_node.GetTransform().SetRot(rot);

                const new_grid_mdl = create_grid_model_with_model_builder(vtx_layout, grid_start_pos, quad_size, 40, 40, true, current_time);
                res.UpdateModel(grid_mdl_ref, new_grid_mdl);

                scene.Update(dt);
                hg.SubmitSceneToPipeline(0, scene, new hg.IntRect(0, 0, res_x, res_y), true, pipeline, res);
            },

            // cleanup
            dispose() {
                scene.Clear();
                res.DestroyAllTextures();
                res.DestroyAllModels();
                res.DestroyAllPrograms();
                hg.DestroyForwardPipeline(pipeline);
            },
        };
    }, { resetFlags: hg.RF_VSync | hg.RF_MSAA4X, ...options });
}
