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

struct FontAtlasData {};

using FontAtlas = FontAtlasTemplate<FontAtlasData>;

class MacFontAtlas : public FontAtlas {
private:
  MacFontAtlas(IGraphicsDevice *);

public:
  MacFontAtlas() = delete;
  static MacFontAtlas *createMacFontAtlas(IGraphicsDevice *, const char *, int,
                                          int);
};
