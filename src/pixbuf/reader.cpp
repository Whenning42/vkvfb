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

#include "reader.h"

#include <cstring>
#include <cassert>

#include "data.h"


ReadPixbuf::ReadPixbuf(ReadPixbuf&& other) noexcept 
  : status(other.status), width(other.width), height(other.height), pixels(other.pixels) {
  other.pixels = nullptr;
}

ReadPixbuf& ReadPixbuf::operator=(ReadPixbuf&& other) noexcept {
  if (this != &other) {
    free(pixels);
    status = other.status;
    width = other.width;
    height = other.height;
    pixels = other.pixels;
    other.pixels = nullptr;
  }
  return *this;
}

void ReadPixbuf::update(int32_t new_w, int32_t new_h, const uint8_t* data) {
  size_t data_size = ShmPixbufData::pixbuf_size(new_w, new_h);
  if (new_w != width || new_h != height) {
    free(pixels);
    width = new_w;
    height = new_h;
    pixels = (uint8_t*)malloc(data_size);
  }
  memcpy(pixels, data, data_size);
}

ShmPixbufReader::ShmPixbufReader(const std::string& path) : mu_(path + "_mu", /*create=*/false) {
  shm_ = Shm(path, 'r', ShmPixbufData::pixbuf_struct_size(0, 0));
  data_ = (ShmPixbufData*)shm_.map();
}

const ReadPixbuf& ShmPixbufReader::read_pixels() {
  LockResult lock = mu_.mu().lock(kOneSecNanos);
  if (lock.state == LockState::OWNERDEAD || lock.state == LockState::TIMEOUT) {
    read_pixbuf_.status = StatusVal::FAILED;
    return read_pixbuf_;
  }

  int32_t w = data_->width;
  int32_t h = data_->height;
  size_t pixbuf_struct_size = ShmPixbufData::pixbuf_struct_size(w, h);
  shm_.resize(pixbuf_struct_size);
  data_ = (ShmPixbufData*)shm_.map();
  read_pixbuf_.update(w, h, &data_->first_pixel);
  read_pixbuf_.status = StatusVal::OK;
  
  return read_pixbuf_;
}
