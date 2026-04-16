local hg = require("harfang")

local Camera = {}

function Camera.compute_view_state(document, rect)
	local width = math.max(rect.w, 1)
	local height = math.max(rect.h, 1)
	local aspect = hg.ComputeAspectRatioX(width, height)

	local scene_camera = document.scene:GetCurrentCamera()
	if scene_camera ~= nil and scene_camera:IsValid() and scene_camera:HasCamera() then
		return scene_camera:ComputeCameraViewState(aspect), scene_camera:GetTransform():GetWorld()
	end

	local editor_camera = document.editor_camera
	if editor_camera.orthographic then
		return hg.ComputeOrthographicViewState(editor_camera.world, editor_camera.orthographic_size, editor_camera.znear, editor_camera.zfar, aspect), editor_camera.world
	end

	return hg.ComputePerspectiveViewState(editor_camera.world, editor_camera.fov, editor_camera.znear, editor_camera.zfar, aspect), editor_camera.world
end

return Camera
