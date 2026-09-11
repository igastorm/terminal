#pragma once
#include "../Application/ImpApplication.hpp"
#include "CommonGraphics.hpp"

//  ========================================================
//
//  Render Pass
//
//  ========================================================

template <class PlatformData>
class ImpRenderPassTemplate : public CommonRenderPass {
protected:
  PlatformData data;

  bool drawVertices(const Vertex *vertices, int vertex_count) override;

public:
  ~ImpRenderPassTemplate() override = default;
};

//  ========================================================
//
//  Texture
//
//  ========================================================

template <class PlatformData> class ImpTextureTemplate : public CommonTexture {
protected:
  PlatformData data;

public:
  ~ImpTextureTemplate() override;
  [[nodiscard]] PlatformData getPlatformData() const;
};

//  ========================================================
//
//  Surface
//
//  ========================================================

template <class PlatformData> class ImpSurfaceTemplate : public CommonSurface {
protected:
  PlatformData data;
  
  bool render(RenderCallBack, void *, const RenderPassDesc) override;
public:
  [[nodiscard]] PlatformData getPlatformData() const;
  ~ImpSurfaceTemplate();
};

//  ========================================================
//
//  Graphics Device
//
//  ========================================================

template <class PlatformData, class PlatformApplicationData>
class ImpGraphicsDeviceTemplate : public CommonGraphicsDevice {
protected:
  PlatformData data;
  ImpApplication<PlatformApplicationData> *appInstance;

  ISurface *createSurfaceFromWindow(IWindow *) override;
  ISurface *createSurfaceFromTexture(ITexture *) override;
  ITexture *createTexture(int, int, TextureDrawable) override;

public:
  ~ImpGraphicsDeviceTemplate() override;

  [[nodiscard]] PlatformData getPlatformData() const;
};
