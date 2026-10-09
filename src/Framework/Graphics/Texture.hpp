#pragma once
#include "../Object/Object.hpp"
#include "IGraphicsDevice.hpp"
#include "ITexture.hpp"

//  ========================================================
//
//  Texture
//
//  ========================================================

class Texture : public Object<ITexture> {
protected:
  IGraphicsDevice *device = nullptr;
  TextureFormat format = TextureFormat::Color;
  int width = 0;
  int height = 0;

  Texture(IGraphicsDevice *, int, int, TextureFormat);

public:
  int release() override;

  TextureFormat getFormat() override;

  int getWidth() override;
  int getHeight() override;

  ~Texture();
};
