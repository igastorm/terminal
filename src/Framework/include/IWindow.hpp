#pragma once
#include "IObject.hpp"

class IWindow : public IObject {
public:
  virtual bool setTitle(const char *) = 0;

  virtual bool show() = 0;

  virtual bool hide() = 0;

  virtual bool setResizeIncrements(int, int) = 0;

  virtual bool setMinSize(int, int) = 0;

  ~IWindow() = default;

  [[nodiscard]] static IWindow *createWindow(int, int, const char *);
};
