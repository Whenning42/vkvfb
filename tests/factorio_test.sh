#!/bin/bash

# DISABLE_SWAPCHAIN=1 \
# LD_LIBRARY_PATH=$(pwd)/third_party/Vulkan-ValidationLayers/build/layers:$LD_LIBRARY_PATH \
# VK_LOADER_LAYERS_ENABLE="VK_LAYER_KHRONOS_validation" \
# VK_LAYER_PATH=$(pwd)/third_party/Vulkan-ValidationLayers/build/layers \
# VK_LOADER_DEBUG=all \
# LD_PRELOAD=/usr/lib/libasan.so \

VK_LAYER_PATH=$(pwd)/build/
VK_LOADER_LAYERS_ENABLE="VK_LAYER_VKVFB_vkvfb" \
MESA_LOADER_DRIVER_OVERRIDE=zink \
gdb ~/Games/factorio/bin/x64/factorio
