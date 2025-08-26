#ifndef SHM_H
#define SHM_H

#include <string>

class Shm {
 public:
  // Mode must be either 'r' or 'w'.
  // In 'w' mode, a new shm is opened at the give path.
  // In 'r' mode, it's required that a shm already exists at path and it's opened
  // for reading.
  Shm(const std::string& path, char mode);
  void resize(size_t new_size);
  void* map() { return map_; }
  size_t size() { return size_; }

 private:
  int shm_fd_;
  char mode_;
  size_t size_;
  void* map_;
};

#endif
