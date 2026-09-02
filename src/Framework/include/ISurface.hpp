#pragma once
#include "IObject.hpp"
#include "IWindow.hpp"

class ISurface : public IObject {
public:
  virtual ~ISurface() = default;

  virtual bool bindToWindow(IWindow *) = 0;

  virtual void unbindWindow() = 0;
};
