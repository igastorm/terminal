#pragma once
#include "IGraphicsDevice.hpp"
#include "ITexture.hpp"

//  ========================================================
//
//  Texture
//
//  ========================================================

class Texture : public ITexture {
private:
  int ref_count = 0;

protected:
  IGraphicsDevice *device = nullptr;
  TextureFormat format = TextureFormat::Color;
  int width = 0;
  int height = 0;

  Texture(IGraphicsDevice *, int, int, TextureFormat);

public:
  int addRef() override;
  int release() override;

  TextureFormat getFormat() override;

  int getWidth() override;
  int getHeight() override;

  ~Texture();
};
