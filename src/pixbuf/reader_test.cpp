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
#include "writer.h"
#include "utility.h"

#include <gtest/gtest.h>
#include <mutex>
#include <cstdlib>
#include <cstring>

void EXPECT_PIXBUF_EQ(const ReadPixbuf& pixbuf, uint8_t* other_data, int32_t other_w, int32_t other_h) {
  EXPECT_EQ(pixbuf.status, StatusVal::OK);
  EXPECT_EQ(pixbuf.width, other_w);
  EXPECT_EQ(pixbuf.height, other_h);
  size_t data_size = other_w * other_h * 4;
  EXPECT_EQ(memcmp(pixbuf.pixels, other_data, data_size), 0);
}

TEST(ShmPixbuf, ReadWriteTest) {
  ShmPixbufWriter writer = ShmPixbufWriter("test_buf");
  ShmPixbufReader reader = ShmPixbufReader("test_buf");

  size_t pixels_0_size = 320*240*4;
  uint8_t* pixels_0 = (uint8_t*)malloc(pixels_0_size);
  memset(pixels_0, 0, pixels_0_size);

  size_t pixels_1_size = 640*480*4;
  uint8_t* pixels_1 = (uint8_t*)malloc(pixels_1_size);
  memset(pixels_1, 1, pixels_1_size);


  // Read and write buffer 0, expecting the correct data and shm size.
  writer.write_pixels(pixels_0, 320, 240);
  const ReadPixbuf& result_0 = reader.read_pixels();
  EXPECT_PIXBUF_EQ(result_0, pixels_0, 320, 240);
  EXPECT_EQ(reader.get_shm().size(), round_to_page(ShmPixbufData::pixbuf_struct_size(320, 240)));

  // Read and write buffer 1, expecting the correct data and shm size.
  writer.write_pixels(pixels_1, 640, 480);
  const ReadPixbuf& result_1 = reader.read_pixels();
  EXPECT_PIXBUF_EQ(result_1, pixels_1, 640, 480);
  EXPECT_EQ(reader.get_shm().size(), round_to_page(ShmPixbufData::pixbuf_struct_size(640, 480)));

  // Read and write buffer 0, expecting the correct data and shm size.
  // We check buffer 0 again after buffer 1 to ensure the reader both grows and
  // shrinks its mapped shm as necessary.
  writer.write_pixels(pixels_0, 320, 240);
  const ReadPixbuf& result_0_again = reader.read_pixels();
  EXPECT_PIXBUF_EQ(result_0_again, pixels_0, 320, 240);
  EXPECT_EQ(reader.get_shm().size(), round_to_page(ShmPixbufData::pixbuf_struct_size(320, 240)));

  free(pixels_0);
  free(pixels_1);
}
