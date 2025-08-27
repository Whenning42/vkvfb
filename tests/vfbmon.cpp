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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <time.h>

#include "shm_pixbuf_reader.h"

class VfbMonitor {
 public:
  VfbMonitor(const std::string& window_id) : window_id_(window_id), reader_(window_id) {
    display_ = XOpenDisplay(nullptr);
    if (!display_) {
      printf("Failed to open X11 display\n");
      exit(1);
    }
    
    screen_ = DefaultScreen(display_);
    root_window_ = RootWindow(display_, screen_);
    
    current_width_ = 0;
    current_height_ = 0;
    window_ = None;
    image_ = nullptr;
    pixels_ = nullptr;
  }
  
  ~VfbMonitor() {
    cleanup();
    if (display_) {
      XCloseDisplay(display_);
    }
  }
  
  void run() {
    printf("Starting VFB monitor for window ID: %s\n", window_id_.c_str());
    
    const double frame_duration = 1.0 / 60.0; // 60 FPS in seconds
    
    while (true) {
      struct timespec frame_start;
      clock_gettime(CLOCK_MONOTONIC, &frame_start);
      double frame_start_sec = frame_start.tv_sec + frame_start.tv_nsec / 1'000'000'000.0;
      
      update_frame();
      handle_x11_events();
      
      struct timespec frame_end;
      clock_gettime(CLOCK_MONOTONIC, &frame_end);
      double frame_end_sec = frame_end.tv_sec + frame_end.tv_nsec / 1'000'000'000.0;
      
      double elapsed = frame_end_sec - frame_start_sec;
      double sleep_time = frame_duration - elapsed;
      
      if (sleep_time > 0) {
        struct timespec sleep_ts = {
          (time_t)sleep_time,
          (long)((sleep_time - (time_t)sleep_time) * 1'000'000'000.0)
        };
        nanosleep(&sleep_ts, nullptr);
      }
    }
  }

 private:
  void cleanup() {
    if (image_) {
      XDestroyImage(image_);
      image_ = nullptr;
    }
    if (pixels_) {
      delete[] pixels_;
      pixels_ = nullptr;
    }
    if (window_ != None && display_) {
      XDestroyWindow(display_, window_);
      window_ = None;
    }
  }
  
  void create_window(int width, int height) {
    if (window_ != None) {
      XDestroyWindow(display_, window_);
    }
    
    XSetWindowAttributes attrs;
    attrs.background_pixel = BlackPixel(display_, screen_);
    attrs.event_mask = ExposureMask;
    
    window_ = XCreateWindow(display_, root_window_,
                           100, 100, width, height, 0,
                           DefaultDepth(display_, screen_),
                           InputOutput,
                           DefaultVisual(display_, screen_),
                           CWBackPixel | CWEventMask,
                           &attrs);
    
    char title[256];
    snprintf(title, sizeof(title), "VFB Monitor - %s", window_id_.c_str());
    XStoreName(display_, window_, title);
    
    XMapWindow(display_, window_);
    XFlush(display_);
    
    gc_ = XCreateGC(display_, window_, 0, nullptr);
    
    printf("Created X11 window %dx%d\n", width, height);
  }
  
  void resize_window(int width, int height) {
    if (window_ != None) {
      XResizeWindow(display_, window_, width, height);
      XFlush(display_);
      printf("Resized X11 window to %dx%d\n", width, height);
    }
  }
  
  bool update_frame() {
    reader_.acquire();
    
    int32_t width = reader_.get_width();
    int32_t height = reader_.get_height();
    int32_t pixels_size = reader_.get_pixels_size();
    
    if (width <= 0 || height <= 0) {
      reader_.release();
      return false;
    }
    
    if (width != current_width_ || height != current_height_) {
      current_width_ = width;
      current_height_ = height;
      
      if (pixels_) {
        delete[] pixels_;
      }
      pixels_ = new uint8_t[pixels_size];
      
      if (image_) {
        XDestroyImage(image_);
      }
      
      if (window_ == None) {
        create_window(width, height);
      } else {
        resize_window(width, height);
      }
      
      image_ = XCreateImage(display_,
                           DefaultVisual(display_, screen_),
                           DefaultDepth(display_, screen_),
                           ZPixmap, 0,
                           reinterpret_cast<char*>(pixels_),
                           width, height, 32, width * 4);
      
      if (!image_) {
        reader_.release();
        printf("Failed to create XImage\n");
        exit(1);
      }
      
      image_->byte_order = LSBFirst;
      image_->bitmap_bit_order = LSBFirst;
    }
    
    reader_.read_pixels(pixels_);
    reader_.release();
    
    convert_bgra_to_rgba(pixels_, width * height);
    
    if (window_ != None && image_) {
      XPutImage(display_, window_, gc_, image_, 0, 0, 0, 0, width, height);
      XFlush(display_);
    }
    
    return true;
  }
  
  void convert_bgra_to_rgba(uint8_t* pixels, int pixel_count) {
    for (int i = 0; i < pixel_count; ++i) {
      uint8_t* pixel = &pixels[i * 4];
      std::swap(pixel[0], pixel[2]);
    }
  }
  
  void handle_x11_events() {
    while (XPending(display_)) {
      XEvent event;
      XNextEvent(display_, &event);
      
      switch (event.type) {
        case Expose:
          if (image_ && current_width_ > 0 && current_height_ > 0) {
            XPutImage(display_, window_, gc_, image_, 0, 0, 0, 0, 
                    current_width_, current_height_);
          }
          break;
      }
    }
  }
  
  std::string window_id_;
  ShmPixbufReader reader_;
  Display* display_;
  int screen_;
  Window root_window_;
  Window window_;
  GC gc_;
  XImage* image_;
  uint8_t* pixels_;
  int32_t current_width_;
  int32_t current_height_;
};

void print_usage(const char* program_name) {
  printf("Usage: %s <window_id>\n", program_name);
  printf("  window_id: VFB window identifier (hex or decimal)\n");
  printf("\n");
  printf("This program monitors a virtual framebuffer and displays it in an X11 window.\n");
  printf("The display updates at 60 FPS and automatically resizes when the VFB changes size.\n");
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    print_usage(argv[0]);
    return 1;
  }
  
  std::string window_id = argv[1];
  
  VfbMonitor monitor(window_id);
  monitor.run();
  
  return 0;
}