#include "shm_pixbuf_reader.h"
#include "shm_pixbuf_writer.h"

#include <gtest/gtest.h>
#include <mutex>
#include <cstdlib>
#include <cstring>

bool pixbuf_is_locked(ShmPixbufReader& reader) {
  return reader.sem().get_value() == 0;
}

TEST(ShmPixbufReader, AcquireReleaseToggleMutex) {
  ShmPixbufWriter writer = ShmPixbufWriter("test_buf");
  ShmPixbufReader reader = ShmPixbufReader("test_buf");

  EXPECT_FALSE(pixbuf_is_locked(reader));
  reader.acquire();
  EXPECT_TRUE(pixbuf_is_locked(reader));
  reader.release();
  EXPECT_FALSE(pixbuf_is_locked(reader));
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
  reader.acquire();
  EXPECT_EQ(reader.get_width(), 320);
  EXPECT_EQ(reader.get_height(), 240);
  uint8_t* pixels_0_read = (uint8_t*)malloc(pixels_0_size);
  reader.read_pixels(pixels_0_read);
  reader.release();
  EXPECT_EQ(memcmp(pixels_0, pixels_0_read, pixels_0_size), 0);
  EXPECT_EQ(reader.get_shm().size(), ShmPixbufData::pixbuf_struct_size(320, 240));

  // Read and write buffer 1, expecting the correct data and shm size.
  writer.write_pixels(pixels_1, 640, 480);
  reader.acquire();
  EXPECT_EQ(reader.get_width(), 640);
  EXPECT_EQ(reader.get_height(), 480);
  uint8_t* pixels_1_read = (uint8_t*)malloc(pixels_1_size);
  reader.read_pixels(pixels_1_read);
  reader.release();
  EXPECT_EQ(memcmp(pixels_1, pixels_1_read, pixels_1_size), 0);
  EXPECT_EQ(reader.get_shm().size(), ShmPixbufData::pixbuf_struct_size(640, 480));

  // Read and write buffer 0, expecting the correct data and shm size.
  // We check buffer 0 again after buffer 1 to ensure the reader both grows and
  // shrinks its mapped shm as necessary.
  writer.write_pixels(pixels_0, 320, 240);
  reader.acquire();
  EXPECT_EQ(reader.get_width(), 320);
  EXPECT_EQ(reader.get_height(), 240);
  reader.read_pixels(pixels_0_read);
  reader.release();
  EXPECT_EQ(memcmp(pixels_0, pixels_0_read, pixels_0_size), 0);
  EXPECT_EQ(reader.get_shm().size(), ShmPixbufData::pixbuf_struct_size(320, 240));

  free(pixels_0);
  free(pixels_1);
  free(pixels_0_read);
  free(pixels_1_read);
}
