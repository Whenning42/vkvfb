// Copyright 2020 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Modifications:
// Copyright 2025 William Henning
//   I re-use much of the Status, StatusOr, and RETURN_IF_ERROR interfaces, but
//   this implementation is my own.

#ifndef STATUS_OR_H_
#define STATUS_OR_H_

#include <cstdlib>
#include <type_traits>

#include "logger.h"

enum class ErrorCode {
  OK = 0,
  UNKNOWN = 1,
  NOT_FOUND = 2,
  // Used for miscellaneous errors that don't warrant their own values.
  GENERAL = 3,
};

class StatusVal {
 public:
  explicit StatusVal(ErrorCode code) : code_(code) {}

  bool ok() const { return code_ == ErrorCode::OK; }
  ErrorCode code() const { return code_; }

 private:
  ErrorCode code_ = ErrorCode::OK;
};

template <typename T>
class StatusOr {
 public:
  // Construct StatusOr with T values.
  StatusOr(const T& value) : status_(StatusVal(ErrorCode::OK)) {
    new (&value_) T(value);
  }

  StatusOr(T&& value) : status_(StatusVal(ErrorCode::OK)) {
    new (&value_) T(std::move(value));
  }

  // Construct StatusOr with a StatusVal.
  StatusOr(const StatusVal& status) : status_(status) {}
  StatusOr(StatusVal&& status) : status_(std::move(status)) {}

  StatusOr(const StatusOr& other) : status_(other.status_) {
    if (other.has_value()) {
      new (&value_) T(other.value_);
    }
  }

  StatusOr(StatusOr&& other) : status_(std::move(other.status_)) {
    if (other.has_value()) {
      new (&value_) T(std::move(other.value_));
    }
  }

  StatusOr& operator=(const StatusOr& other) {
    if (this != &other) {
      if (has_value()) {
        value_.~T();
      }
      status_ = other.status_;
      if (other.has_value()) {
        new (&value_) T(other.value_);
      }
    }
    return *this;
  }

  StatusOr& operator=(StatusOr&& other) {
    if (this != &other) {
      if (has_value()) {
        value_.~T();
      }
      status_ = std::move(other.status_);
      if (other.has_value()) {
        new (&value_) T(std::move(other.value_));
      }
    }
    return *this;
  }

  ~StatusOr() {
    if (has_value()) {
      value_.~T();
    }
  }

  bool ok() const { return status_.ok(); }
  const StatusVal& status() const { return status_; }

  // value() is undefined if this StatusOr doesn't hold a value.
  T& value() { return value_; }
  const T& value() const { return value_; }

  // Dereference results are undefined if this StatusOr doesn't hold a value.
  T& operator*() { return value_; }
  const T& operator*() const { return value_; }
  T* operator->() { return &value_; }
  const T* operator->() const { return &value_; }

  // Returns value if present, otherwise ERROR() logs and exit(1)
  T& value_or_die() {
    if (!has_value()) {
      ERROR("value_or_die() called on error status");
      exit(1);
    }
    return value_;
  }

 private:
  bool has_value() const { return ok(); }

  StatusVal status_ = StatusVal(ErrorCode::UNKNOWN);
  union {
    T value_;
  };
};

#define RETURN_IF_ERROR(expr)    \
  do {                           \
    auto status_or = (expr);     \
    if (!status_or.ok()) {       \
      return status_or.status(); \
    }                            \
  } while (0)

#endif  // STATUS_OR_H_
