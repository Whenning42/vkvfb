#ifndef SHM_PIXBUF_DATA_H
#define SHM_PIXBUF_DATA_H

#include <atomic>
#include <mutex>
#include <string>
#include <cstdint>
#include <cstddef>
#include <fcntl.h>
#include <semaphore.h>

struct ShmPixbufData {
  // Field accesses are expected to be synchronized between processes by callers
  int32_t width = 0;
  int32_t height = 0;
  uint8_t* pixels;
  // A width*height*4 sized block of pixels, that starts here.
  uint8_t first_pixel;

  ShmPixbufData(): pixels(&first_pixel) {}

  static size_t pixbuf_size(int32_t width, int32_t height) {
    return width*height*4;
  }
  static size_t pixbuf_struct_size(int32_t width, int32_t height) {
    return pixbuf_size(width, height) + offsetof(ShmPixbufData, first_pixel);
  }
};

#endif
