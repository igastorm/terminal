#include "MacFont.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <cstdlib>

MacFont::~MacFont() {
  if (bitmap_data != nullptr) {
    free(bitmap_data);
    bitmap_data = nullptr;
  }
  if (ctx != nullptr) {
    CGContextRelease(ctx);
    ctx = nullptr;
  }
  if (font != nullptr) {
    CFRelease(font);
    font = nullptr;
  }
}

ITexture *MacFont::createFontTextureBase(IGraphicsDevice *device, char chracter,
                                         int size) {
  if (device == nullptr || size < 0) {
    return nullptr;
  }

  size_t bytes_per_row = size * sizeof(std::uint8_t);
  size_t total_bytes = bytes_per_row * size;
  bitmap_data = static_cast<std::uint8_t *>(
      std::calloc(total_bytes, sizeof(std::uint8_t)));
  if (bitmap_data == nullptr) {
    return nullptr;
  }

  // 白黒フォーマットで作成
  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceGray();

  if (color_space == nullptr) {
    return nullptr;
  }

  ctx = CGBitmapContextCreate(bitmap_data, size, size, 8 * sizeof(std::uint8_t),
                              bytes_per_row, color_space, kCGImageAlphaNone);
  CGColorSpaceRelease(color_space);
  if (ctx == nullptr) {
    return nullptr;
  }

  // 初期の塗りつぶし色ではなくペン (バケツのインクの色 ) の色のようなもの
  // 塗りつぶすと黒になる
  CGContextSetGrayFillColor(ctx, 0.0f, 1.0f);

  // 先ほど設定した黒で背景をクリア
  CGContextFillRect(ctx, CGRectMake(0, 0, size, size));

  // 文字は白で描画すべきなのでバケツを白に切り替え
  CGContextSetGrayFillColor(ctx, 1.0f, 1.0f);

  // アンチエイリアスを有効 (境界をなめらかにするらしい)
  CGContextSetShouldAntialias(ctx, true);

  // フォントをなめらかにするらしい (アンチエイリアスとの違いがよくわからんが)
  CGContextSetAllowsFontSmoothing(ctx, true);
  CGContextSetShouldSmoothFonts(ctx, true);

  // なんか自分で解放してはダメらしい
  CFStringRef font_name = CFSTR("Menlo");

  // テクスチャのサイズと同じだと大きすぎるので 80% くらいにする
  font = CTFontCreateWithName(font_name, size * 0.8f, nullptr);
  if (font == nullptr) {
    return nullptr;
  }

  // UTF-16 にしてやる必要がある
  // ASCII の範囲ならスタティックキャストすればいいのか
  UniChar unichar_c = static_cast<UniChar>(chracter);

  // 文字コードとは別のフォント内の番号らしい
  CGGlyph glyph = 0;

  // 文字コードからグリフ番号を得る (複数の文字もできるらしい)
  if (!CTFontGetGlyphsForCharacters(font, &unichar_c, &glyph, 1)) {
    return nullptr;
  }

  // 中心からどれくらい文字が下に伸びるかの値を取得するらしい
  CGFloat descent = CTFontGetDescent(font);

  // 左下が原点
  // 文字を書く位置を指定する
  // 端っこではなくなるべく中心になるように
  CGPoint text_position = CGPointMake(size * 0.1f, descent + (size * 0.1f));

  // 文字を描画する (複数の文字を一括でやることもできる)
  CTFontDrawGlyphs(font, &glyph, &text_position, 1, ctx);

  // テクスチャへのコピー
  device->addRef();
  TextureDesc desc;
  desc.drawable_flag = TextureDrawable::Disable;
  desc.format = TextureFormat::Mono;

  ITexture *texture = device->createTexture(size, size, desc);
  if (texture == nullptr) {
    device->release();
    return nullptr;
  }

  texture->upload(bitmap_data, total_bytes, bytes_per_row);

  device->release();
  return texture;
}
