#include "Window.hpp"
#include "../Application/Application.hpp"
#include <cstdlib>

int Window::addRef() { return ++this->ref_count; }

int Window::release() {
  if (--this->ref_count == 0) {
    this->~Window();
    free(this);
    return 0;
  }
  return this->ref_count;
}

Window::Window(IApplication *appInstance) {
  this->addRef();
  if (appInstance != nullptr) {
    this->appInstance = appInstance;
    static_cast<Application *>(this->appInstance)->addRef();
  }
}

Window::~Window() {
  if (this->appInstance != nullptr) {
    static_cast<Application*>(this->appInstance)->release();
    this->appInstance = nullptr;
  }
}
