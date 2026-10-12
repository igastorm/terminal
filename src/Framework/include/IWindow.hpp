#pragma once
#include "IObject.hpp"

class IWindow : public IObject {
public:
  virtual bool setTitle(const char8_t *) = 0;

  virtual bool show() = 0;

  virtual bool hide() = 0;

  virtual bool setResizeIncrements(int, int) = 0;

  virtual bool setMinSize(int, int) = 0;

  virtual void setUserData(void *) = 0;

  virtual void *getUserData() const = 0;

  ~IWindow() = default;

  [[nodiscard]] static IWindow *createWindow(int, int, const char8_t *);
};
