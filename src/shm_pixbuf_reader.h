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

#ifndef SHM_PIXBUF_READER_H
#define SHM_PIXBUF_READER_H

#include <cstddef>
#include <string>

#include "shm.h"
#include "shm_pixbuf_data.h"
#include "timeout_sem.h"
#include "shm_mutex.h"
#include "pmutex.h"
#include "constants.h"

enum class StatusVal {
  OK,
  FAILED
};

struct ReadPixbuf {
  StatusVal status = StatusVal::OK;
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

class ShmPixbufReader {
 public:
  ShmPixbufReader(const std::string& path);
  const ReadPixbuf& read_pixels();

  // Exposed for testing.
  ShmPixbufData& get_data() { return *data_; };
  Shm& get_shm() {return shm_; };

 private:
  // shm_ and data_ are protected by mu_.
  ShmMutex mu_;
  Shm shm_;
  ShmPixbufData* data_;

  ReadPixbuf read_pixbuf_;
};

#endif // SHM_PIXBUF_READER_H
