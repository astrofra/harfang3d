hg = require("harfang")

function CreatePhysicCubeEx(scene, size, mtx, model_ref, materials, rb_type, mass)
	local rb_type = rb_type or hg.RBT_Dynamic
	local mass = mass or 0
	local node = hg.CreateObject(scene, mtx, model_ref, materials)
	node:SetName("Physic Cube")
	local rb = scene:CreateRigidBody()
	rb:SetType(rb_type)
	node:SetRigidBody(rb)
	local col = scene:CreateCollision()
	col:SetType(hg.CT_Cube)
	col:SetSize(size)
	col:SetMass(mass)
	node:SetCollision(0, col)
	return node, rb
end

function GetNodes(scene, paths)
	local nodes = {}
	for _, path in ipairs(paths) do
		local key = string.match(path, "([^/]+)$")
		nodes[key] = scene:GetNodeEx(path)
	end
	return nodes
end

function CreateAnchors(nodes, joint_paths)
	local anchors = {}
	for _, path in ipairs(joint_paths) do
		local key = path .. "_anchor"   
		anchors[key] = hg.TransformationMat4(nodes[path]:GetTransform():GetPos(), nodes[path]:GetTransform():GetRot())
	end
	return anchors
end

local node_paths = {
	"chest",
	"chest/chest_bone",
	"chest/chest_joint_to_head",
	"chest/chest_joint_to_right_arm_2",
	"chest/chest_joint_to_right_arm_3",
	"chest/chest_joint_to_left_arm_2",
	"chest/chest_joint_to_left_arm_3",
	"chest/chest_joint_to_right_leg",
	"chest/chest_joint_to_right_leg_2",
	"chest/chest_joint_to_right_leg_3",
	"chest/chest_joint_to_right_leg_4",
	"chest/chest_joint_to_left_leg",
	"chest/chest_joint_to_left_leg_2",
	"chest/chest_joint_to_left_leg_3",
	"chest/chest_joint_to_left_leg_4",
	"left_leg/left_leg_joint_to_chest",
	"left_leg/left_leg_joint_to_chest_2",
	"left_leg/left_leg_joint_to_chest_3",
	"left_leg/left_leg_joint_to_chest_4",
	"right_leg/right_leg_joint_to_chest",
	"right_leg/right_leg_joint_to_chest_2",
	"right_leg/right_leg_joint_to_chest_3",
	"right_leg/right_leg_joint_to_chest_4",
	"right_arm/right_arm_joint_to_chest",
	"right_arm/right_arm_joint_to_chest_2",
	"left_arm/left_arm_joint_to_chest",
	"left_arm/left_arm_joint_to_chest_2",
	"head/head_joint_to_chest",
	"left_leg/left_leg_joint_to_ground",
	"left_leg/left_leg_joint_to_ground_2",
	"left_leg/left_leg_joint_to_ground_3",
	"right_leg/right_leg_joint_to_ground",
	"right_leg/right_leg_joint_to_ground_2",
	"right_leg/right_leg_joint_to_ground_3",
	"ground/ground_joint_to_left_leg",
	"ground/ground_joint_to_left_leg_2",
	"ground/ground_joint_to_left_leg_3",
	"ground/ground_joint_to_right_leg",
	"ground/ground_joint_to_right_leg_2",
	"ground/ground_joint_to_right_leg_3",
	"right_arm",
	"right_arm/right_arm_bone",
	"left_arm",
	"left_arm/left_arm_bone",
	"right_leg",
	"right_leg/right_leg_bone",
	"left_leg",
	"left_leg/left_leg_bone",
	"head",
	"head/head_bone",
	"ground"
}

local joint_paths = {
	"right_arm_joint_to_chest",
	"right_arm_joint_to_chest_2",
	"left_arm_joint_to_chest",
	"left_arm_joint_to_chest_2",
	"right_leg_joint_to_chest",
	"right_leg_joint_to_chest_2",
	"right_leg_joint_to_chest_3",
	"right_leg_joint_to_chest_4",
	"left_leg_joint_to_chest",
	"left_leg_joint_to_chest_2",
	"left_leg_joint_to_chest_3",
	"left_leg_joint_to_chest_4",
	"head_joint_to_chest",
	"chest_joint_to_head",
	"chest_joint_to_right_arm_2",
	"chest_joint_to_right_arm_3",
	"chest_joint_to_left_arm_2",
	"chest_joint_to_left_arm_3",
	"chest_joint_to_right_leg",
	"chest_joint_to_right_leg_2",
	"chest_joint_to_right_leg_3",
	"chest_joint_to_right_leg_4",
	"chest_joint_to_left_leg",
	"chest_joint_to_left_leg_2",
	"chest_joint_to_left_leg_3",
	"chest_joint_to_left_leg_4",
	"left_leg_joint_to_ground",
	"left_leg_joint_to_ground_2",
	"left_leg_joint_to_ground_3",
	"right_leg_joint_to_ground",
	"right_leg_joint_to_ground_2",
	"right_leg_joint_to_ground_3",
	"ground_joint_to_left_leg",
	"ground_joint_to_left_leg_2",
	"ground_joint_to_left_leg_3",
	"ground_joint_to_right_leg",
	"ground_joint_to_right_leg_2",
	"ground_joint_to_right_leg_3"
}

hg.AddAssetsFolder('assets_compiled')

hg.InputInit()
hg.WindowSystemInit()

local res_x, res_y = 1280, 720
local win = hg.RenderInit('Physics Test', res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X)

local pipeline = hg.CreateForwardPipeline()
local res = hg.PipelineResources()

local scene = hg.Scene()
hg.LoadSceneFromAssets("cube/cube_remastered.scn", scene, res, hg.GetForwardPipelineInfo())

local cam_mat = hg.TransformationMat4(hg.Vec3(0, 7, -15), hg.Vec3(hg.Deg(10), 0, 0))
local cam = hg.CreateCamera(scene, cam_mat, 0.01, 1000)
local c = cam:GetCamera()
local projection_matrix = hg.ComputePerspectiveProjectionMatrix(c:GetZNear(), c:GetZFar(), hg.FovToZoomFactor(c:GetFov()), hg.Vec2(res_x / res_y, 1))

scene:SetCurrentCamera(cam)

local physics = hg.SceneBullet3Physics()
physics:SceneCreatePhysicsFromAssets(scene)
local physics_step = hg.time_from_sec_f(1 / 60)
local dt_frame_step = hg.time_from_sec_f(1 / 60)

local clocks = hg.SceneClocks()

local nodes = GetNodes(scene, node_paths)
local anchors = CreateAnchors(nodes, joint_paths)

physics:Add6DofConstraint(nodes.chest, nodes.right_arm, anchors.chest_joint_to_right_arm_2_anchor, anchors.right_arm_joint_to_chest_anchor)
physics:Add6DofConstraint(nodes.chest, nodes.right_arm, anchors.chest_joint_to_right_arm_3_anchor, anchors.right_arm_joint_to_chest_2_anchor)
physics:Add6DofConstraint(nodes.chest, nodes.left_arm, anchors.chest_joint_to_left_arm_2_anchor, anchors.left_arm_joint_to_chest_anchor)
physics:Add6DofConstraint(nodes.chest, nodes.left_arm, anchors.chest_joint_to_left_arm_3_anchor, anchors.left_arm_joint_to_chest_2_anchor)
physics:Add6DofConstraint(nodes.chest, nodes.right_leg, anchors.chest_joint_to_right_leg_anchor, anchors.right_leg_joint_to_chest_anchor)
physics:Add6DofConstraint(nodes.chest, nodes.right_leg, anchors.chest_joint_to_right_leg_2_anchor, anchors.right_leg_joint_to_chest_2_anchor)
physics:Add6DofConstraint(nodes.chest, nodes.right_leg, anchors.chest_joint_to_right_leg_3_anchor, anchors.right_leg_joint_to_chest_3_anchor)
physics:Add6DofConstraint(nodes.chest, nodes.left_leg, anchors.chest_joint_to_left_leg_anchor, anchors.left_leg_joint_to_chest_anchor)
physics:Add6DofConstraint(nodes.chest, nodes.left_leg, anchors.chest_joint_to_left_leg_2_anchor, anchors.left_leg_joint_to_chest_2_anchor)
physics:Add6DofConstraint(nodes.chest, nodes.left_leg, anchors.chest_joint_to_left_leg_3_anchor, anchors.left_leg_joint_to_chest_3_anchor)
physics:Add6DofConstraint(nodes.chest, nodes.head, anchors.chest_joint_to_head_anchor, anchors.head_joint_to_chest_anchor)

physics:Add6DofConstraint(nodes.left_leg, nodes.ground, anchors.left_leg_joint_to_ground_anchor, anchors.ground_joint_to_left_leg_anchor)
physics:Add6DofConstraint(nodes.left_leg, nodes.ground, anchors.left_leg_joint_to_ground_2_anchor, anchors.ground_joint_to_left_leg_2_anchor)
physics:Add6DofConstraint(nodes.left_leg, nodes.ground, anchors.left_leg_joint_to_ground_3_anchor, anchors.ground_joint_to_left_leg_3_anchor)
physics:Add6DofConstraint(nodes.right_leg, nodes.ground, anchors.right_leg_joint_to_ground_anchor, anchors.ground_joint_to_right_leg_anchor)
physics:Add6DofConstraint(nodes.right_leg, nodes.ground, anchors.right_leg_joint_to_ground_2_anchor, anchors.ground_joint_to_right_leg_2_anchor)
physics:Add6DofConstraint(nodes.right_leg, nodes.ground, anchors.right_leg_joint_to_ground_3_anchor, anchors.ground_joint_to_right_leg_3_anchor)

local keyboard = hg.Keyboard()

while not keyboard:Down(hg.K_Escape) and hg.IsWindowOpen(win) do
	keyboard:Update()
	physics:NodeWake(nodes.chest)

	local view_id = 0

    hg.SceneUpdateSystems(scene, clocks, dt_frame_step, physics, physics_step, 3)
	view_id, _ = hg.SubmitSceneToPipeline(view_id, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res)

	hg.Frame()
	hg.UpdateWindow(win)
end

scene:Clear()
scene:GarbageCollect()

hg.RenderShutdown()
hg.DestroyWindow(win)
hg.WindowSystemShutdown()
hg.InputShutdown()
