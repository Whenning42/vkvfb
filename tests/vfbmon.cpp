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

#include "pixbuf/reader.h"

// Return epoch time in seconds.
double time_sec() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Sleep for the given amount of time.
void sleep_sec(double seconds) {
  if (seconds > 0) {
    struct timespec ts = {
      (time_t)seconds,
      (long)((seconds - (time_t)seconds) * 1e9)
    };
    nanosleep(&ts, nullptr);
  }
}

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
    
    width_ = 0;
    height_ = 0;
    window_ = None;
    image_ = nullptr;
    pixels_ = nullptr;
  }
  
  void run() {
    printf("Starting VFB monitor for window ID: %s\n", window_id_.c_str());
    const double frame_duration = 1.0 / 60.0; // 60 FPS in seconds
    
    while (true) {
      double frame_start = time_sec();
      update_frame();

      // Keep the event queue processed.
      while (XPending(display_)) {
        XEvent event;
        XNextEvent(display_, &event);
      }
      
      // Sleep till next read.
      double frame_end = time_sec();
      sleep_sec(frame_duration - (frame_end - frame_start));
    }
  }

 private:
  void create_window(int width, int height) {
    if (window_ != None) {
      XDestroyWindow(display_, window_);
    }
    
    XSetWindowAttributes attrs;
    attrs.background_pixel = BlackPixel(display_, screen_);
    attrs.event_mask = 0;

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

  void resize_window_and_image(int32_t width, int32_t height, uint8_t* data) {
      if (image_) {
        free(image_);
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
                           reinterpret_cast<char*>(data),
                           width, height, 32, width * 4);
      if (!image_) {
        printf("Failed to create XImage\n");
        exit(1);
      }
      image_->byte_order = LSBFirst;
      image_->bitmap_bit_order = LSBFirst;
  }
  
  bool update_frame() {
    const ReadPixbuf& read = reader_.read_pixels();
    if (read.status != StatusVal::OK) {
      return false;
    }
    
    int32_t width = read.width;
    int32_t height = read.height;
    
    if (width <= 0 || height <= 0) {
      return false;
    }
    if (width != width_ || height != height_) {
      width_ = width;
      height_ = height;
      resize_window_and_image(width, height, read.pixels);
    }
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
  
  std::string window_id_;
  ShmPixbufReader reader_;
  Display* display_;
  int screen_;
  Window root_window_;
  Window window_;
  GC gc_;
  XImage* image_;
  uint8_t* pixels_;
  int32_t width_;
  int32_t height_;
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
