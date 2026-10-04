#include "Bitmap.hpp"
#include <cstdlib>

//  ========================================================
//
//  Bitmap
//
//  ========================================================

int Bitmap::addRef() { return ++this->ref_count; }

int Bitmap::release() {
  if (--this->ref_count == 0) {
    this->~Bitmap();
    free(this);
    return 0;
  }
  return this->ref_count;
}

Bitmap::Bitmap(std::size_t width, std::size_t height, void *bitmap_data)
    : width(width), height(height), bitmap_data(bitmap_data) {}

Bitmap::~Bitmap() {
  if (this->bitmap_data != nullptr) {
    std::free(this->bitmap_data);
    this->bitmap_data = nullptr;
  }
}

std::size_t Bitmap::getWidth() const { return this->width; }

std::size_t Bitmap::getHeight() const { return this->height; }

void *Bitmap::getBitmapData() const { return this->bitmap_data; }
