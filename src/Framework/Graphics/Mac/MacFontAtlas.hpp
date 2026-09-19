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

class MacFontAtlasHelper {
private:
  CFStringRef cf_font_name = nullptr;
  CTFontRef font = nullptr;
  CGContextRef ctx = nullptr;
  float descent = 0.0f;
  float cell_width = 0.0f;
  float cell_height = 0.0f;
  bool is_ready = false;
  int atlas_width = 0;
  int atlas_height = 0;
  int cols_per_row = 0;
  std::uint8_t* bitmap = nullptr;

  bool getCellSize();

public:
  bool isReady() const { return this->is_ready; }
  float getCellWidth() const {return this->cell_width;}
  float getCellHeight() const {return this->cell_height;}
  const CTFontRef getFont() const { return this->font; }
  int getColsPerRow() const {return this->cols_per_row;}
  const float getDescent() const { return this->descent; }
  bool initCTX();
  bool drawBitmap(char, GlyphUV*, int ,int);
  MacFontAtlasHelper(const char *, size_t, float, int,int);
  ~MacFontAtlasHelper();
};

struct FontAtlasData {};

using FontAtlas = FontAtlasTemplate<FontAtlasData>;

class MacFontAtlas : public FontAtlas {
private:
  MacFontAtlas(IGraphicsDevice *);

public:
  MacFontAtlas() = delete;
  ~MacFontAtlas();
  static MacFontAtlas *createMacFontAtlas(IGraphicsDevice *, const char *,
                                          size_t, float);
};
