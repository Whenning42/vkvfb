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

#include "pixbuf_reader.h"

#include <cassert>
#include <cstring>

#include "pixbuf_data.h"
#include "status_or.h"

ReadPixbuf::ReadPixbuf(ReadPixbuf&& other) noexcept
    : code(other.code),
      width(other.width),
      height(other.height),
      pixels(other.pixels) {
  other.pixels = nullptr;
}

ReadPixbuf& ReadPixbuf::operator=(ReadPixbuf&& other) noexcept {
  if (this != &other) {
    free(pixels);
    code = other.code;
    width = other.width;
    height = other.height;
    pixels = other.pixels;
    other.pixels = nullptr;
  }
  return *this;
}

void ReadPixbuf::update(int32_t new_w, int32_t new_h, const uint8_t* data) {
  size_t data_size = PixbufData::pixbuf_size(new_w, new_h);
  if (new_w != width || new_h != height) {
    free(pixels);
    width = new_w;
    height = new_h;
    pixels = (uint8_t*)malloc(data_size);
  }
  memcpy(pixels, data, data_size);
}

StatusOr<PixbufReader> PixbufReader::Create(const std::string& path) {
  StatusOr<ShmMutex> mu_result =
      ShmMutex::Create(path + "_mu", /*create=*/false);
  RETURN_IF_ERROR(mu_result);

  StatusOr<Shm> shm =
      Shm::Create(path, 'r', PixbufData::pixbuf_struct_size(0, 0));
  RETURN_IF_ERROR(shm);

  return PixbufReader(std::move(*mu_result), std::move(*shm));
}

PixbufReader::PixbufReader(ShmMutex&& mu, Shm&& shm)
    : mu_(std::move(mu)), shm_(std::move(shm)) {
  data_ = (PixbufData*)shm_.map();
}

const ReadPixbuf& PixbufReader::read_pixels() {
  LockResult lock = mu_.mu().lock(kOneSecNanos);
  if (lock.state == LockState::OWNERDEAD || lock.state == LockState::TIMEOUT) {
    read_pixbuf_.code = ErrorCode::GENERAL;
    return read_pixbuf_;
  }

  int32_t w = data_->width;
  int32_t h = data_->height;
  size_t pixbuf_struct_size = PixbufData::pixbuf_struct_size(w, h);
  shm_.resize(pixbuf_struct_size);
  data_ = (PixbufData*)shm_.map();
  read_pixbuf_.update(w, h, &data_->first_pixel);
  read_pixbuf_.code = ErrorCode::OK;

  return read_pixbuf_;
}
