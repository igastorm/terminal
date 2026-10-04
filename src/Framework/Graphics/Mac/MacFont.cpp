#include "MacFont.hpp"
#include "CharConverter.hpp"
#include "MacBitmap.hpp"
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

MacFont::MacFont(CTFontRef ct_font, float descent, float half_width,
                 float height)
    : Font(half_width, height) {
  this->data.ct_font = ct_font;
  this->data.descent = descent;
}

MacFont::~MacFont() {
  if (this->data.ct_font != nullptr) {
    CFRelease(this->data.ct_font);
    this->data.ct_font = nullptr;
  }
}

MacFont *MacFont::createMacFont(const char8_t *font_name, float size) {
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

  // 大文字の M のグリフを取得する
  // 等幅の場合, これに合わせるとちょうどいいらしい
  UniChar char_M = 'M';
  CGGlyph glyph_M = 0;
  if (!CTFontGetGlyphsForCharacters(ct_font, &char_M, &glyph_M, 1)) {
    CFRelease(ct_font);
    return nullptr;
  }

  // 次の文字に進むとどれだけ位置が進むかを取得
  // kCTFontOrientationHorizontal なので横方向
  // まとめると文字のセルに必要な横幅を取得している
  CGSize advance_M = {};
  CTFontGetAdvancesForGlyphs(ct_font, kCTFontOrientationHorizontal, &glyph_M,
                             &advance_M, 1);

  // ベースラインから上に必要な高さ
  CGFloat ascent = CTFontGetAscent(ct_font);

  // ベースラインから下に必要な高さ
  CGFloat descent = CTFontGetDescent(ct_font);

  // 推奨される行間の間隔
  CGFloat leading = CTFontGetLeading(ct_font);

  // 小数点以下を切り上げておく
  float half_width = std::ceill(advance_M.width);
  // ascent + descent + leading の中に実質 advance_M.height
  // が含まれているようなもの
  float height = std::ceill(ascent + descent + leading);

  MacFont *font = static_cast<MacFont *>(std::malloc(sizeof(MacFont)));
  if (font == nullptr) {
    std::perror("malloc failed (createFont)");
    return nullptr;
  }

  font = new (font) MacFont(ct_font, descent, half_width, height);

  return font;
}

bool MacFont::drawGlyph(IBitmap *ibitmap, const char32_t code_point, int x,
                        int y) {
  MacBitmap *bitmap = static_cast<MacBitmap *>(ibitmap);
  if (bitmap == nullptr) {
    return false;
  }

  CGContextRef cg_context = bitmap->getCGContext();
  if (cg_context == nullptr) {
    return false;
  }

  // サイズチェック
  std::size_t width = bitmap->getWidth();
  std::size_t height = bitmap->getHeight();
  FontCellSize cell_size = this->getCellSize();
  if (width < cell_size.half_width + x || height < cell_size.height + y) {
    return false;
  }

  CGGlyph glyph = 0;
  UniChar unichar_c[2] = {};
  std::size_t utf16_len = 0;
  CharConverter::Result result =
      CharConverter::cvtUTF32ToUTF16(code_point, unichar_c, &utf16_len);
  if (result != CharConverter::Result::Success) {
    return false;
  }

  if (!CTFontGetGlyphsForCharacters(this->data.ct_font, unichar_c, &glyph,
                                    utf16_len)) {
    return false;
  }

  // CoreGraphics は左下が原点なので変換が必要
  int cg_y = height - (y + cell_size.height);
  // CoreGraphics は左下原点だからベースラインの位置は descent を足せばいい
  CGPoint pos = CGPointMake(x, cg_y + this->data.descent);
  CTFontDrawGlyphs(this->data.ct_font, &glyph, &pos, 1, cg_context);

  return true;
}

IFont *IFont::createFont(const char8_t *font_name, float size) {
  return MacFont::createMacFont(font_name, size);
}
