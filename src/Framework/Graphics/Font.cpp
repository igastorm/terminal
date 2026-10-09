#include "Font.hpp"

//  ========================================================
//
//  Font
//
//  ========================================================

int Font::release() {
  int ref_count = this->Object<IFont>::release();
  if (ref_count == 0) {
    this->~Font();
    return 0;
  }
  return ref_count;
}

FontCellSize Font::getCellSize() const {
  return {this->half_width, this->height};
}

Font::Font(float half_width, float height)
    : half_width(half_width), height(height) {
  this->addRef();
}

Font::~Font() {}
