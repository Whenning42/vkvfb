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

#ifndef PIXBUF_PIXBUF_READER_H_
#define PIXBUF_PIXBUF_READER_H_

#include <cstddef>
#include <string>

#include "constants.h"
#include "ipc/pmutex.h"
#include "ipc/shm.h"
#include "ipc/shm_mutex.h"
#include "pixbuf/pixbuf_data.h"
#include "status_or.h"

struct ReadPixbuf {
  ErrorCode code = ErrorCode::OK;
  int32_t width = 0;
  int32_t height = 0;
  uint8_t* pixels = nullptr;

  ReadPixbuf() = default;
  ~ReadPixbuf() { free(pixels); }

  // ReadPixbuf is moveable, but not copyable.
  ReadPixbuf(const ReadPixbuf&) = delete;
  ReadPixbuf& operator=(const ReadPixbuf&) = delete;
  ReadPixbuf(ReadPixbuf&& other) noexcept;
  ReadPixbuf& operator=(ReadPixbuf&& other) noexcept;

  void update(int32_t new_w, int32_t new_h, const uint8_t* data);
};

class PixbufReader {
 public:
  // Factory function to create a PixbufReader.
  // 'path' should be the name of the window you're trying to connect to.
  // You can get it from xwininfo, or from the bottom of `ls -1tr /dev/shm`.
  // Returns StatusOr<PixbufReader> with a NOT_FOUND status if the shared-memory
  // at 'path' cannot be opened.
  static StatusOr<PixbufReader> Create(const std::string& path);

  const ReadPixbuf& read_pixels();

  // Exposed for testing.
  PixbufData& get_data() { return *data_; };
  Shm& get_shm() { return shm_; };

 private:
  // Private constructor, use Create() instead.
  PixbufReader(ShmMutex&& mu, Shm&& shm);

  // shm_ and data_ are protected by mu_.
  ShmMutex mu_;
  Shm shm_;
  PixbufData* data_;

  ReadPixbuf read_pixbuf_;
};

#endif  // PIXBUF_PIXBUF_READER_H_
