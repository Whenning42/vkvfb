Vkvfb is a Vulkan layer that replaces Vulkan's default presentation behavior with
presentation to a shared memory pixel buffer. This makes it a useful tool for
building window capture or streaming tools, especially if you want to run these
tools in contexts where you can access the graphics card, but don't have an accelerated
presentation path, such as Xvfb nested X11 sessions.

Note: Vkvfb only supports Linux and X11 with no plans to expand support.

# Getting Started

Dependencies: Vulkan, Gtest, Xlib, Xcb, and Xvfb (for some tests).

```
meson setup build
(cd build; meson compile)
(cd build; meson test --print-errorlogs) # Optional, run tests.
```

For example usage, see [factorio_test.sh](tests/factorio_test.sh)

<> TODO: Update usage when we've got python pixbuf reader bindings available.

# Acknowledgements

This project is forked from
[vk_callback_swapchain](https://github.com/google/vk_callback_swapchain) and retains
much of its code. Thank you vk_callback_swapchain devs!

# Roadmap

This project should be fairly stable at this point. I might add support for Windows,
Mac, or Wayland, but it's not likely. I'll also add any missing presentation support
such as supporting presentation of non-RGBA8 images, on an as needed basis for
supporting apps that I'm interested in.

# Contributing

I'll make an effort to engage with issues, feature requests, or pull requests, but
no guarentees.
