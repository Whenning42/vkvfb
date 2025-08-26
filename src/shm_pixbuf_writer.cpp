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

#include <cstring>

#include "shm_pixbuf_data.h"

ShmPixbufWriter::ShmPixbufWriter(const std::string& path) 
  : shm_(path, 'w'), sem_(path + "_sem", true, 1) {
  data_ = new (shm_.map()) ShmPixbufData();
}

void ShmPixbufWriter::write_pixels(const uint8_t* pixels, int32_t width, int32_t height) {
  // Wait maximum 0.2 seconds for semaphore
  current_generation_ = sem_.wait(200'000'000);
  
  size_t new_shm_size = ShmPixbufData::pixbuf_struct_size(width, height);
  size_t pixbuf_size = ShmPixbufData::pixbuf_size(width, height);
  shm_.resize(new_shm_size);
  data_ = (ShmPixbufData*)shm_.map();
  data_->width = width;
  data_->height = height;
  memcpy(&data_->first_pixel, pixels, pixbuf_size);
  
  sem_.post(current_generation_);
}
