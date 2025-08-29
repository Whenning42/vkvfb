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

#ifndef LOGGER_H
#define LOGGER_H

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/types.h>

inline const char* kLogLayer = "LAYER";
inline const char* kLogSync = "SYNC";

namespace logger {

// TODO: Consider making out file controllable.

inline std::mutex mu;
inline int fd = STDOUT_FILENO;

inline void log(const char* channel, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);

  // TODO: Consider implementing channel filtering.

  timespec time;
  clock_gettime(CLOCK_REALTIME, &time);

  tm local_time;
  localtime_r(&time.tv_sec, &local_time);

  char time_s[64];
  int time_slen = snprintf(
    time_s,
    64,
    "%d/%02d/%02d %02d:%02d:%02d.%06ld",
    local_time.tm_year + 1900,
    local_time.tm_mon + 1,
    local_time.tm_mday,
    local_time.tm_hour,
    local_time.tm_min,
    local_time.tm_sec,
    time.tv_nsec / 1000
  );
  if (time_slen < 0) {
    perror("snprintf for time string.");
    time_slen = 0;
  }

  char process_s[32];
  int process_slen = snprintf(process_s, 32, " tid: %d", gettid());
  if (process_slen < 0) {
    perror("snprintf for process string.");
    process_slen = 0;
  }

  char log_s[2048];
  int log_slen = vsnprintf(log_s, 2048, fmt, args);
  if (log_slen < 0) {
    perror("snprintf for log string.");
    log_slen = 0;
  }

  std::lock_guard<std::mutex> lock(mu);
  iovec writes[4] = {
    {time_s, (size_t)time_slen},
    {process_s, (size_t)process_slen},
    {log_s, (size_t)log_slen},
    {const_cast<char*>("\n"), (size_t)1},
  };
  writev(fd, writes, 4);

  va_end(args);
}
}

#define LOG(...) ::logger::log(__VA_ARGS__)

#endif // LOGGER_H
