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

class MacFontAtlasHelper;
struct CellSize {
private:
  CGFloat descent = 0.0;
  friend class MacFontAtlasHelper;

public:
  float cell_width = 0.0f;
  float cell_height = 0.0f;
};

class MacFontAtlasHelper {
public:
  [[nodiscard]] static CTFontRef createCTFont(const char *, float);
  [[nodiscard]] static CGContextRef createBitmapContext(std::uint8_t *, size_t,
                                                        int, int);
  [[nodiscard]] static CellSize getCellSize(CTFontRef);
  [[nodiscard]] static bool drawBitmap(CGContextRef, CTFontRef, CellSize,
                                       const UniChar *, size_t, int, int, int,
                                       int);
  MacFontAtlasHelper(const char *, float, int, int);
  ~MacFontAtlasHelper();
};

struct FontAtlasData {
  CTFontRef font = nullptr;
  CGContextRef ctx = nullptr;
  int cols_per_row = 0;
};

using FontAtlas = FontAtlasTemplate<FontAtlasData>;

class MacFontAtlas : public FontAtlas {
private:
  MacFontAtlas(IGraphicsDevice *);

public:
  MacFontAtlas() = delete;
  ~MacFontAtlas();
  static MacFontAtlas *createMacFontAtlas(IGraphicsDevice *, const char *,
                                          float);
  [[nodiscard]] GlyphUV getOrCreateGlyphUV(uint32_t code_point,
                                           UniChar unichar_c[2],
                                           size_t utf16_len, int cols);
};
