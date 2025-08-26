#ifndef SHM_PIXBUF_READER_H
#define SHM_PIXBUF_READER_H

#include <cstddef>
#include <string>

#include "shm.h"
#include "shm_pixbuf_data.h"
#include "timeout_sem.h"


class ShmPixbufReader {
 public:
  ShmPixbufReader(const std::string& path);

  // Callers are required to run any get or read functions between calls to acquire
  // and release.
  void acquire() { current_generation_ = sem_.wait(UINT64_MAX); }
  void release() { sem_.post(current_generation_); }
  int32_t get_width();
  int32_t get_height();
  int32_t get_pixels_size();
  // Will 'get_pixels_size()' bytes of data to 'out_pixels'.
  void read_pixels(uint8_t* out_pixels);

  // Exposed for testing.
  ShmPixbufData& get_data() { return *data_; };
  Shm& get_shm() {return shm_; };
  Semaphore& sem() { return sem_; };

 private:
  Shm shm_;
  ShmPixbufData* data_;
  Semaphore sem_;
  Semaphore::gen current_generation_;

  #ifdef DEBUG
  bool acquired_shm_ = false;
  #endif
};

#endif // SHM_PIXBUF_READER_H
