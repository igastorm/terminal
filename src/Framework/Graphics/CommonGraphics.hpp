#pragma once
#include "IGraphicsDevice.hpp"
#include "ISurface.hpp"
#include "ITexture.hpp"

enum class BindObject { none, window, texture };

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

  virtual ~CommonRenderPass() = default;
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

public:
  int addRef() override;
  int release() override;

  TextureFormat getFormat() override;

  int getWidth() override;
  int getHeight() override;
  
  virtual ~CommonTexture() = default;
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
    // Window 用と Texture 用で実装が分かれるので共用体で OK
    IWindow *window = nullptr;
    ITexture *texture;
  };

public:
  int addRef() override;
  int release() override;
  virtual ~CommonSurface() = default;
};

//  ========================================================
//
//  Graphics Device
//
//  ========================================================

class CommonGraphicsDevice : public IGraphicsDevice {
private:
  int ref_count = 0;

public:
  int addRef() override;
  int release() override;
  virtual ~CommonGraphicsDevice() = default;
};
