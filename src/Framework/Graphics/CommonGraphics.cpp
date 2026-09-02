#include "CommonGraphics.hpp"
#include <cstdlib>

int CommonGraphics::addRef() { return ++this->ref_count; }

int CommonGraphics::release() {
  if (--this->ref_count == 0) {
    this->~CommonGraphics();
    free(this);
    return 0;
  }
  return this->ref_count;
}
