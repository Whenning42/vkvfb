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

#ifndef GENERIC_UNIQUE_PTR_H_
#define GENERIC_UNIQUE_PTR_H_

#include <memory>

class generic_unique_ptr {
 public:
  generic_unique_ptr() : ptr_(nullptr, [](void*){}) {}
  // Used by make_generic_unique.
  generic_unique_ptr(void* p, void(*deleter)(void*)) : ptr_(p, deleter) {}
  
  // Is moveable.
  generic_unique_ptr(generic_unique_ptr&& other) = default;
  generic_unique_ptr& operator=(generic_unique_ptr&& other) = default;
  
  // Isn't copyable.
  generic_unique_ptr(const generic_unique_ptr&) = delete;
  generic_unique_ptr& operator=(const generic_unique_ptr&) = delete;
  
  // Exposes get().
  void* get() const { return ptr_.get(); }

 private:
  std::unique_ptr<void, void(*)(void*)> ptr_;
};

// Takes in a newly allocated pointer to T, i.e. usage is:
//   make_generic_unique(new T(...));
template <typename T> generic_unique_ptr make_generic_unique(T* obj) {
  return generic_unique_ptr(obj, [](void* p){ delete static_cast<T*>(p); });
}

#endif  // GENERIC_UNIQUE_PTR_H_
