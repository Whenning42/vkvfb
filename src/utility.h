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

#ifndef UTILITY_H_
#define UTILITY_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

// If the given expression evaluates to zero, calls perror to print errno and exits.
#define CCHECK_ERRNO(expr, msg) \
{ \
  int r = expr; \
  if (r) { \
    perror(msg); \
    exit(1); \
  } \
}

// If the given expression evaluates to a non-zero value, prints the error given
// by the return value.
#define CCHECK_EVAL(expr, msg) \
{ \
  int r__ = expr; \
  if (r__) { \
    char errbuf[256]; \
    char* error_s = strerror_r(r__, errbuf, sizeof(errbuf)); \
    fprintf(stderr, "%s: error: %d,  %s\n", msg, r__, error_s); \
    exit(1); \
  } \
}

#define CCHECK(condition, message, error_val) \
{ \
  if (!(condition)) { \
    char error_str[256]; \
    strerror_r(error_val, error_str, sizeof(error_str)); \
    fprintf(stderr, "%s: %s\n", message, error_str); \
    exit(1); \
  } \
}

inline size_t round_to_page(size_t size) {
  size_t ps = getpagesize();
  return ps * ((size + ps - 1) / ps);
}

#endif  // UTILITY_H_
