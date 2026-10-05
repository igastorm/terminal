#include "Window.hpp"
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

Window::Window() { this->addRef(); }

Window::~Window() {}
