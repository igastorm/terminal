#pragma once
#include "../Surface.hpp"
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

//  ========================================================
//
//  Surface
//
//  ========================================================

class MacSurface : public Surface {
protected:
  dispatch_semaphore_t in_flight_semaphore = nil;

  MacSurface(IGraphicsDevice *device, IObject *window_or_texture)
      : Surface(device, window_or_texture) {}

public:
  MacSurface() = delete;
  ~MacSurface();
};

// ヘルパークラス
class RenderHelper {
private:
  MTLRenderPassDescriptor *mtl_pass_desc = nil;
  IGraphicsDevice *device = nullptr;
  bool is_ready = false;

public:
  static constexpr float inv_255 = 1.0f / 255.0f;
  MTLRenderPassDescriptor *getMTLRenderPassDescripter(id<MTLTexture>,
                                                      const RenderPassDesc *);
  [[nodiscard]] bool renderBase(id<MTLRenderCommandEncoder>, float, float,
                                RenderCallBack callback, void *data);
  bool isReady() const;
  RenderHelper(IGraphicsDevice *);
  ~RenderHelper();
};
