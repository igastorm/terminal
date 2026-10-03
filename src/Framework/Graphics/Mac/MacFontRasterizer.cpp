#include "MacFontRasterizer.hpp"
#include <cstdio>
#include <cstdlib>
#include <new> // IWYU pragma: keep

MacFontRasterizer::MacFontRasterizer(CGContextRef cg_context) {
  this->data.cg_context = cg_context;
}

MacFontRasterizer::~MacFontRasterizer() {
  if (this->data.cg_context != nullptr) {
    CGContextRelease(this->data.cg_context);
    this->data.cg_context = nullptr;
  }
}

MacFontRasterizer *MacFontRasterizer::createMacFontRasterizer(
    IFont *font, std::uint8_t *bitmap_data, std::size_t bytes, std::size_t width,
    std::size_t height, std::size_t bytes_per_row) {
  if (font == nullptr || bitmap_data == nullptr || width == 0 || height == 0) {
    return nullptr;
  }

  // 1行分のバイト数が1行分のデータ量に満たないならエラー
  // bytes_per_row が大きい分には OK (Rect
  // に合わせて無駄な部分がカットされるだけなので)
  if (bytes_per_row < width * sizeof(std::uint8_t)) {
    return nullptr;
  }

  // 全体の容量が不足していたらエラー
  if (bytes < bytes_per_row * height) {
    return nullptr;
  }

  // 白黒フォーマットで作成
  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceGray();
  if (color_space == nullptr) {
    return nullptr;
  }

  CGContextRef cg_context = CGBitmapContextCreate(
      bitmap_data, width, height, 8 * sizeof(std::uint8_t),
      width * sizeof(std::uint8_t), color_space, kCGImageAlphaNone);
  CGColorSpaceRelease(color_space);
  if (cg_context == nullptr) {
    return nullptr;
  }

  // 初期の塗りつぶし色ではなくペン (バケツのインクの色 ) の色のようなもの
  // 塗りつぶすと黒になる
  CGContextSetGrayFillColor(cg_context, 0.0f, 1.0f);

  // 先ほど設定した黒で背景をクリア
  CGContextFillRect(cg_context, CGRectMake(0, 0, width, height));

  // 文字は白で描画すべきなのでバケツを白に切り替え
  CGContextSetGrayFillColor(cg_context, 1.0f, 1.0f);

  // アンチエイリアスを有効 (境界をなめらかにするらしい)
  CGContextSetShouldAntialias(cg_context, true);

  // 文字をなめらかにするらしい
  CGContextSetAllowsFontSmoothing(cg_context, true);
  CGContextSetShouldSmoothFonts(cg_context, true);

  MacFontRasterizer *font_rasterizer =
      static_cast<MacFontRasterizer *>(std::malloc(sizeof(MacFontRasterizer)));
  if (font_rasterizer == nullptr) {
    std::perror("malloc failed (createFont)");
    return nullptr;
  }

  font_rasterizer = new (font_rasterizer) MacFontRasterizer(cg_context);

  return font_rasterizer;
}

IFontRasterizer *IFontRasterizer::createFontRasterizer(IFont *, std::uint8_t *,
                                                       std::size_t, std::size_t,
                                                       std::size_t,
                                                       std::size_t) {
  return nullptr;
}
