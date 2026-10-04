#pragma once
#include "IApplication.hpp"
#include "IGraphicsDevice.hpp"

//  ========================================================
//
//  Graphics Device
//
//  ========================================================

class GraphicsDevice : public IGraphicsDevice {
private:
  int ref_count = 0;

protected:
  IApplication *appInstance = nullptr;

public:
  int addRef() override;
  int release() override;
  GraphicsDevice(IApplication *);
  ~GraphicsDevice();
};
