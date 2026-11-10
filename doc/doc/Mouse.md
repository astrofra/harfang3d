Helper class holding the current and previous device state to enable delta state queries.

Use [GetMouseNames] to query for available mouse devices.

`Wheel()` and `HWheel()` retain their legacy integer return values. Use
`WheelDelta()` and `HWheelDelta()` for floating-point vertical/horizontal scroll,
including slow high-resolution wheel and trackpad movements. Deltas represent
the events collected since the previous read, not a cumulative wheel position.

`MouseState` keeps the integer `wheel`/`hwheel` fields and adds zero-initialized
float `wheel_delta`/`hwheel_delta` fields with the same `WheelDelta()`/
`HWheelDelta()` accessors. Legacy/custom readers that fill only integer fields
remain supported: a zero float field falls back to the corresponding integer.
The GLFW backend fills both representations before clearing its event totals.

This preserves the existing source API. Native clients embedding `MouseState`
must be rebuilt with the updated headers because its structure has grown.
