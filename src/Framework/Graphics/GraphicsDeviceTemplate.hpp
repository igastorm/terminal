#pragma once
#include "CommonGraphics.hpp"

//  ========================================================
//
//  Graphics Device
//
//  ========================================================

template <class PlatformData, class PlatformApplicationData>
class GraphicsDeviceTemplate : public CommonGraphicsDevice {
protected:
  PlatformData data;

  ISurface *createSurfaceFromWindow(IWindow *) override;
  ISurface *createSurfaceFromTexture(ITexture *) override;
  ITexture *createTexture(int, int, const TextureDesc) override;
  ITexture *createFontTexture(const char *, int) override;

  GraphicsDeviceTemplate(IApplication *appInstance)
      : CommonGraphicsDevice(appInstance) {}

public:
  ~GraphicsDeviceTemplate() override;

  [[nodiscard]] PlatformData getPlatformData() const;
};