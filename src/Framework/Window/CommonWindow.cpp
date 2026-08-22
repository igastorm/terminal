#include "CommonWindow.hpp"
#include <cstdlib>

int CommonWindow::addRef(void) { return ++this->ref_count; }

int CommonWindow::release(void) {
  if (--this->ref_count == 0) {
    this->~CommonWindow();
    free(this);
    return 0;
  }
  return this->ref_count;
}
