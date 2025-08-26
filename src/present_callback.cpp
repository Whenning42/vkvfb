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

#include "present_callback.h"

void present_callback(void* user_data, uint8_t* pixels, size_t pixels_size) {
  PresentData& present_data = *(PresentData*)user_data;
  present_data.writer.write_pixels(pixels, present_data.width, present_data.height);
}

void cleanup_callback(void* user_data) {
  PresentData* present_data = (PresentData*)user_data;
  delete present_data;
}
