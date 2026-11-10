Orbit a camera around a target using a quaternion arcball, exponential dolly and screen-space pan. The controller is independent of scenes and rendering.

Construct it with a target position, a positive distance and an optional camera-world orientation quaternion. The default constructor starts at a distance of 3.4 from the origin, looking along +Z. `Reset()` restores the pose captured at construction. Copies have independent state and retain this reset pose.

`target`, `orientation` and `distance` are editable. `min_distance`, `max_distance` and `dolly_speed` default to 0.15, 50 and 0.12. Supply positive, ordered distance limits. Limits are applied on update and reset, so a larger range can be configured after construction without losing the requested reset distance.

Two `Update` overloads are available:

- `Update(mouse, width, height, fov, drag_valid)` uses the left button to orbit, right/middle buttons to pan and the wheel to dolly. It suppresses motion on the first frame any of these buttons is pressed. Set `drag_valid` to false when reacquiring focus; wheel input remains enabled.
- `Update(orbit, pan, x, y, dx, dy, wheel, width, height, fov)` takes explicit values for custom input and deterministic playback.

The vertical field of view `fov` is in radians and defaults to pi/3. The mouse overload's `drag_valid` defaults to true. Pointer positions/deltas and viewport dimensions must use the same units, with Y pointing down. Use client dimensions for client-coordinate mouse input, including on displays where framebuffer dimensions differ. Empty viewports ignore updates; panning requires a field of view between 0 and pi. Rotations preserve roll; an exactly antipodal drag leaves the orientation unchanged.

Call `mouse:Update()` before updating the controller. The application handles window focus and keyboard shortcuts. `ComputeWorld()` returns the camera world transform, ready for `ComputePerspectiveViewState`; it does not return the inverse view matrix.

The mouse overload reads `WheelDelta()` so fractional scroll moves the camera
on every update. The explicit-input overload already accepts a floating-point
`wheel` value. Legacy integer-only mouse readers remain supported.

```lua
local camera = hg.TrackballController(hg.Vec3(0, 0, 0), 3.4)
local fov = math.pi / 3

-- inside the frame loop, after updating mouse and keyboard
if focused then
	if keyboard:Pressed(hg.K_R) then
		camera:Reset()
	end
	camera:Update(mouse, client_width, client_height, fov, was_focused)
end
local camera_world = camera:ComputeWorld()
```
