Initialize the render system.

The initial backbuffer uses the window's drawable pixel size, including Retina scaling. Overloads that create a window take its dimensions in screen coordinates; query [GetWindowFrameBufferSize] afterward for rendering dimensions.

Viewports and scene submission rectangles must use drawable pixels too. Existing applications that use the requested window dimensions for rendering should switch to [GetWindowFrameBufferSize] or [RenderResetToWindow].

To change the states of the render system afterward use [RenderReset].
