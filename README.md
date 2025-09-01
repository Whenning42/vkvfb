Vkvfb is a Vulkan layer that directs an application's presented frames to a
shared-memory buffer instead of the desktop. This makes it a performant and low-level
tool for enabling application rendering in contexts where you have access to Vulkan, but
not to a hardware accelerated display path (e.g. running a game under Xvfb).

I've written it to simplify the graphics stack of my downstream project [BounceRL](https://github.com/Whenning42/bounce-rl), 
a tool for connecting AI agents to games. Vkvfb is also interesting in its own
right as an example of how to do Vulkan layer-based frame capture and how to manage a
synchronized and resizable shared memory segment ([src/layer/](src/layer/) and
[src/ipc/](src/ipc/) respectively).

Note: Vkvfb only supports Linux and X11.

# Getting Started

Dependencies: Vulkan, GTest, Xlib, XCB, and Xvfb (for some tests).

```sh
meson setup build
meson compile -C build
meson test -C build --print-errorlogs # Optional: run tests
```

For demos of enabling and using the layer see
[factorio_test.sh](tests/factorio_test.sh) and [vfbmon](tests/vfbmon.cpp). Note: Vfbmon
requires you to pass in the X11 window id of the app you're capturing. You can get this
from xwininfo, xlib/xcb, or from looking at vkvfb's /dev/shm files and guessing which
one corresponds to your window.

To build something with vkvfb, you'd specify the loader env vars yourself and directly
use the PixbufReader class.

<!-- TODO: Update usage when we've got Python pixbuf reader bindings available. -->

# Acknowledgements

This project is forked from
[vk_callback_swapchain](https://github.com/google/vk_callback_swapchain) and retains
much of its code. Thank you vk_callback_swapchain devs!

# Roadmap

This project should be fairly stable. I'll add additional platforms
(Mac/Windows/Wayland) or presentation support as needed for my use cases.

# Contributing

Issues, feature requests, or pull requests are all welcome, but no guarantees that
I'll respond to them. For major fixes or changes, open an issue before submitting
a pull request.

# Related Projects

For other examples of vulkan capture layers, check out:

- [obs-vkcapture](https://github.com/nowrep/obs-vkcapture/tree/master): A capture layer for OBS.
- [pyrofling](https://github.com/Themaister/pyrofling): A capture layer for audio and video streaming.
- [vk_callback_swapchain](https://github.com/google/vk_callback_swapchain): A swapchain implementation that calls a provided callback.

