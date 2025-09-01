Vkvfb is a Vulkan layer that captures frames to a shared-memory buffer. It can
be used to build capture or streaming tools or to run apps when you have Vulkan access
but no Vulkan display path (e.g. Xvfb sessions on a machine with a graphics card).

Note: Vkvfb only supports Linux and X11.

# Getting Started

Dependencies: Vulkan, GTest, Xlib, XCB, and Xvfb (for some tests).

```
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
