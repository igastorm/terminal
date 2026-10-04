#include "Font.hpp"
#include <cstdlib>

//  ========================================================
//
//  Font
//
//  ========================================================

int Font::addRef() { return ++this->ref_count; }

int Font::release() {
  if (--this->ref_count == 0) {
    this->~Font();
    free(this);
    return 0;
  }
  return this->ref_count;
}

FontCellSize Font::getCellSize() const {
  return {this->half_width, this->height};
}

Font::Font(float half_width, float height)
    : half_width(half_width), height(height) {
  this->addRef();
}

Font::~Font() {}
