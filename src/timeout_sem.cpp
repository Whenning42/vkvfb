#include "timeout_sem.h"

#include <cstdlib>
#include <cstdio>
#include <fcntl.h>
#include <errno.h>
#include <time.h>


MPMu::MPMu(const std::string& name, int oflag, int mode) {
  sem_ = sem_open(name.c_str(), oflag, mode, 1);
  if (sem_ == SEM_FAILED) {
    perror("MPMu sem_open failed");
    exit(1);
  }
}

void MPMu::lock() {
  if (sem_wait(sem_)) {
    perror("MPmu lock");
    exit(1);
  }
}

void MPMu::unlock() {
  if (sem_post(sem_)) {
    perror("MPMu unlock");
    exit(1);
  }
}

Semaphore::Semaphore(const std::string& path, bool create, int32_t initial_value) 
  : sem_mu_(path + "_mu", create ? O_CREAT : 0, 0644), initial_value_(initial_value) {
  int oflag = 0;
  if (create) {
    oflag |= O_CREAT;
  }
  sem_ = sem_open(path.c_str(), oflag, 0644, initial_value);
  if (sem_ == SEM_FAILED) {
    perror("Semaphore sem_open failed");
    exit(1);
  }
}

void Semaphore::post(gen g) {
  sem_mu_.lock();
  if (g != generation_) {
    sem_mu_.unlock();
    return;
  }

  if (sem_post(sem_)) {
    perror("sem post");
    exit(1);
  }
  sem_mu_.unlock();
}

Semaphore::gen Semaphore::wait(uint64_t nanos) {
  struct timespec abs_timeout;
  if (nanos == UINT64_MAX) {
    // Wait indefinitely
    if (sem_wait(sem_)) {
      perror("sem_wait failed");
      exit(1);
    }
  } else {
    // Convert relative timeout to absolute time
    if (clock_gettime(CLOCK_REALTIME, &abs_timeout)) {
      perror("clock_gettime failed");
      exit(1);
    }
    
    abs_timeout.tv_sec += nanos / 1000000000;
    abs_timeout.tv_nsec += nanos % 1000000000;
    if (abs_timeout.tv_nsec >= 1000000000) {
      abs_timeout.tv_sec += 1;
      abs_timeout.tv_nsec -= 1000000000;
    }
    
    int ret = sem_timedwait(sem_, &abs_timeout);
    if (ret && errno == ETIMEDOUT) {
      advance_gen();
    } else if (ret) {
      perror("sem_timedwait failed");
      exit(1);
    }
  }
  return generation_;
}

void Semaphore::advance_gen() {
  sem_mu_.lock();
  generation_ += 1;

  // Get current semaphore value and post until we reach initial_value_
  int current_value;
  if (sem_getvalue(sem_, &current_value)) {
    perror("sem_getvalue failed");
    sem_mu_.unlock();
    exit(1);
  }

  while (current_value < initial_value_) {
    if (sem_post(sem_)) {
      perror("advance_gen post failed");
      sem_mu_.unlock();
      exit(1);
    }
    current_value++;
  }

  sem_mu_.unlock();
}

int32_t Semaphore::get_value() {
  int value;
  if (sem_getvalue(sem_, &value)) {
    perror("sem_getvalue failed");
    exit(1);
  }
  return static_cast<int32_t>(value);
}
