#pragma once
#include "IFontAtlas.hpp"
#include "IGraphicsDevice.hpp"

//  ========================================================
//
//  Font Atals
//
//  ========================================================

struct HashEntry {
  static constexpr size_t HASH_SIZE = 1024;
  std::uint32_t codepoint = 0;
  GlyphUV glyph_table = {};
};

class FontAtlas : public IFontAtlas {
private:
  int ref_count = 0;

protected:
  std::uint8_t *on_demand_bitmap_data = nullptr;
  GlyphUV glyph_table[95] = {};
  HashEntry glyph_hash_table[HashEntry::HASH_SIZE] = {};
  float cell_width = 0.0f;
  float cell_height = 0.0f;
  int cols_per_row = 0;

  int atlas_width = 0;
  int atlas_height = 0;

  IGraphicsDevice *device = nullptr;
  ITexture *texture = nullptr;

  float cursor_x = 0.0f;
  float cursor_y = 0.0f;

  bool rewindCursor();
  size_t hashCodepoint(std::uint32_t) const;
  const HashEntry *findEntry(uint32_t code_point) const;

  FontAtlas(IGraphicsDevice *);

public:
  ITexture *getTexture() override;
  int addRef() override;
  int release() override;
  ~FontAtlas();
};
