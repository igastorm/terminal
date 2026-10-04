#pragma once
#include "../FontAtlas.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

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

struct FontAtlasData {
  CTFontRef font = nullptr;
  CGContextRef ctx = nullptr;
  int cols_per_row = 0;
};

class MacFontAtlas : public FontAtlas {
private:
  FontAtlasData data = {};
  bool drawText(IRenderPass *, const char *, float, float,
                std::uint32_t) override;
  GlyphUV getGlyphUV(char32_t) override;

  bool preloadGlyphs32(const char32_t *) override;

  bool updateGlyphCache(const char32_t) override;

  MacFontAtlas(IGraphicsDevice *);
  friend class MacFontAtlasHelper;

public:
  MacFontAtlas() = delete;
  ~MacFontAtlas();
  static MacFontAtlas *createMacFontAtlas(IGraphicsDevice *, const char *,
                                          float, int, int);
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
  // MacFontAtlasHelper() = delete;
};
