Return the window drawable width and height in pixels. On Retina displays this can be twice the client size on each axis. Query the drawable directly instead of multiplying the client size by the content scale: the relationship is platform dependent.

Use these dimensions with [RenderReset], [SetViewRect], [SetView2D] and scene submission rectangles. [NewWindow] and [SetWindowClientSize] still use screen coordinates.

Returns false and zero dimensions for a null window. A valid window may temporarily have zero drawable dimensions; skip rendering until both dimensions are positive.

The drawable size may change when resizing the window or moving it between displays. [RenderResetToWindow] queries it and resets the backbuffer when needed; applications must also update their viewports and projections.
