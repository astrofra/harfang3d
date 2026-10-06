// Create and draw models without a pipeline and using ModelBuilder

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	return runWindow('Harfang - Draw and Create Models using ModelBuilder - no Pipeline', () => {
		// vertex layout, materials and models
		const vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

		const mdl_builder = new hg.ModelBuilder();

		// Below are the 6 faces of the cube model, with 4 vertices declared for each face, that are added as 2 triangles to make a complete quad face

		// -
		let vertex0 = new hg.Vertex(); // Vertex constructor
		vertex0.pos = new hg.Vec3(-0.5, -0.5, -0.5); // Declaring the vertex position
		vertex0.normal = new hg.Vec3(0, 0, -1); // Declaring the normal
		vertex0.uv0 = new hg.Vec2(0, 0); // Declaring the UV
		let a = mdl_builder.AddVertex(vertex0); // Adding the vertex to the ModelBuilder

		let vertex1 = new hg.Vertex();
		vertex1.pos = new hg.Vec3(-0.5, 0.5, -0.5);
		vertex1.normal = new hg.Vec3(0, 0, -1);
		vertex1.uv0 = new hg.Vec2(0, 1);
		let b = mdl_builder.AddVertex(vertex1);

		let vertex2 = new hg.Vertex();
		vertex2.pos = new hg.Vec3(0.5, 0.5, -0.5);
		vertex2.normal = new hg.Vec3(0, 0, -1);
		vertex2.uv0 = new hg.Vec2(1, 1);
		let c = mdl_builder.AddVertex(vertex2);

		let vertex3 = new hg.Vertex();
		vertex3.pos = new hg.Vec3(0.5, -0.5, -0.5);
		vertex3.normal = new hg.Vec3(0, 0, -1);
		vertex3.uv0 = new hg.Vec2(1, 0);
		let d = mdl_builder.AddVertex(vertex3);

		mdl_builder.AddTriangle(d, c, b); // Adding the first triangle of the current face
		mdl_builder.AddTriangle(b, a, d); // Second triangle

		// +
		vertex0 = new hg.Vertex();
		vertex0.pos = new hg.Vec3(-0.5, -0.5, 0.5);
		vertex0.normal = new hg.Vec3(0, 0, 1);
		vertex0.uv0 = new hg.Vec2(0, 0);
		a = mdl_builder.AddVertex(vertex0);

		vertex1 = new hg.Vertex();
		vertex1.pos = new hg.Vec3(-0.5,0.5, 0.5);
		vertex1.normal = new hg.Vec3(0, 0, 1);
		vertex1.uv0 = new hg.Vec2(0, 1);
		b = mdl_builder.AddVertex(vertex1);

		vertex2 = new hg.Vertex();
		vertex2.pos = new hg.Vec3(0.5, 0.5, 0.5);
		vertex2.normal = new hg.Vec3(0, 0, 1);
		vertex2.uv0 = new hg.Vec2(1, 1);
		c = mdl_builder.AddVertex(vertex2);

		vertex3 = new hg.Vertex();
		vertex3.pos = new hg.Vec3(0.5, -0.5, 0.5);
		vertex3.normal = new hg.Vec3(0, 0, 1);
		vertex3.uv0 = new hg.Vec2(1, 0);
		d = mdl_builder.AddVertex(vertex3);

		mdl_builder.AddTriangle(a, b, c);
		mdl_builder.AddTriangle(a, c, d);

		// -
		vertex0 = new hg.Vertex();
		vertex0.pos = new hg.Vec3(-0.5, -0.5, -0.5);
		vertex0.normal = new hg.Vec3(0, -1, 0);
		vertex0.uv0 = new hg.Vec2(0, 0);
		a = mdl_builder.AddVertex(vertex0);

		vertex1 = new hg.Vertex();
		vertex1.pos = new hg.Vec3(-0.5, -0.5, 0.5);
		vertex1.normal = new hg.Vec3(0, -1, 0);
		vertex1.uv0 = new hg.Vec2(0, 1);
		b = mdl_builder.AddVertex(vertex1);

		vertex2 = new hg.Vertex();
		vertex2.pos = new hg.Vec3(0.5, -0.5, 0.5);
		vertex2.normal = new hg.Vec3(0, -1, 0);
		vertex2.uv0 = new hg.Vec2(1, 1);
		c = mdl_builder.AddVertex(vertex2);

		vertex3 = new hg.Vertex();
		vertex3.pos = new hg.Vec3(0.5, -0.5, -0.5);
		vertex3.normal = new hg.Vec3(0, -1, 0);
		vertex3.uv0 = new hg.Vec2(1, 0);
		d = mdl_builder.AddVertex(vertex3);

		mdl_builder.AddTriangle(a, b, c);
		mdl_builder.AddTriangle(a, c, d);

		// +
		vertex0 = new hg.Vertex();
		vertex0.pos = new hg.Vec3(-0.5, 0.5, -0.5);
		vertex0.normal = new hg.Vec3(0, 1, 0);
		vertex0.uv0 = new hg.Vec2(0, 0);
		a = mdl_builder.AddVertex(vertex0);

		vertex1 = new hg.Vertex();
		vertex1.pos = new hg.Vec3(-0.5, 0.5, 0.5);
		vertex1.normal = new hg.Vec3(0, 1, 0);
		vertex1.uv0 = new hg.Vec2(0, 1);
		b = mdl_builder.AddVertex(vertex1);

		vertex2 = new hg.Vertex();
		vertex2.pos = new hg.Vec3(0.5, 0.5, 0.5);
		vertex2.normal = new hg.Vec3(0, 1, 0);
		vertex2.uv0 = new hg.Vec2(1, 1);
		c = mdl_builder.AddVertex(vertex2);

		vertex3 = new hg.Vertex();
		vertex3.pos = new hg.Vec3(0.5, 0.5, -0.5);
		vertex3.normal = new hg.Vec3(0, 1, 0);
		vertex3.uv0 = new hg.Vec2(1, 0);
		d = mdl_builder.AddVertex(vertex3);

		mdl_builder.AddTriangle(d, c, b);
		mdl_builder.AddTriangle(b, a, d);

		// -
		vertex0 = new hg.Vertex();
		vertex0.pos = new hg.Vec3(-0.5, -0.5, -0.5);
		vertex0.normal = new hg.Vec3(-1, 0, 0);
		vertex0.uv0 = new hg.Vec2(0, 0);
		a = mdl_builder.AddVertex(vertex0);

		vertex1 = new hg.Vertex();
		vertex1.pos = new hg.Vec3(-0.5, -0.5, 0.5);
		vertex1.normal = new hg.Vec3(-1, 0, 0);
		vertex1.uv0 = new hg.Vec2(0, 1);
		b = mdl_builder.AddVertex(vertex1);

		vertex2 = new hg.Vertex();
		vertex2.pos = new hg.Vec3(-0.5, 0.5, 0.5);
		vertex2.normal = new hg.Vec3(-1, 0, 0);
		vertex2.uv0 = new hg.Vec2(1, 1);
		c = mdl_builder.AddVertex(vertex2);

		vertex3 = new hg.Vertex();
		vertex3.pos = new hg.Vec3(-0.5, 0.5, -0.5);
		vertex3.normal = new hg.Vec3(-1, 0, 0);
		vertex3.uv0 = new hg.Vec2(1, 0);
		d = mdl_builder.AddVertex(vertex3);

		mdl_builder.AddTriangle(d, c, b);
		mdl_builder.AddTriangle(b, a, d);

		// +
		vertex0 = new hg.Vertex();
		vertex0.pos = new hg.Vec3(0.5, -0.5, -0.5);
		vertex0.normal = new hg.Vec3(1, 0, 0);
		vertex0.uv0 = new hg.Vec2(0, 0);
		a = mdl_builder.AddVertex(vertex0);

		vertex1 = new hg.Vertex();
		vertex1.pos = new hg.Vec3(0.5, -0.5, 0.5);
		vertex1.normal = new hg.Vec3(1, 0, 0);
		vertex1.uv0 = new hg.Vec2(0, 1);
		b = mdl_builder.AddVertex(vertex1);

		vertex2 = new hg.Vertex();
		vertex2.pos = new hg.Vec3(0.5, 0.5, 0.5);
		vertex2.normal = new hg.Vec3(1, 0, 0);
		vertex2.uv0 = new hg.Vec2(1, 1);
		c = mdl_builder.AddVertex(vertex2);

		vertex3 = new hg.Vertex();
		vertex3.pos = new hg.Vec3(0.5, 0.5, -0.5);
		vertex3.normal = new hg.Vec3(1, 0, 0);
		vertex3.uv0 = new hg.Vec2(1, 0);
		d = mdl_builder.AddVertex(vertex3);

		mdl_builder.AddTriangle(a, b, c);
		mdl_builder.AddTriangle(a, c, d);

		const cube_mdl = mdl_builder.MakeModel(vtx_layout); // Create the actuel cube model

		const ground_mdl = hg.CreatePlaneModel(vtx_layout, 5, 5, 1, 1);

		const shader = hg.LoadProgramFromAssets('shaders/mdl');

		// Own the models' GPU buffers.
		const res = new hg.PipelineResources();
		res.AddModel('cube', cube_mdl);
		res.AddModel('ground', ground_mdl);

		// main loop
		let angle = 0;

		return {
			draw(dt, res_x, res_y) {
				angle += hg.time_to_sec_f(dt);

				const viewpoint = hg.TranslationMat4(new hg.Vec3(0, 1, -3));
				hg.SetViewPerspective(0, 0, 0, res_x, res_y, viewpoint);

				hg.DrawModel(0, cube_mdl, shader, [], [], hg.TransformationMat4(
					new hg.Vec3(0, 1, 0), new hg.Vec3(angle, angle, angle)));
				hg.DrawModel(0, ground_mdl, shader, [], [],
					hg.TranslationMat4(new hg.Vec3(0, 0, 0)));
			},

			// cleanup
			dispose() {
				res.DestroyAllModels();
				hg.DestroyProgram(shader);
			},
		};
	}, { resetFlags: hg.RF_VSync | hg.RF_MSAA4X, ...options });
}
