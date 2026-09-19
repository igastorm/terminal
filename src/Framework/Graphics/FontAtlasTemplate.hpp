#pragma once
#include "CommonGraphics.hpp"

template <class PlatformData> class FontAtlasTemplate : public CommonFontAtlas {
protected:
  PlatformData data;

  FontAtlasTemplate(IGraphicsDevice *device) : CommonFontAtlas(device) {};

  bool drawText(IRenderPass *, const char *, float, float,
                std::uint32_t) override;

public:
  FontAtlasTemplate() = delete;
  ~FontAtlasTemplate() = default;
};
