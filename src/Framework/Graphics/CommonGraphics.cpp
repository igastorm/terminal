#include "CommonGraphics.hpp"
#include <cstdlib>

//  ----------------------------
//  Surface
//  ----------------------------

int CommonSurface::addRefBase() { return ++this->ref_count; }

int CommonSurface::release() {
  if (--this->ref_count == 0) {
    this->~CommonSurface();
    free(this);
    return 0;
  }
  return this->ref_count;
}

//  ----------------------------
//  Graphics Device
//  ----------------------------

int CommonGraphicsDevice::addRefBase() { return ++this->ref_count; }

int CommonGraphicsDevice::release() {
  if (--this->ref_count == 0) {
    this->~CommonGraphicsDevice();
    free(this);
    return 0;
  }
  return this->ref_count;
}
