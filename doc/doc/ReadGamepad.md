Read the current state of a named gamepad. If no name is passed, `default` is implied.

On macOS with the GLFW backend, `default` prefers the first connected Apple
GameController extended gamepad, falling back to GLFW slot 0 when none is
available. Native devices can also be read explicitly as `gamecontroller_slot_0`
through `gamecontroller_slot_15`. The existing `gamepad_slot_*` readers retain
their GLFW behavior. Apple-managed Bluetooth controllers may be visible to GLFW
while their HID input values remain frozen; use `default` or a native reader for
these devices.

Native stick Y axes and triggers use the same ranges as the GLFW readers:
positive Y points down, and triggers range from -1 (released) to +1 (fully pressed).

See [GetGamepadNames].
