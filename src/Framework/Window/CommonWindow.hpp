#pragma once
#include "IWindow.hpp"

class CommonWindow : public IWindow {
private:
  int ref_count = 0;

public:
  int addRef() override;
  int release() override;
  virtual ~CommonWindow() = default;
};
