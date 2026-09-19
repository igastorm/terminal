#pragma once
#include "../FontAtlasTemplate.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

class MacFont {
private:
  std::uint8_t *bitmap_data = nullptr;
  CGContextRef ctx = nullptr;
  CTFontRef font = nullptr;

public:
  ITexture *createFontTextureBase(IGraphicsDevice *, const char *, int);
  MacFont() = default;
  ~MacFont();
};

//  ========================================================
//
//  FontAtlas
//
//  ========================================================

struct GlyphUV {
  float u_min = 0.0f, v_min = 0.0f;
  float u_max = 0.0f, v_max = 0.0f;
};

struct HashEntry {
  uint32_t codepoint = 0;
  GlyphUV glyph_uv = {};
};

struct FontAtlasData {
  float cell_width = 0.0f;
  float cell_height = 0.0f;

  GlyphUV glyph_table[95] = {};
  ITexture* texture = nullptr;
};

using FontAtlas = FontAtlasTemplate<FontAtlasData>;

class MacFontAtlas : public FontAtlas {
private:
  MacFontAtlas(IGraphicsDevice *);

public:
  MacFontAtlas() = delete;
  ~MacFontAtlas();
  static MacFontAtlas *createMacFontAtlas(IGraphicsDevice *, const char *, size_t,
                                          float);
};
