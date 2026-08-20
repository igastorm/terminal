#pragma once
#include "IObject.hpp"

class IWindow : public IObject {
public:
  virtual bool setTitle(const char *) = 0;

  virtual bool show(void) = 0;

  virtual bool hide(void) = 0;

  virtual bool shouldClose(void) = 0;
  
  virtual ~IWindow(void) = default;
};
