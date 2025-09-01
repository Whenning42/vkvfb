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

#include <cstdio>
#include <cstdlib>
#include <string>

#include "pixbuf/reader.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <pixbuf_path>\n", argv[0]);
        return 1;
    }

    std::string pixbuf_path = argv[1];

    ShmPixbufReader reader(pixbuf_path);
    
    const ReadPixbuf& result = reader.read_pixels();
    if (result.status != StatusVal::OK) {
        printf("Failed to read pixels\n");
        return 1;
    }
    
    int32_t width = result.width;
    int32_t height = result.height;
    int32_t pixels_size = width * height * 4; // Assume RGBA format
    uint8_t* pixels = result.pixels;
    
    FILE* dim_file = fopen("tests/out/snapshot_dim.txt", "w");
    if (!dim_file) {
        printf("Failed to open tests/out/snapshot_dim.txt for writing\n");
        return 1;
    }
    fprintf(dim_file, "%d %d\n", width, height);
    fclose(dim_file);
    
    FILE* raw_file = fopen("tests/out/snapshot.raw", "wb");
    if (!raw_file) {
        printf("Failed to open tests/out/snapshot.raw for writing\n");
        return 1;
    }
    fwrite(pixels, 1, pixels_size, raw_file);
    fclose(raw_file);
    
    printf("Snapshot saved: %dx%d (%d bytes)\n", width, height, pixels_size);
    
    return 0;
}
