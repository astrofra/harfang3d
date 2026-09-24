Resize the renderer backbuffer to the window's drawable pixel dimensions, including Retina scaling. The input/output `width` and `height` are backbuffer dimensions in pixels, not window coordinates.

Return true and update the dimensions if a reset was needed and carried out. Return false without changing them if they already match, the window query fails, or either drawable dimension is zero. This avoids resetting the renderer to an empty surface while a window is minimized.

After a successful reset, update viewports, projections and other resources that depend on the backbuffer size. [GetWindowClientSize] remains available for window layout in screen coordinates.
