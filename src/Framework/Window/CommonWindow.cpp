#include "CommonWindow.hpp"
#include <cstdlib>

int CommonWindow::addRef() { return ++this->ref_count; }

int CommonWindow::release() {
  if (--this->ref_count == 0) {
    this->~CommonWindow();
    free(this);
    return 0;
  }
  return this->ref_count;
}
