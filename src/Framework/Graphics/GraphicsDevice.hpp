#pragma once
#include "IGraphicsDevice.hpp"

//  ========================================================
//
//  Graphics Device
//
//  ========================================================

class GraphicsDevice : public IGraphicsDevice {
private:
  int ref_count = 0;

public:
  int addRef() override;
  int release() override;
  GraphicsDevice();
  ~GraphicsDevice();
};
