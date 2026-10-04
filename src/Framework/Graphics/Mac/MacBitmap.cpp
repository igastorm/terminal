#include "MacBitmap.hpp"
#include <cstdio>
#include <cstdlib>
#include <new> // IWYU pragma: keep

MacBitmap::MacBitmap(CGContextRef cg_context, void *bitmap_data,
                     std::size_t width, std::size_t height)
    : CommonBitmap(width, height, bitmap_data) {
  this->data.cg_context = cg_context;
}

MacBitmap::~MacBitmap() {
  if (this->data.cg_context != nullptr) {
    CGContextRelease(this->data.cg_context);
    this->data.cg_context = nullptr;
  }
}

MacBitmap *MacBitmap::createMacBitmap(std::size_t bytes, std::size_t width,
                                      std::size_t height) {
  if (width == 0 || height == 0) {
    return nullptr;
  }

  if (bytes < width * height * sizeof(std::uint8_t)) {
    return nullptr;
  }

  void *bitmap_data = std::calloc(width * height, sizeof(std::uint8_t));
  if (bitmap_data == nullptr) {
    return nullptr;
  }

  // 白黒フォーマットで作成
  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceGray();
  if (color_space == nullptr) {
    std::free(bitmap_data);
    return nullptr;
  }

  CGContextRef cg_context = CGBitmapContextCreate(
      bitmap_data, width, height, 8 * sizeof(std::uint8_t),
      width * sizeof(std::uint8_t), color_space, kCGImageAlphaNone);
  CGColorSpaceRelease(color_space);
  if (cg_context == nullptr) {
    std::free(bitmap_data);
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

  MacBitmap *bitmap = static_cast<MacBitmap *>(std::malloc(sizeof(MacBitmap)));
  if (bitmap == nullptr) {
    std::perror("malloc failed (createFont)");
    std::free(bitmap_data);
    return nullptr;
  }

  bitmap = new (bitmap) MacBitmap(cg_context, bitmap_data, width, height);

  return bitmap;
}

CGContextRef MacBitmap::getCGContext() const { return this->data.cg_context; }

IBitmap *IBitmap::createBitmap(std::size_t, std::size_t, std::size_t) {
  return nullptr;
}
