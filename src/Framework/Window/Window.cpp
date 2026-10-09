#include "Window.hpp"

int Window::release() {
  int ref_count = this->Object<IWindow>::release();
  if (ref_count == 0) {
    this->~Window();
    return 0;
  }
  return ref_count;
}

Window::Window() { this->addRef(); }

Window::~Window() {}
