#pragma once
#include "../Object/Object.hpp"
#include "IGraphicsDevice.hpp"

//  ========================================================
//
//  Graphics Device
//
//  ========================================================

class GraphicsDevice : public Object<IGraphicsDevice> {
public:
  int release() override;
  GraphicsDevice();
  ~GraphicsDevice();
};
