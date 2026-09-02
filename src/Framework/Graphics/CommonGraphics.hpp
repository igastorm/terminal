#pragma once
#include "IGraphicsDevice.hpp"

class CommonGraphics : public IGraphicsDevice {
private:
  int ref_count = 0;

public:
  int addRef() override;
  int release() override;
  virtual ~CommonGraphics() = default;
};
