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

#ifndef PRESENT_CALLBACK_H
#define PRESENT_CALLBACK_H

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vulkan/vulkan.h>

#include "shm_pixbuf_writer.h"


// Struct to store and pass to present callback.
struct PresentData {
  int32_t width;
  int32_t height;
  ShmPixbufWriter writer;
  VkCompositeAlphaFlagBitsKHR composite_mode;

  PresentData(int32_t w, int32_t h, const std::string& window_name, VkCompositeAlphaFlagBitsKHR mode)
    : width(w), height(h), writer(window_name), composite_mode(mode) {}
};

void present_callback(void* user_data, uint8_t* pixels, size_t pixels_size);
void cleanup_callback(void* user_data);

#endif  // PRESENT_CALLBACK_H
