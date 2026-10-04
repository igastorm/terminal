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

Font::Font() {}

Font::~Font() {}
