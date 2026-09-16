#include "CommonWindow.hpp"
#include "../Application/CommonApplication.hpp"
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

CommonWindow::CommonWindow(IApplication *appInstance) {
  this->addRef();
  if (appInstance != nullptr) {
    this->appInstance = appInstance;
    static_cast<CommonApplication *>(this->appInstance)->addRef();
  }
}

CommonWindow::~CommonWindow() {
  if (this->appInstance != nullptr) {
    static_cast<CommonApplication*>(this->appInstance)->release();
    this->appInstance = nullptr;
  }
}
