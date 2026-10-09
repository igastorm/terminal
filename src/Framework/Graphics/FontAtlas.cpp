#include "FontAtlas.hpp"
#include <cstdlib>

//  ========================================================
//
//  Font Atlas
//
//  ========================================================

ITexture *FontAtlas::getTexture() { return this->texture; }

int FontAtlas::release() {
  int ref_count = this->Object<IFontAtlas>::release();
  if (ref_count == 0) {
    this->~FontAtlas();
    return 0;
  }
  return ref_count;
}

// コードポイントをハッシュ化
size_t FontAtlas::hashCodepoint(std::uint32_t cp) const {
  // このようなハッシュ関数にするとなんか重複が少なくなるらしい
  // 原理はいまいちわからん
  return (cp * 2654435761u) & (HashEntry::HASH_SIZE - 1);
}

bool FontAtlas::rewindCursor() {
  int cols_per_row = this->cols_per_row;
  if (cols_per_row == 0) {
    return false;
  }

  this->cursor_x = 0.0f;

  // 整数で切り上げを行う公式らしい
  int ascii_rows = (95 + cols_per_row - 1) / cols_per_row;
  this->cursor_y = ascii_rows * cell_height;
  return true;
}

// ハッシュテーブルに既に存在するか探すだけ
const HashEntry *FontAtlas::findEntry(uint32_t code_point) const {
  const std::size_t start_idx = this->hashCodepoint(code_point);
  std::size_t idx = start_idx;

  while (this->glyph_hash_table[idx].codepoint != 0) {
    if (this->glyph_hash_table[idx].codepoint == code_point) {
      // 見つかった
      return &this->glyph_hash_table[idx];
    }
    idx = (idx + 1) & (HashEntry::HASH_SIZE - 1);
    if (idx == start_idx) {
      // 一周した（満タンかつ見つからなかった）
      break;
    }
  }

  // キャッシュに存在しない
  return nullptr;
}

FontAtlas::FontAtlas(IGraphicsDevice *device) {
  this->addRef();
  if (device != nullptr) {
    this->device = device;
    this->device->addRef();
  }
}

FontAtlas::~FontAtlas() {
  if (this->texture != nullptr) {
    this->texture->release();
    this->texture = nullptr;
  }
  if (this->device != nullptr) {
    this->device->release();
    this->device = nullptr;
  }
  if (this->on_demand_bitmap_data != nullptr) {
    std::free(this->on_demand_bitmap_data);
    this->on_demand_bitmap_data = nullptr;
  }
}
