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

#ifndef SHM_PIXBUF_DATA_H
#define SHM_PIXBUF_DATA_H

#include <atomic>
#include <mutex>
#include <string>
#include <cstdint>
#include <cstddef>
#include <fcntl.h>
#include <semaphore.h>

struct ShmPixbufData {
  // Field accesses are expected to be synchronized between processes by callers
  int32_t width = 0;
  int32_t height = 0;
  uint8_t* pixels;
  // A width*height*4 sized block of pixels, that starts here.
  uint8_t first_pixel;

  ShmPixbufData(): pixels(&first_pixel) {}

  static size_t pixbuf_size(int32_t width, int32_t height) {
    // Casting to size_t before multiplication protects against possible overflows at very high resolutions.
    return (size_t)width * (size_t)height * 4;
  }
  static size_t pixbuf_struct_size(int32_t width, int32_t height) {
    return pixbuf_size(width, height) + offsetof(ShmPixbufData, first_pixel);
  }
};

#endif
