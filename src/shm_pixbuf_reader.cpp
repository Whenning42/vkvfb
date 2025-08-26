#include "shm_pixbuf_reader.h"

#include <cstring>
#include <cassert>

#include "shm_pixbuf_data.h"

ShmPixbufReader::ShmPixbufReader(const std::string& path) 
  : shm_(path, 'r'), sem_(path + "_sem", false, 1) {
  data_ = (ShmPixbufData*)shm_.map();
}

int32_t ShmPixbufReader::get_width() {
  #ifdef DEBUG
  assert(acquired_shm_);
  #endif
  return data_->width;
}

int32_t ShmPixbufReader::get_height() {
  #ifdef DEBUG
  assert(acquired_shm_);
  #endif
  return data_->height;
}

int32_t ShmPixbufReader::get_pixels_size() {
  #ifdef DEBUG
  assert(acquired_shm_);
  #endif
  return ShmPixbufData::pixbuf_size(data_->width, data_->height);
}

void ShmPixbufReader::read_pixels(uint8_t* out_pixels) {
  #ifdef DEBUG
  assert(acquired_shm_);
  #endif

  int32_t w = get_width();
  int32_t h = get_height();

  size_t pixbuf_struct_size = ShmPixbufData::pixbuf_struct_size(w, h);
  shm_.resize(pixbuf_struct_size);
  data_ = (ShmPixbufData*)shm_.map();
  memcpy(out_pixels, &data_->first_pixel, ShmPixbufData::pixbuf_size(w, h));
}
