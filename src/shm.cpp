#include "shm.h"
#include "shm_pixbuf_data.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>

Shm::Shm(const std::string& path, char mode) : mode_(mode) {
  int flags = O_RDWR;
  if (mode == 'w') {
    flags |= O_CREAT;
  }
  
  shm_fd_ = shm_open(path.c_str(), flags, 0644);
  if (shm_fd_ == -1) {
    perror("Failed to open shared memory");
    exit(1);
  }
  
  size_ = ShmPixbufData::pixbuf_struct_size(0, 0);
  
  if (mode == 'w') {
    if (ftruncate(shm_fd_, size_) == -1) {
      perror("Failed to truncate shared memory");
      close(shm_fd_);
      exit(1);
    }
  }
  
  map_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);
  if (map_ == MAP_FAILED) {
    perror("Failed to map shared memory");
    close(shm_fd_);
    exit(1);
  }
}

void Shm::resize(size_t new_size) {
  if (new_size == size_) {
    return;
  }
  
  if (mode_ == 'w') {
    if (ftruncate(shm_fd_, new_size) == -1) {
      perror("Failed to resize shared memory");
      exit(1);
    }
  }
  
  void* new_map = mremap(map_, size_, new_size, MREMAP_MAYMOVE);
  if (new_map == MAP_FAILED) {
    perror("Failed to remap shared memory");
    exit(1);
  }
  
  map_ = new_map;
  size_ = new_size;
}
