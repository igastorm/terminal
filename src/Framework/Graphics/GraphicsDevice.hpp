#pragma once
#include "../Object/Object.hpp"
#include "IGraphicsDevice.hpp"

//  ========================================================
//
//  Graphics Device
//
//  ========================================================

class GraphicsDevice : public Object<IGraphicsDevice> {
protected:
  GraphicsDevice() = default;
  ~GraphicsDevice() = default;

public:
  int release() override;
};
