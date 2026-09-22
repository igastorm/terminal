#include "MacFontAtlas.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new> // IWYU pragma: keep
#include <wchar.h>

enum class CvtCharCodeResult {
  Success,    // 正常に1文字デコードできた
  Incomplete, // バイトが途中で切れている (次の read() のデータを待つべき)
  Invalid,    // 明らかな不正 (0xFF など。1バイト読み飛ばして '' を出すべき)
  Error       // 引数がおかしい
};

// 以下をもとに実装
// https://ja.wikipedia.org/wiki/UTF-8
// https://ja.wikipedia.org/wiki/UTF-16
// https://ja.wikipedia.org/wiki/Unicode#サロゲートペア
// dst_cap は文字数単位
// サイズは要素単位
// 一文字分専用
// 別に文字列全体にも対応しているが code_point の容量チェックがめんどくさいので
// つまり最終引数は最後の文字のコードポイントを返す

CvtCharCodeResult cvtUTF8ToUTF32(const uint8_t *src, std::size_t src_len,
                                 std::size_t *consumed_src_bytes,
                                 std::uint32_t *out_code_point) {
  // 継続バイトかの判定
  // 文字の先頭ではなく, 前のバイトの続きであることを示す値
  // 2バイト目以降の下限から上限の範囲内か
  auto isContinuationByte = [](uint8_t b) -> bool {
    // 10000000 ~ 10111111
    return (0x80 <= b && b <= 0xBF);
  };

  if (src == nullptr || src_len == 0 || out_code_point == nullptr ||
      consumed_src_bytes == nullptr) {
    return CvtCharCodeResult::Error;
  }

  std::size_t &i = *consumed_src_bytes;
  i = 0;

  std::uint32_t &code_point = *out_code_point;
  code_point = 0;

  while (i < src_len) {
    // 各文字の先頭バイト
    uint8_t b0 = src[i];

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
        // ただし後から続きを取得できるかも
        i = 0;
        return CvtCharCodeResult::Incomplete;
      }

      b1 = src[i + 1];
      if (!isContinuationByte(b1)) {
        // 2バイト目なのに前のバイトの続きじゃなかったらおかしい
        // 2バイト目に入るべき値の範囲外
        i++;
        return CvtCharCodeResult::Invalid;
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
        // ただし後から続きを取得できるかも
        i = 0;
        return CvtCharCodeResult::Incomplete;
      }

      b1 = src[i + 1];
      b2 = src[i + 2];
      if (!isContinuationByte(b1) || !isContinuationByte(b2)) {
        // 2バイト目以降なのに前のバイトの続きじゃなかったらおかしい
        // 2バイト目以降に入るべき値の範囲外
        i++;
        return CvtCharCodeResult::Invalid;
      }

      // UTF-16 で16ビットをはみ出す (サロゲートというらしい) 部分
      // UTF-32 への変換だとしてもそのまま UTF-8 にみられる場合は不正らしい
      // 0x10000 ~ 0x10FFFF の範囲である必要がある
      // 0xED 0xA0 ~
      if (b0 == 0xED && 0xA0 <= b1) {
        i++;
        return CvtCharCodeResult::Invalid;
      }

      // 0xE0 0x80 ~ 0x9F は本来1バイトの文字を3バイトで表してるから不正らしい
      if (b0 == 0xE0 && 0x80 <= b1 && b1 <= 0x9F) {
        i++;
        return CvtCharCodeResult::Invalid;
      }

      // 識別ビットを削除して繋げる
      code_point = static_cast<uint32_t>(b0 & 0x0F) << 12 |
                   static_cast<uint32_t>(b1 & 0x3F) << 6 |
                   static_cast<uint32_t>(b2 & 0x3F);
      i += 3;
    } else if (b0 >= 0xF0 && b0 <= 0xF4) {
      // 4バイトの UTF-8 (1バイト目はすでに b0 に入ってる)
      // 11110000 ~ 11110100
      uint8_t b1 = 0, b2 = 0, b3 = 0;
      if (i + 3 >= src_len) {
        // 続きのデータがないならエラー
        // ただし後から続きを取得できるかも
        i = 0;
        return CvtCharCodeResult::Incomplete;
      }

      b1 = src[i + 1];
      b2 = src[i + 2];
      b3 = src[i + 3];
      if (!isContinuationByte(b1) || !isContinuationByte(b2) ||
          !isContinuationByte(b3)) {
        // 2バイト目以降なのに前のバイトの続きじゃなかったらおかしい
        // 2バイト目以降に入るべき値の範囲外
        i++;
        return CvtCharCodeResult::Invalid;
      }

      // 0xF0 0x80 ~ 0x8F は不正らしい
      if (b0 == 0xF0 && 0x80 <= b1 && b1 <= 0x8F) {
        i++;
        return CvtCharCodeResult::Invalid;
      }

      // 0xF4 0x90 ~ は不正らしい
      if (b0 == 0xF4 && 0x90 <= b1) {
        i++;
        return CvtCharCodeResult::Invalid;
      }

      // 識別ビットを削除して繋げる
      code_point = static_cast<uint32_t>(b0 & 0x07) << 18 |
                   static_cast<uint32_t>(b1 & 0x3F) << 12 |
                   static_cast<uint32_t>(b2 & 0x3F) << 6 |
                   static_cast<uint32_t>(b3 & 0x3F);
      i += 4;
    } else {
      // その他は不正
      i++;
      return CvtCharCodeResult::Invalid;
    }

    // 一文字分の処理が完了したら即時抜ける
    break;
  }
  if (out_code_point != nullptr) {
    *out_code_point = code_point;
  }
  if (consumed_src_bytes != nullptr) {
    *consumed_src_bytes = i;
  }
  return CvtCharCodeResult::Success;
}

CvtCharCodeResult cvtUTF32ToUTF16(std::uint32_t code_point, uint16_t (&dst)[2],
                                  std::size_t *out_utf16_len) {
  std::size_t utf16_len = 0;
  dst[0] = 0;
  dst[1] = 0;
  // サロゲート領域 (0xD800〜0xDFFF) 自体 と 0x10FFFF 超えは不正
  if ((code_point >= 0xD800 && code_point <= 0xDFFF) || code_point > 0x10FFFF) {
    if (out_utf16_len != nullptr) {
      *out_utf16_len = 0;
    }
    return CvtCharCodeResult::Invalid;
  }

  if (code_point <= 0xFFFF) {
    // サロゲートでない
    dst[0] = static_cast<uint16_t>(code_point);
    utf16_len = 1;
  } else if (0x10000 <= code_point && code_point <= 0x10FFFF) {
    // 10000000000000000 ~ 100001111111111111111
    // サロゲート
    uint32_t tmp = code_point - 0x10000;
    uint16_t high = static_cast<uint16_t>(
        (tmp >> 10) + 0xD800); // 0x400 で割って 0xD800 を足す
    uint16_t low = static_cast<uint16_t>(
        (tmp & 0x3FF) + 0xDC00); // 0x400 で割った余りに 0xDC00 を足す
    dst[0] = high;
    dst[1] = low;
    utf16_len = 2;
  }

  if (out_utf16_len != nullptr) {
    *out_utf16_len = utf16_len;
  }

  return CvtCharCodeResult::Success;
}

// 直接変換用
CvtCharCodeResult cvtUTF8ToUTF16(const uint8_t *src, std::size_t src_len,
                                 uint16_t (&dst)[2],
                                 std::size_t *consumed_src_bytes,
                                 std::size_t *out_utf16_len,
                                 std::uint32_t *out_code_point) {
  dst[0] = 0;
  dst[1] = 0;
  std::uint32_t code_point = 0;
  CvtCharCodeResult res =
      cvtUTF8ToUTF32(src, src_len, consumed_src_bytes, &code_point);
  if (res != CvtCharCodeResult::Success) {
    return res;
  }

  if (out_code_point != nullptr) {
    *out_code_point = code_point;
  }

  return cvtUTF32ToUTF16(code_point, dst, out_utf16_len);
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
  UniChar unichar_c[2] = {};

  size_t len = 0;
  size_t consumed = 0;
  std::uint32_t code_point = 0;

  CvtCharCodeResult result = cvtUTF8ToUTF16(
      reinterpret_cast<const uint8_t *>(chracter), std::strlen(chracter),
      unichar_c, &consumed, &len, &code_point);

  if (result != CvtCharCodeResult::Success) {
    return nullptr;
  }

  if (len == 0) {
    return nullptr;
  }

  if (unichar_c[0] <= 0xD800 && unichar_c[1]) {
    return nullptr;
  }

  // 文字コードとは別のフォント内の番号らしい
  CGGlyph glyph = 0;

  // 文字コードからグリフ番号を得る (複数の文字もできるらしい)
  // フォントが対応してない文字だとエラー
  if (!CTFontGetGlyphsForCharacters(
          font, reinterpret_cast<UniChar *>(unichar_c), &glyph, len)) {
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

  texture->upload(bitmap_data, total_bytes, bytes_per_row,
                  {0, 0, static_cast<int>(width), static_cast<int>(height)});

  device->release();
  return texture;
}

//  ========================================================
//
//  Mac Font Atlas Helper
//
//  ========================================================

CTFontRef MacFontAtlasHelper::createCTFont(const char *font_name,
                                           float font_size) {
  if (font_name == nullptr || font_size == 0.0f) {
    return nullptr;
  }

  CFStringRef cf_font_name = CFStringCreateWithCString(
      kCFAllocatorDefault, font_name, kCFStringEncodingUTF8);
  if (cf_font_name == nullptr) {
    return nullptr;
  }

  // 第三引数は斜体とかを作りたい時に使うらしい
  CTFontRef font = CTFontCreateWithName(cf_font_name, font_size, nullptr);
  CFRelease(cf_font_name);

  return font;
}

CellSize MacFontAtlasHelper::getCellSize(CTFontRef font) {
  if (font == nullptr) {
    return {};
  }

  // 大文字の M のグリフを取得する
  // 等幅の場合, これに合わせるとちょうどいいらしい
  UniChar char_M = 'M';
  CGGlyph glyph_M = 0;
  CTFontGetGlyphsForCharacters(font, &char_M, &glyph_M, 1);

  // 次の文字に進むとどれだけ位置が進むかを取得
  // kCTFontOrientationHorizontal なので横方向
  // まとめると文字のセルに必要な横幅を取得している
  CGSize advance_M = {};
  CTFontGetAdvancesForGlyphs(font, kCTFontOrientationHorizontal, &glyph_M,
                             &advance_M, 1);

  // ベースラインから上に必要な高さ
  CGFloat ascent = CTFontGetAscent(font);

  // ベースラインから下に必要な高さ
  // こいつはメンバ変数
  CGFloat descent = CTFontGetDescent(font);

  // 推奨される行間の間隔
  CGFloat leading = CTFontGetLeading(font);

  CellSize cell_size = {};

  cell_size.descent = descent;

  // 小数点以下を切り上げておく
  cell_size.cell_width = std::ceill(advance_M.width);
  // ascent + descent + leading の中に実質 advance_M.height
  // が含まれているようなもの
  cell_size.cell_height = std::ceill(ascent + descent + leading);

  return cell_size;
}

CGContextRef MacFontAtlasHelper::createBitmapContext(std::uint8_t *bitmap_data,
                                                     size_t bytes_bitmap_data,
                                                     int width, int height) {
  if (bitmap_data == nullptr || width == 0 || height == 0) {
    return nullptr;
  }

  size_t total_bytes = width * height * sizeof(std::uint8_t);
  if (total_bytes != bytes_bitmap_data) {
    return nullptr;
  }

  // 白黒フォーマットで作成
  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceGray();
  if (color_space == nullptr) {
    return nullptr;
  }

  CGContextRef ctx = CGBitmapContextCreate(
      bitmap_data, width, height, 8 * sizeof(std::uint8_t),
      width * sizeof(std::uint8_t), color_space, kCGImageAlphaNone);
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

  // 文字をなめらかにするらしい
  CGContextSetAllowsFontSmoothing(ctx, true);
  CGContextSetShouldSmoothFonts(ctx, true);

  return ctx;
}

bool MacFontAtlasHelper::drawBitmap(CGContextRef ctx, CTFontRef font,
                                    CellSize cell_size,
                                    const UniChar *unichar_c, size_t len,
                                    int campas_width, int campas_height, int x,
                                    int y) {
  if (ctx == nullptr || font == nullptr) {
    return false;
  }
  if (campas_width == 0 || campas_height == 0 ||
      cell_size.cell_height == 0.0f) {
    return false;
  }

  // 一文字分しか受け付けないようにする
  if (len == 0 || len > 2 || unichar_c == nullptr) {
    return false;
  }
  if (len == 2 && (unichar_c[0] <= 0xD800 || 0xDBFF <= unichar_c[0])) {
    return false;
  }

  CGGlyph glyph = 0;
  if (!CTFontGetGlyphsForCharacters(font, unichar_c, &glyph, len)) {
    return false;
  }

  // CoreGraphics は左下が原点なので変換が必要
  int cg_y = campas_height - (y + cell_size.cell_height);

  // CoreGraphics は左下原点だからベースラインの位置は descent を足せばいい
  CGPoint pos = CGPointMake(x, cg_y + cell_size.descent);
  CTFontDrawGlyphs(font, &glyph, &pos, 1, ctx);

  return true;
}

//  ========================================================
//
//  Mac Font Atlas
//
//  ========================================================

MacFontAtlas::MacFontAtlas(IGraphicsDevice *device) : FontAtlas(device) {}

MacFontAtlas::~MacFontAtlas() {
  // device と texture は親のデストラクタで参照カウントを減らしている
  if (this->data.font != nullptr) {
    CFRelease(this->data.font);
    this->data.font = nullptr;
  }
  if (this->data.ctx != nullptr) {
    CGContextRelease(this->data.ctx);
    this->data.ctx = nullptr;
  }
  // on_demand_bitmap_data は親のコンストラクタで free してる
}

MacFontAtlas *MacFontAtlas::createMacFontAtlas(IGraphicsDevice *device,
                                               const char *font_name,
                                               float font_size) {
  if (std::strlen(font_name) == 0) {
    return nullptr;
  }

  MacFontAtlas *font_atlas =
      static_cast<MacFontAtlas *>(std::malloc(sizeof(MacFontAtlas)));
  if (font_atlas == nullptr) {
    std::perror("malloc failed (createFontAtlas)");
    return nullptr;
  }

  // device はコンストラクタで参照カウントを増やしてある
  font_atlas = new (font_atlas) MacFontAtlas(device);

  font_atlas->data.font =
      MacFontAtlasHelper::createCTFont(font_name, font_size);
  if (font_atlas->data.font == nullptr) {
    font_atlas->release();
    return nullptr;
  }

  // 半角文字 セルサイズを取得 (小数点以下切り上げ済み)
  CellSize cell_size = MacFontAtlasHelper::getCellSize(font_atlas->data.font);
  if (cell_size.cell_height == 0.0f || cell_size.cell_width == 0.0f) {
    font_atlas->release();
    return nullptr;
  }
  font_atlas->cell_width = cell_size.cell_width;
  font_atlas->cell_height = cell_size.cell_height;

  // とりあえず 512 x 512 = 約 256KB 分
  int atlas_width = 512;
  int atlas_height = 512;
  font_atlas->atlas_width = atlas_width;
  font_atlas->atlas_height = atlas_height;
  const int total_bytes = atlas_width * atlas_height * sizeof(std::uint8_t);

  // 一行当たりの文字数を計算しておく (半角ベース)
  font_atlas->cols_per_row =
      atlas_width / static_cast<int>(font_atlas->cell_width);

  std::uint8_t *bitmap_data = static_cast<std::uint8_t *>(
      std::calloc(total_bytes, sizeof(std::uint8_t)));
  if (bitmap_data == nullptr) {
    font_atlas->release();
    return nullptr;
  }

  font_atlas->data.ctx = MacFontAtlasHelper::createBitmapContext(
      bitmap_data, total_bytes, atlas_width, atlas_height);
  if (font_atlas->data.ctx == nullptr) {
    // ctx が bitmap を参照してるので free は後ろに書く必要がある
    font_atlas->release();
    std::free(bitmap_data);
    return nullptr;
  }

  // アトラステクスチャは一次元的にしたいところがだが GPU の回路上, 縦,
  // 横の大きさに上限があるらしく二次元的に作る必要がある
  // あと縦とか横にに極端にでかいとキャッシュ効率が悪いらしい
  // ' ' から '~' まで
  for (int i = 0; i < 95; i++) {
    char c = static_cast<char>(i + ' ');
    UniChar unichar_c[2] = {};
    size_t len = 0;
    size_t consumed = 0;
    std::uint32_t code_point = 0;
    CvtCharCodeResult cvt_result =
        cvtUTF8ToUTF16(reinterpret_cast<const std::uint8_t *>(&c),
                       sizeof(c) / sizeof(std::uint8_t), unichar_c, &consumed,
                       &len, &code_point);
    if (cvt_result != CvtCharCodeResult::Success) {
      // ctx が bitmap を参照してるので free は後ろに書く必要がある
      font_atlas->release();
      std::free(bitmap_data);
      return nullptr;
    }

    // グリッド上の位置
    // col は最終列まで行ったら自動的に巻き戻される
    // row は最終列まで行ったら自動的に大きくなる
    int col = i % font_atlas->cols_per_row;
    int row = i / font_atlas->cols_per_row;
    int x = col * static_cast<int>(font_atlas->cell_width);
    int y = row * static_cast<int>(font_atlas->cell_height);

    if (!MacFontAtlasHelper::drawBitmap(
            font_atlas->data.ctx, font_atlas->data.font, cell_size, unichar_c,
            len, atlas_width, atlas_height, x, y)) {
      // ctx が bitmap を参照してるので free は後ろに書く必要がある
      font_atlas->release();
      std::free(bitmap_data);
      return nullptr;
    }

    // UV 座標の記録
    // 0.0f ~ 1.0f に正規化してる
    // 頂点座標のようにピクセル座標で受け付けるようにシェーダを改造するのもあり
    font_atlas->glyph_table[i].u_min = x / static_cast<float>(atlas_width);
    font_atlas->glyph_table[i].v_min = y / static_cast<float>(atlas_height);
    font_atlas->glyph_table[i].u_max =
        (x + font_atlas->cell_width) / static_cast<float>(atlas_width);
    font_atlas->glyph_table[i].v_max =
        (y + font_atlas->cell_height) / static_cast<float>(atlas_height);
  }

  TextureDesc desc;
  desc.format = TextureFormat::Mono;
  desc.drawable_flag = TextureDrawable::Disable;

  font_atlas->texture = device->createTexture(atlas_width, atlas_height, desc);
  if (font_atlas->texture == nullptr) {
    std::free(bitmap_data);
    font_atlas->release();
    return nullptr;
  }

  if (!font_atlas->texture->upload(bitmap_data, atlas_width * atlas_height,
                                   atlas_width * sizeof(std::uint8_t),
                                   {0, 0, atlas_width, atlas_height})) {
    std::free(bitmap_data);
    font_atlas->release();
    return nullptr;
  }

  // オブジェクト自体は破棄するがオンデマンド描画用に変数は再利用する
  CGContextRelease(font_atlas->data.ctx);
  font_atlas->data.ctx = nullptr;
  std::free(bitmap_data);

  // オンデマンドキャッシュ生成用のビットマップをあらかじめ用意
  // サイズは全角文字一つ分
  size_t size = font_atlas->cell_width * 2.0f * font_atlas->cell_height *
                sizeof(std::uint8_t);
  font_atlas->on_demand_bitmap_data =
      static_cast<std::uint8_t *>(std::calloc(size, sizeof(std::uint8_t)));
  if (font_atlas->on_demand_bitmap_data == nullptr) {
    font_atlas->release();
    return nullptr;
  }

  font_atlas->data.ctx = MacFontAtlasHelper::createBitmapContext(
      font_atlas->on_demand_bitmap_data, size, font_atlas->cell_width * 2.0f,
      font_atlas->cell_height);
  if (font_atlas->data.ctx == nullptr) {
    font_atlas->release();
    return nullptr;
  }

  // カーソルを初期化
  font_atlas->rewindCursor();

  return font_atlas;
}

GlyphUV MacFontAtlasHelper::getOrCreateGlyphUV(FontAtlas *font_atlas_template,
                                               uint32_t code_point,
                                               UniChar unichar_c[2],
                                               size_t utf16_len, int cols) {
  // 中身は MacFontAtlas のはずだから大丈夫なキャスト
  MacFontAtlas *font_atlas = static_cast<MacFontAtlas *>(font_atlas_template);

  // ハッシュテーブルを検索
  size_t start_idx = font_atlas->hashCodepoint(code_point);
  size_t idx = start_idx;
  while (font_atlas->glyph_hash_table[idx].codepoint != 0 &&
         font_atlas->glyph_hash_table[idx].codepoint != code_point) {
    // 末尾まで行ったら自動で巻き戻る
    // % 使って自動折り返ししてたやつの高速版 & すると結果的にあまりが出てくる
    // 一見すると if で抜けるので巻き戻しが不要だが start_idx
    // からではなく全体から見れば一周する可能性もある
    idx = (idx + 1) & (HashEntry::HASH_SIZE - 1);
    if (idx == start_idx) {
      // 一周したなら満タンを意味する (キャッシュフラッシュ)
      std::memset(font_atlas->glyph_hash_table, 0,
                  sizeof(font_atlas->glyph_hash_table));
      font_atlas->rewindCursor();
      idx = font_atlas->hashCodepoint(code_point);
      break;
    }
  }

  HashEntry *entry = &font_atlas->glyph_hash_table[idx];

  // すでにキャッシュにあれば、その UV を返す
  if (entry->codepoint == code_point) {
    return entry->glyph_table;
  }

  // 未キャッシュの場合
  float char_width = font_atlas->cell_width * cols;
  int atlas_width = font_atlas->texture->getWidth();
  int atlas_height = font_atlas->texture->getHeight();

  // 横幅チェック
  // はみ出すなら行を進める
  if (font_atlas->cursor_x + char_width > atlas_width) {
    font_atlas->cursor_x = 0.0f;
    font_atlas->cursor_y += font_atlas->cell_height;
  }

  // 高さチェク
  // はみ出すならフラッシュ (満タン)
  if (font_atlas->cursor_y + font_atlas->cell_height > atlas_height) {
    std::memset(font_atlas->glyph_hash_table, 0,
                sizeof(font_atlas->glyph_hash_table));
    font_atlas->rewindCursor();
    // フラッシュしたのでインデックスを再取得
    idx = font_atlas->hashCodepoint(code_point);
    entry = &font_atlas->glyph_hash_table[idx];
  }

  // 作業用ビットマップをクリア
  size_t on_demand_size =
      (font_atlas->cell_width * 2) * font_atlas->cell_height;
  std::memset(font_atlas->on_demand_bitmap_data, 0, on_demand_size);

  CellSize cell_size = MacFontAtlasHelper::getCellSize(font_atlas->data.font);

  if (!MacFontAtlasHelper::drawBitmap(
          font_atlas->data.ctx, font_atlas->data.font, cell_size, unichar_c,
          utf16_len, char_width, font_atlas->cell_height, 0, 0)) {
    return {};
  }

  // texure 上のカーソル位置に焼く
  if (!font_atlas->texture->upload(
          font_atlas->on_demand_bitmap_data,
          char_width * font_atlas->cell_height, char_width,
          {static_cast<int>(font_atlas->cursor_x),
           static_cast<int>(font_atlas->cursor_y), static_cast<int>(char_width),
           static_cast<int>(font_atlas->cell_height)})) {
    return {};
  }

  // GlyphUV を生成
  GlyphUV uv = {};
  uv.u_min = font_atlas->cursor_x / static_cast<float>(atlas_width);
  uv.v_min = font_atlas->cursor_y / static_cast<float>(atlas_height);
  uv.u_max =
      (font_atlas->cursor_x + char_width) / static_cast<float>(atlas_width);
  uv.v_max = (font_atlas->cursor_y + font_atlas->cell_height) /
             static_cast<float>(atlas_height);

  entry->codepoint = code_point;
  entry->glyph_table = uv;

  // カーソルを進める
  font_atlas->cursor_x += char_width;

  return uv;
}

template <>
bool FontAtlas::drawText(IRenderPass *pass, const char *str, float start_x,
                         float start_y, std::uint32_t color) {
  // 文字から UV 座標を取得
  auto getGlyphUV = [this](char c) -> GlyphUV {
    if (c >= 32 && c <= 126) {
      return this->glyph_table[c - 32];
    }
    return this->glyph_table[0]; // 範囲外はスペース
  };

  float current_x = start_x;
  float y = start_y;
  const uint8_t *ptr = reinterpret_cast<const uint8_t *>(str);
  size_t remaining = std::strlen(str);

  while (remaining > 0) {
    float cw = this->cell_width;
    float ch = this->cell_height;
    GlyphUV uv = {};
    size_t consumed = 0;
    uint8_t b0 = *ptr;

    if (b0 >= 32 && b0 <= 126) {
      // ----------------------------
      // ASCII 文字の場合
      // ----------------------------
      uv = getGlyphUV(static_cast<char>(b0));
      consumed = 1;
    } else {
      // ----------------------------
      // 非 ASCII 文字の場合
      // ----------------------------
      UniChar unichar_c[2] = {};
      uint32_t code_point = 0;
      size_t utf16_len = 0;

      CvtCharCodeResult cvt_result = cvtUTF8ToUTF16(
          ptr, remaining, unichar_c, &consumed, &utf16_len, &code_point);
      if (cvt_result != CvtCharCodeResult::Success) {
        // 壊れた文字はスキップ (置換文字にするのもあり)
        ptr++;
        remaining--;
        continue;
      }

      // 文字幅 (半角なら 1, 全角なら 2)
      int cols = wcwidth(static_cast<wchar_t>(code_point));
      if (cols <= 0) {
        cols = 1;
      }
      cw = cw * cols;

      uv = MacFontAtlasHelper::getOrCreateGlyphUV(this, code_point, unichar_c,
                                                  utf16_len, cols);
    }
    VertexTex quad[6] = {
        {{current_x, y}, {uv.u_min, uv.v_min}, color},
        {{current_x + cw, y}, {uv.u_max, uv.v_min}, color},
        {{current_x, y + ch}, {uv.u_min, uv.v_max}, color},
        {{current_x, y + ch}, {uv.u_min, uv.v_max}, color},
        {{current_x + cw, y}, {uv.u_max, uv.v_min}, color},
        {{current_x + cw, y + ch}, {uv.u_max, uv.v_max}, color},
    };
    pass->drawVerticesTex(this->texture, quad, 6);

    // 進める
    current_x += cw;
    ptr += consumed;
    remaining -= consumed;
  }

  return true;
}
