#pragma once
#include "IApplication.hpp"
#include "IFontAtlas.hpp"
#include "IGraphicsDevice.hpp"
#include "ISurface.hpp"
#include "ITexture.hpp"

//  ========================================================
//
//  Render Pass
//
//  ========================================================

class CommonRenderPass : public IRenderPass {
private:
  int ref_count = 0;

public:
  [[deprecated(
      "Should be used as a temporary object on the stack within `render()`")]]
  int addRef() override;

  [[deprecated(
      "Should be used as a temporary object on the stack within `render()`")]]
  int release() override;

  ~CommonRenderPass() = default;
};

//  ========================================================
//
//  Font Atals
//
//  ========================================================

class CommonFontAtlas : public IFontAtlas {
private:
  int ref_count = 0;

protected:
  IGraphicsDevice *device;

  CommonFontAtlas(IGraphicsDevice *);

public:
  int addRef() override;
  int release() override;
  ~CommonFontAtlas();
};

//  ========================================================
//
//  Texture
//
//  ========================================================

class CommonTexture : public ITexture {
private:
  int ref_count = 0;

protected:
  IGraphicsDevice *device = nullptr;
  TextureFormat format = TextureFormat::Color;
  int width = 0;
  int height = 0;

  CommonTexture(IGraphicsDevice *, int, int, TextureFormat);

public:
  int addRef() override;
  int release() override;

  TextureFormat getFormat() override;

  int getWidth() override;
  int getHeight() override;

  ~CommonTexture();
};

//  ========================================================
//
//  Surface
//
//  ========================================================

class CommonSurface : public ISurface {
private:
  int ref_count = 0;

protected:
  IGraphicsDevice *device = nullptr;
  union {
    IObject *window_or_texture = nullptr;
    // Window 用と Texture 用で実装が分かれるので共用体で OK
    IWindow *window;
    ITexture *texture;
  };

  CommonSurface(IGraphicsDevice *, IObject *);

public:
  int addRef() override;
  int release() override;
  ~CommonSurface();
};

//  ========================================================
//
//  Graphics Device
//
//  ========================================================

class CommonGraphicsDevice : public IGraphicsDevice {
private:
  int ref_count = 0;

protected:
  IApplication *appInstance = nullptr;

public:
  int addRef() override;
  int release() override;
  CommonGraphicsDevice(IApplication *);
  ~CommonGraphicsDevice();
};
