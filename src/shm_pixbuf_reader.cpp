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

#include "shm_pixbuf_reader.h"

#include <cstring>
#include <cassert>

#include "shm_pixbuf_data.h"
#include "tinylog.h"

ShmPixbufReader::ShmPixbufReader(const std::string& path) 
  : shm_(path, 'r'), sem_(path + "_sem", false, 1) {
  data_ = (ShmPixbufData*)shm_.map();
}

void ShmPixbufReader::acquire() {
  current_generation_ = sem_.wait(UINT64_MAX);
  LOGF(kLogSync, "Acquired reader sem: %s\n", sem_.name().c_str());
}
void ShmPixbufReader::release() {
  sem_.post(current_generation_);
  LOGF(kLogSync, "Released reader sem: %s\n", sem_.name().c_str());
}

int32_t ShmPixbufReader::get_width() {
  #ifdef DEBUG
  assert(acquired_shm_);
  #endif
  return data_->width;
}

int32_t ShmPixbufReader::get_height() {
  #ifdef DEBUG
  assert(acquired_shm_);
  #endif
  return data_->height;
}

int32_t ShmPixbufReader::get_pixels_size() {
  #ifdef DEBUG
  assert(acquired_shm_);
  #endif
  return ShmPixbufData::pixbuf_size(data_->width, data_->height);
}

void ShmPixbufReader::read_pixels(uint8_t* out_pixels) {
  #ifdef DEBUG
  assert(acquired_shm_);
  #endif

  int32_t w = get_width();
  int32_t h = get_height();

  size_t pixbuf_struct_size = ShmPixbufData::pixbuf_struct_size(w, h);
  shm_.resize(pixbuf_struct_size);
  data_ = (ShmPixbufData*)shm_.map();
  memcpy(out_pixels, &data_->first_pixel, ShmPixbufData::pixbuf_size(w, h));
}
