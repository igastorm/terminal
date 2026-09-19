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
  IFontAtlas *createFontAtlas(const char *, float) override;

  GraphicsDeviceTemplate(IApplication *appInstance)
      : CommonGraphicsDevice(appInstance) {}

public:
  ~GraphicsDeviceTemplate() = default;

  [[nodiscard]] PlatformData getPlatformData() const { return this->data; }
};
