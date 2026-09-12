#include "MacFont.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <cstdlib>
#include <cstring>

// 以下をもとに実装
// https://ja.wikipedia.org/wiki/UTF-8
// https://ja.wikipedia.org/wiki/UTF-16
// dst_cap は文字数単位
size_t cvtUTF8ToUTF16BMP(const uint8_t *src, size_t src_len, uint16_t *dst,
                         size_t dst_cap) {
  // 継続バイトかの判定
  // 文字の先頭ではなく, 前のバイトの続きであることを示す値
  // 2バイト目以降の下限から上限の範囲内か
  auto isContinuationByte = [](uint8_t b) -> bool {
    // 10000000 ~ 10111111
    return (0x80 <= b && b <= 0xBF);
  };

  if (src == nullptr || src_len == 0 || dst == nullptr || dst_cap == 0) {
    return 0;
  }

  size_t i = 0;
  size_t out = 0;

  while (i < src_len) {
    // 各文字の先頭バイト
    uint8_t b0 = src[i];
    uint32_t code_point = 0;

    // ASCII はそのまま
    // 0 ~ 01111111
    if (b0 <= 0x7F) {
      code_point = src[i];
      i++;
    } else if (0xC2 <= b0 && b0 <= 0xDF) {
      // 2バイトの UTF-8 (1バイト目はすでに b0 に入ってる)
      // 11000010 ~ 11011111
      uint8_t b1 = 0;
      if (i + 1 >= src_len) {
        // 続きのデータがないならエラー
        return 0;
      }

      b1 = src[i + 1];
      if (!isContinuationByte(b1)) {
        // 2バイト目なのに前のバイトの続きじゃなかったらおかしい
        // 2バイト目に入るべき値の範囲外
        return 0;
      }
      // 識別ビットを削除して繋げる
      code_point = static_cast<uint32_t>(b0 & 0x1F) << 6 |
                   static_cast<uint32_t>(b1 & 0x3F);
      i += 2;
    } else if (0xE0 <= b0 && b0 <= 0xEF) {
      // 3バイトの UTF-8 (1バイト目はすでに b0 に入ってる)
      // 11100000 ~ 11101111
      uint8_t b1 = 0, b2 = 0;
      if (i + 2 >= src_len) {
        // 続きのデータがないならエラー
        return 0;
      }

      b1 = src[i + 1];
      b2 = src[i + 2];
      if (!isContinuationByte(b1) || !isContinuationByte(b2)) {
        // 2バイト目以降なのに前のバイトの続きじゃなかったらおかしい
        // 2バイト目以降に入るべき値の範囲外
        return 0;
      }

      // 0xE0 0x80 ~ 0x9F は本来1バイトの文字を3バイトで表してるから不正らしい
      if (b0 == 0xE0 && 0x80 <= b1 && b1 <= 0x9F) {
        return 0;
      }

      // UTF-16 で16ビットをはみ出す (サロゲートというらしい) 部分を拒否
      // 0xED 0xA0 ~
      if (b0 == 0xED && 0xA0 <= b1) {
        return 0;
      }

      // 識別ビットを削除して繋げる
      code_point = static_cast<uint32_t>(b0 & 0x0F) << 12 |
                   static_cast<uint32_t>(b1 & 0x3F) << 6 |
                   static_cast<uint32_t>(b2 & 0x3F);
      i += 3;
    } else if (b0 >= 0xF0 && b0 <= 0xF4) {
      // 3バイトの UTF-8 (1バイト目はすでに b0 に入ってる)
      // 11110000 ~ 11110100
      // Wiki によると有効バイトが 21bit なので明らかにサロゲート領域
      return 0;
    } else {
      // その他は不正
      return 0;
    }
    // 最終チェック
    // この範囲はサロゲートらしい
    if (0x10000 <= code_point && code_point <= 0x10FFFF) {
      return 0;
    }
    if (out >= dst_cap) {
      return 0;
    }
    // BMP の範囲内はコードポイントがそのまま入るらしい
    dst[out] = static_cast<uint16_t>(code_point);
    out++;
  }
  return out;
}

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

ITexture *MacFont::createFontTextureBase(IGraphicsDevice *device,
                                         const char *chracter, int size) {
  if (device == nullptr || size < 0) {
    return nullptr;
  }

  float width = size;
  float height = size;

  size_t bytes_per_row = width * sizeof(std::uint8_t);
  size_t total_bytes = bytes_per_row * height;
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

  ctx = CGBitmapContextCreate(bitmap_data, width, height,
                              8 * sizeof(std::uint8_t), bytes_per_row,
                              color_space, kCGImageAlphaNone);
  CGColorSpaceRelease(color_space);
  if (ctx == nullptr) {
    return nullptr;
  }

  // 初期の塗りつぶし色ではなくペン (バケツのインクの色 ) の色のようなもの
  // 塗りつぶすと黒になる
  CGContextSetGrayFillColor(ctx, 0.0f, 1.0f);

  // 先ほど設定した黒で背景をクリア
  CGContextFillRect(ctx, CGRectMake(0, 0, width, height));

  // 文字は白で描画すべきなのでバケツを白に切り替え
  CGContextSetGrayFillColor(ctx, 1.0f, 1.0f);

  // アンチエイリアスを有効 (境界をなめらかにするらしい)
  CGContextSetShouldAntialias(ctx, true);

  // フォントをなめらかにするらしい (アンチエイリアスとの違いがよくわからんが)
  CGContextSetAllowsFontSmoothing(ctx, true);
  CGContextSetShouldSmoothFonts(ctx, true);

  // なんか自分で解放してはダメらしい
  CFStringRef font_name = CFSTR("BIZ UDGothic");

  // 試しにサイズを 1:2 にして Menlo だと100% にするとはみ出す
  // 半角文字の比は厳密に 1:2 というわけではないのか
  font = CTFontCreateWithName(font_name, size, nullptr);
  if (font == nullptr) {
    return nullptr;
  }

  // UTF-16 にしてやる必要がある
  UniChar unichar_c = 0;

  if (cvtUTF8ToUTF16BMP(reinterpret_cast<const uint8_t *>(chracter),
                        std::strlen(chracter), &unichar_c,
                        1) != 1) {
    return nullptr;
  }

  // 文字コードとは別のフォント内の番号らしい
  CGGlyph glyph = 0;

  // 文字コードからグリフ番号を得る (複数の文字もできるらしい)
  if (!CTFontGetGlyphsForCharacters(font, &unichar_c, &glyph, 1)) {
    return nullptr;
  }

  // 文字の形全体を囲む最小の矩形を得る
  CGRect glyph_bounds = {};
  CTFontGetBoundingRectsForGlyphs(font, kCTFontOrientationDefault, &glyph,
                                  &glyph_bounds, 1);

  // 中心からどれくらい文字が下に伸びるかの値を取得するらしい
  CGFloat descent = CTFontGetDescent(font);

  // 左下が原点
  // 文字を書く位置を指定する
  // 端っこではなくなるべく中心になるように
  CGPoint text_position =
      CGPointMake((width - (glyph_bounds.size.width)) * 0.5f, descent);

  // 文字を描画する (複数の文字を一括でやることもできる)
  CTFontDrawGlyphs(font, &glyph, &text_position, 1, ctx);

  // テクスチャへのコピー
  device->addRef();
  TextureDesc desc;
  desc.drawable_flag = TextureDrawable::Disable;
  desc.format = TextureFormat::Mono;

  ITexture *texture = device->createTexture(width, height, desc);
  if (texture == nullptr) {
    device->release();
    return nullptr;
  }

  texture->upload(bitmap_data, total_bytes, bytes_per_row);

  device->release();
  return texture;
}
