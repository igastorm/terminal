#pragma once
#include "CommonGraphics.hpp"

template <class PlatformData>
class FontAtlasTemplate : public CommonFontAtlas {
  protected:
    PlatformData data;

    FontAtlasTemplate(IGraphicsDevice *device)
        : CommonFontAtlas(device) {};
  public:
    FontAtlasTemplate() = delete;
    ~FontAtlasTemplate() = default;
};