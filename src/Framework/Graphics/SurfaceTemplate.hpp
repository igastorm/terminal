#pragma once
#include "CommonGraphics.hpp"

//  ========================================================
//
//  Surface
//
//  ========================================================

template <class PlatformData> class SurfaceTemplate : public CommonSurface {
protected:
  PlatformData data;

  bool render(RenderCallBack, void *, const RenderPassDesc) override;

  SurfaceTemplate(IGraphicsDevice *device, IObject *window_or_texture)
      : CommonSurface(device, window_or_texture) {}

public:
  [[nodiscard]] PlatformData getPlatformData() const;
  SurfaceTemplate() = delete;
  ~SurfaceTemplate() override;
};