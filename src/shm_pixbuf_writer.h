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

#ifndef SHM_PIXBUF_WRITER_H_
#define SHM_PIXBUF_WRITER_H_

#include <cstddef>
#include <string>

#include "shm.h"
#include "shm_mutex.h"
#include "shm_pixbuf_data.h"
#include "timeout_sem.h"

class ShmPixbufWriter {
 public:
  ShmPixbufWriter(const std::string& path);
  void write_pixels(const uint8_t* pixels, int32_t width, int32_t height, bool force_opaque=false);

 private:
   // Protects shm_ and data_.
   ShmMutex mu_;
   Shm shm_;
   ShmPixbufData* data_;
};

#endif // SHM_PIXBUF_WRITER_H_
