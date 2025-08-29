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

#include "shm_pixbuf_writer.h"

#include <cassert>
#include <cstring>

#include "shm_pixbuf_data.h"

namespace {

void memcpy_pixels(void* dst, const void* src, size_t size, bool force_opaque) {
  if (!force_opaque) {
    memcpy(dst, src, size);
    return;
  }

  // Check alignment for uint32_t operations
  assert(((uintptr_t)src & 3) == 0 && "src must be 4-byte aligned");
  assert(((uintptr_t)dst & 3) == 0 && "dst must be 4-byte aligned");

  // Memcpy clamping every 4th byte to 255.
  uint32_t* from = (uint32_t*)(src);
  uint32_t* to = (uint32_t*)(dst);
  uint32_t* end = (uint32_t*)(src) + size/4;
  while (from != end) {
    *to = *from | 0xff000000u;
    from++;
    to++;
  }
}

}

ShmPixbufWriter::ShmPixbufWriter(const std::string& path):
  sem_(path + "_sem", /*create=*/true, /*initial_value=*/1) {
  current_generation_ = sem_.wait(2'000'000'000);
  // LOGF("Acquired write create sem: %s\n", sem_.name().c_str());
  shm_ = Shm(path, 'w');
  data_ = new (shm_.map()) ShmPixbufData();
  sem_.post(current_generation_);
  // LOGF("Released write create sem: %s\n", sem_.name().c_str());
}

void ShmPixbufWriter::write_pixels(const uint8_t* pixels, int32_t width, int32_t height, bool force_opaque) {
  // Wait maximum 2 seconds for semaphore
  current_generation_ = sem_.wait(2'000'000'000);
  
  size_t new_shm_size = ShmPixbufData::pixbuf_struct_size(width, height);
  size_t pixbuf_size = ShmPixbufData::pixbuf_size(width, height);
  shm_.resize(new_shm_size);
  data_ = (ShmPixbufData*)shm_.map();
  data_->width = width;
  data_->height = height;
  memcpy_pixels(&data_->first_pixel, pixels, pixbuf_size, force_opaque);
  
  sem_.post(current_generation_);
}
