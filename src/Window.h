#pragma once
#include "Object.h"

class Window : public Object {
public:
  virtual ~Window(void) = default;

  static Window *createWindow(int, int, const char *);
};
