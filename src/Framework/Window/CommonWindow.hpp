#pragma once
#include "IWindow.hpp"

class CommonWindow : public IWindow {
private:
  int ref_count = 0;

public:
  int addRef(void) override;
  int release(void) override;
  virtual ~CommonWindow(void) = default;
};
