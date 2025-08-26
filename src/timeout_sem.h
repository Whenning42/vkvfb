#ifndef TIMEOUT_SEM_H
#define TIMEOUT_SEM_H

#include <atomic>
#include <cstddef>
#include <semaphore.h>
#include <string>


class MPMu {
 public:
  MPMu(const std::string& name, int oflag, int mode);
  void lock();
  void unlock();

 private:
  sem_t* sem_ = nullptr;
};

// A generational semaphore that supports timeouts for waits that unlocks
// the semahpore if the thread can't acquire the semaphore within the specified time.
class Semaphore {
 public:
  using gen = int64_t;
  Semaphore(const std::string& path, bool create, int32_t initial_value);
  void post(gen g);
  // Pass in UINT64_MAX to wait indefintely.
  gen wait(uint64_t nanos);
  int32_t get_value();

 private:
  void advance_gen();

  std::atomic<gen> generation_ = 0;

  // Protects sem in the post and advance gen case.
  MPMu sem_mu_;
  sem_t* sem_;
  int32_t initial_value_;
};

#endif

