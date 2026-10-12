#include "Window.hpp"

int Window::release() {
  int ref_count = this->Object<IWindow>::release();
  if (ref_count == 0) {
    this->~Window();
    return 0;
  }
  return ref_count;
}

void Window::setUserData(void *user_data) { this->user_data = user_data; }

void *Window::getUserData() const { return this->user_data; }
