Begin an ImGui frame. This function must be called once per frame before any other ImGui call.

When using multiple contexts, it must be called for each context you intend to use during the current frame.

The overload with a final `mouse_scale` vector converts client-coordinate mouse
positions into the supplied display dimensions. When `width` and `height` are
framebuffer pixels, pass `Vec2(framebuffer_width / client_width,
framebuffer_height / client_height)`, using the current window sizes. This keeps
mouse hit testing aligned on Retina/HiDPI displays and after resizing. The
existing overloads retain a scale of `(1, 1)`.

See [ImGuiEndFrame].
