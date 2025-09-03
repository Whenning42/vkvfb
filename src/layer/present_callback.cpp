/*
 * Copyright (C) 2025 William Henning
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "present_callback.h"

#include <cstdlib>

#include "logger.h"

SwapchainData::SwapchainData(int32_t w, int32_t h, PixbufWriter&& writer_param,
                             VkCompositeAlphaFlagBitsKHR mode)
    : width(w),
      height(h),
      writer(std::move(writer_param)),
      composite_mode(mode) {}

void present_callback(void* user_data, uint8_t* pixels, size_t pixels_size) {
  SwapchainData& swapchain_data = *(SwapchainData*)user_data;
  bool force_opaque = false;
  if (swapchain_data.composite_mode == VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
    force_opaque = true;
  }

  swapchain_data.writer.write_pixels(pixels, swapchain_data.width, swapchain_data.height, force_opaque);
}

void cleanup_callback(void* user_data) {
  SwapchainData* swapchain_data = (SwapchainData*)user_data;
  delete swapchain_data;
}
