#pragma once
#include "CommonGraphics.hpp"

template <class PlatformData> class FontAtlasTemplate : public CommonFontAtlas {
protected:
  PlatformData data;

  FontAtlasTemplate(IGraphicsDevice *device) : CommonFontAtlas(device) {};

  bool drawText(IRenderPass *, const char *, float, float,
                std::uint32_t) override;
  GlyphUV getGlyphUV(wchar_t) override;

  bool preloadGlyphs32(const wchar_t*) override;

  bool updateGlyphCache(const wchar_t) override;

public:
  FontAtlasTemplate() = delete;
  ~FontAtlasTemplate() = default;
};
