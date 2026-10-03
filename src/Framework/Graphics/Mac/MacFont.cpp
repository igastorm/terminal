#include "MacFont.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

CTFontRef MacFont::createCTFont(const char8_t *font_name, float size) {
  CFStringRef cf_font_name = CFStringCreateWithCString(
      kCFAllocatorDefault, reinterpret_cast<const char *>(font_name),
      kCFStringEncodingUTF8);

  if (cf_font_name == nullptr) {
    return nullptr;
  }

  // 第三引数は斜体とかを作りたい時に使うらしい
  CTFontRef font = CTFontCreateWithName(cf_font_name, size, nullptr);
  CFRelease(cf_font_name);

  return font;
}

MacFont::MacFont(CTFontRef ct_font, float size) {
  this->data.ct_font = ct_font;
  this->data.size = size;
}

MacFont::~MacFont() {
  if (this->data.ct_font != nullptr) {
    CFRelease(this->data.ct_font);
    this->data.ct_font = nullptr;
  }
}

MacFont *MacFont::createFont(const char8_t *font_name, float size) {
  if (font_name == nullptr || size == 0.0f) {
    return nullptr;
  }

  if (std::char_traits<char8_t>::length(font_name) == 0) {
    return nullptr;
  }

  CTFontRef ct_font = createCTFont(font_name, size);
  if (ct_font == nullptr) {
    return nullptr;
  }

  MacFont *font = static_cast<MacFont *>(std::malloc(sizeof(MacFont)));
  if (font == nullptr) {
    std::perror("malloc failed (createFont)");
    return nullptr;
  }

  font = new (font) MacFont(ct_font, size);

  return font;
}

MacFontData MacFont::getData() const { return this->data; }

FontCellSize MacFont::getCellSize() const {
  if (this->data.ct_font == nullptr) {
    return {};
  }

  // 大文字の M のグリフを取得する
  // 等幅の場合, これに合わせるとちょうどいいらしい
  UniChar char_M = 'M';
  CGGlyph glyph_M = 0;
  CTFontGetGlyphsForCharacters(this->data.ct_font, &char_M, &glyph_M, 1);

  // 次の文字に進むとどれだけ位置が進むかを取得
  // kCTFontOrientationHorizontal なので横方向
  // まとめると文字のセルに必要な横幅を取得している
  CGSize advance_M = {};
  CTFontGetAdvancesForGlyphs(this->data.ct_font, kCTFontOrientationHorizontal,
                             &glyph_M, &advance_M, 1);

  FontCellSize cell_size = {};

  // ベースラインから上に必要な高さ
  cell_size.ascent = CTFontGetAscent(this->data.ct_font);

  // ベースラインから下に必要な高さ
  cell_size.descent = CTFontGetDescent(this->data.ct_font);

  // 推奨される行間の間隔
  cell_size.leading = CTFontGetLeading(this->data.ct_font);

  // 小数点以下を切り上げておく
  cell_size.width = std::ceill(advance_M.width);
  // ascent + descent + leading の中に実質 advance_M.height
  // が含まれているようなもの
  cell_size.height =
      std::ceill(cell_size.ascent + cell_size.descent + cell_size.leading);

  return cell_size;
}
