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
  bool drawVerticesTex(ITexture *, const VertexTex *vertices,
                       int vertex_count) override;

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
  bool upload(const void *, size_t, size_t) override;
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
  ImpApplicationTemplate<PlatformApplicationData> *appInstance;

  ISurface *createSurfaceFromWindow(IWindow *) override;
  ISurface *createSurfaceFromTexture(ITexture *) override;
  ITexture *createTexture(int, int, const TextureDesc) override;
  ITexture *createFontTexture(char, int) override;

public:
  ~ImpGraphicsDeviceTemplate() override;

  [[nodiscard]] PlatformData getPlatformData() const;
};
