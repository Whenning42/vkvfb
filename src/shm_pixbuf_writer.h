#ifndef SHM_PIXBUF_WRITER_H
#define SHM_PIXBUF_WRITER_H

#include <cstddef>
#include <string>

#include "shm.h"
#include "shm_pixbuf_data.h"
#include "timeout_sem.h"

class ShmPixbufWriter {
 public:
  ShmPixbufWriter(const std::string& path);
  void write_pixels(const uint8_t* pixels, int32_t width, int32_t height);

 private:
   Shm shm_;
   ShmPixbufData* data_;
   Semaphore sem_;
   Semaphore::gen current_generation_;
};

#endif // SHM_PIXBUF_WRITER_H
