#pragma once
#include "IObject.hpp"

class IWindow : public IObject {
public:
  virtual bool setTitle(const char *) = 0;

  virtual bool show() = 0;

  virtual bool hide() = 0;
  
  virtual ~IWindow() = default;
};
