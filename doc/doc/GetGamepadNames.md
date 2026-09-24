Return a list of names for all supported gamepad devices on the system.

These are registered input readers, including unoccupied slots. Check the
`IsConnected` state returned by [ReadGamepad] to determine whether a reader has a
connected device. On macOS with the GLFW backend, `gamecontroller_slot_0` through
`gamecontroller_slot_15` expose Apple's native GameController input in addition
to the GLFW readers. `default` prefers a connected native controller.

See [ReadGamepad].
