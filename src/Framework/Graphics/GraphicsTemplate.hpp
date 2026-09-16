#pragma once
#include "CommonGraphics.hpp"

//  ========================================================
//
//  Render Pass
//
//  ========================================================

template <class PlatformData>
class RenderPassTemplate : public CommonRenderPass {
protected:
  PlatformData data;

  bool drawVertices(const Vertex *vertices, int vertex_count) override;
  bool drawVerticesTex(ITexture *, const VertexTex *vertices,
                       int vertex_count) override;

public:
  ~RenderPassTemplate() override = default;
};

//  ========================================================
//
//  Texture
//
//  ========================================================

template <class PlatformData> class TextureTemplate : public CommonTexture {
protected:
  PlatformData data;

  TextureTemplate(IGraphicsDevice *device, int w, int h, TextureFormat format)
      : CommonTexture(device, w, h, format) {};

public:
  TextureTemplate() = delete;
  ~TextureTemplate() override;
  [[nodiscard]] PlatformData getPlatformData() const;
  bool upload(const void *, size_t, size_t) override;
};

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
