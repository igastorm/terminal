#include "MacFontAtlas.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new> // IWYU pragma: keep

// 以下をもとに実装
// https://ja.wikipedia.org/wiki/UTF-8
// https://ja.wikipedia.org/wiki/UTF-16
// https://ja.wikipedia.org/wiki/Unicode#サロゲートペア
// dst_cap は文字数単位
// サイズは要素単位
size_t cvtUTF8ToUTF16(const uint8_t *src, size_t src_len, uint16_t *dst,
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

      // UTF-16 で16ビットをはみ出す (サロゲートというらしい) 部分
      // UTF-32 への変換だとしてもそのまま UTF-8 にみられる場合は不正らしい
      // 0x10000 ~ 0x10FFFF の範囲である必要がある
      // 0xED 0xA0 ~
      if (b0 == 0xED && 0xA0 <= b1) {
        return 0;
      }

      // 0xE0 0x80 ~ 0x9F は本来1バイトの文字を3バイトで表してるから不正らしい
      if (b0 == 0xE0 && 0x80 <= b1 && b1 <= 0x9F) {
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
      uint8_t b1 = 0, b2 = 0, b3 = 0;
      if (i + 3 >= src_len) {
        // 続きがないならエラー
        return 0;
      }

      b1 = src[i + 1];
      b2 = src[i + 2];
      b3 = src[i + 3];
      if (!isContinuationByte(b1) || !isContinuationByte(b2) ||
          !isContinuationByte(b3)) {
        // 2バイト目以降なのに前のバイトの続きじゃなかったらおかしい
        // 2バイト目以降に入るべき値の範囲外
        return 0;
      }

      // 0xF0 0x80 ~ 0x8F は不正らしい
      if (b0 == 0xF0 && 0x80 <= b1 && b1 <= 0x8F) {
        return 0;
      }

      // 0xF4 0x90 ~ は不正らしい
      if (b0 == 0xF4 && 0x90 <= b1) {
        return 0;
      }

      // 識別ビットを削除して繋げる
      code_point = static_cast<uint32_t>(b0 & 0x07) << 18 |
                   static_cast<uint32_t>(b1 & 0x3F) << 12 |
                   static_cast<uint32_t>(b2 & 0x3F) << 6 |
                   static_cast<uint32_t>(b3 & 0x3F);
      i += 4;
    } else {
      // その他は不正
      return 0;
    }
    if (code_point <= 0xFFFF && out < dst_cap) {
      // サロゲートでない
      dst[out] = static_cast<uint16_t>(code_point);
      out++;
    } else if (0x10000 <= code_point && code_point <= 0x10FFFF &&
               out + 1 < dst_cap) {
      // 10000000000000000 ~ 100001111111111111111
      // サロゲート
      uint32_t tmp = code_point - 0x10000;
      uint16_t high = (tmp >> 10) + 0xD800; // 0x400 で割って 0xD800 を足す
      uint16_t low =
          (tmp & 0x3FF) + 0xDC00; // 0x400 で割った余りに 0xDC00 を足す
      dst[out] = high;
      dst[out + 1] = low;
      out += 2;
    }
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
  UniChar unichar_c[2] = {};

  size_t len = cvtUTF8ToUTF16(reinterpret_cast<const uint8_t *>(chracter),
                              std::strlen(chracter), unichar_c, 2);

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
//  FontAtlas
//
//  ========================================================

MacFontAtlasHelper::MacFontAtlasHelper(const char *font_name, float font_size,
                                       int atlas_width, int atlas_height)
    : atlas_width(atlas_width), atlas_height(atlas_height) {
  this->is_ready = false;
  if (std::strlen(font_name) == 0 || this->atlas_width == 0 ||
      this->atlas_height == 0) {
    return;
  }

  this->cf_font_name = CFStringCreateWithCString(kCFAllocatorDefault, font_name,
                                                 kCFStringEncodingUTF8);
  if (this->cf_font_name == nullptr) {
    return;
  }

  // 第三引数は斜体とかを作りたい時に使うらしい
  this->font = CTFontCreateWithName(cf_font_name, font_size, nullptr);
  if (this->font == nullptr) {
    return;
  }

  if (!this->getCellSize()) {
    return;
  }

  this->is_ready = this->initCTX();
}

MacFontAtlasHelper::~MacFontAtlasHelper() {
  if (this->cf_font_name != nullptr) {
    CFRelease(this->cf_font_name);
    this->cf_font_name = nullptr;
  }
  if (this->font != nullptr) {
    CFRelease(this->font);
    this->font = nullptr;
  }
  if (this->ctx != nullptr) {
    CGContextRelease(this->ctx);
    this->ctx = nullptr;
  }
  if (this->bitmap_data != nullptr) {
    std::free(this->bitmap_data);
    this->bitmap_data = nullptr;
  }
}

bool MacFontAtlasHelper::getCellSize() {
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
  descent = CTFontGetDescent(font);

  // 推奨される行間の間隔
  CGFloat leading = CTFontGetLeading(font);

  // 小数点以下を切り上げておく
  this->cell_width = std::ceill(advance_M.width);
  // ascent + descent + leading の中に実質 advance_M.height
  // が含まれているようなもの
  this->cell_height = std::ceill(ascent + descent + leading);

  if (this->cell_width > 0.0f && this->cell_height > 0.0f) {
    return true;
  }
  return false;
}

bool MacFontAtlasHelper::initCTX() {
  // 白黒フォーマットで作成
  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceGray();

  if (color_space == nullptr) {
    return false;
  }

  this->cols_per_row = atlas_width / static_cast<int>(this->cell_width);
  size_t total_bytes = atlas_width * atlas_height;
  this->bitmap_data = static_cast<std::uint8_t *>(
      std::calloc(total_bytes, sizeof(std::uint8_t)));
  if (this->bitmap_data == nullptr) {
    return false;
  }

  this->ctx = CGBitmapContextCreate(
      this->bitmap_data, atlas_width, atlas_height, 8 * sizeof(std::uint8_t),
      atlas_width * sizeof(std::uint8_t), color_space, kCGImageAlphaNone);
  CGColorSpaceRelease(color_space);
  if (ctx == nullptr) {
    return false;
  }

  // 初期の塗りつぶし色ではなくペン (バケツのインクの色 ) の色のようなもの
  // 塗りつぶすと黒になる
  CGContextSetGrayFillColor(ctx, 0.0f, 1.0f);

  // 先ほど設定した黒で背景をクリア
  CGContextFillRect(ctx, CGRectMake(0, 0, atlas_width, atlas_height));

  // 文字は白で描画すべきなのでバケツを白に切り替え
  CGContextSetGrayFillColor(ctx, 1.0f, 1.0f);

  // アンチエイリアスを有効 (境界をなめらかにするらしい)
  CGContextSetShouldAntialias(ctx, true);

  // 文字をなめらかにするらしい
  CGContextSetAllowsFontSmoothing(ctx, true);
  CGContextSetShouldSmoothFonts(ctx, true);

  return true;
}

bool MacFontAtlasHelper::drawBitmap(char c, GlyphUV *uv, int col, int row) {
  if (uv == nullptr || this->font == nullptr) {
    return false;
  }

  UniChar unichar_c = static_cast<UniChar>(c);
  CGGlyph glyph = 0;
  if (!CTFontGetGlyphsForCharacters(this->font, &unichar_c, &glyph, 1)) {
    return false;
  }

  // ビットマップ上の位置
  int x = col * this->cell_width;
  int y = row * this->cell_height;
  // CoreGraphics は左下が原点なので変換が必要
  int cg_y = this->atlas_height - (row + 1) * this->cell_height;

  // CoreGraphics は左下原点だからベースラインの位置は descent を足せばいい
  CGPoint pos = CGPointMake(x, cg_y + descent);
  CTFontDrawGlyphs(font, &glyph, &pos, 1, this->ctx);

  // UV 座標の記録
  // 0.0f ~ 1.0f に正規化してる
  // 頂点座標のようにピクセル座標で受け付けるようにシェーダを改造するのもあり
  uv->u_min = x / static_cast<float>(this->atlas_width);
  uv->v_min = y / static_cast<float>(this->atlas_height);
  uv->u_max = (x + this->cell_width) / static_cast<float>(this->atlas_width);
  uv->v_max = (y + this->cell_height) / static_cast<float>(this->atlas_height);

  return true;
}

MacFontAtlas::MacFontAtlas(IGraphicsDevice *device) : FontAtlas(device) {}

MacFontAtlas::~MacFontAtlas() {
  // device と texture は親のデストラクタで参照カウントを減らしている
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

  // とりあえず 512 x 512 = 約 256KB 分
  constexpr int atlas_width = 512;
  constexpr int atlas_height = 512;

  MacFontAtlasHelper helper(font_name, font_size, atlas_width, atlas_height);
  if (!helper.isReady()) {
    font_atlas->release();
    return nullptr;
  }

  // セルサイズを得る
  // サイズは小数点以下切り上げ済み
  font_atlas->cell_width = helper.getCellWidth();
  font_atlas->cell_height = helper.getCellHeight();
  int cols_per_row = atlas_width / static_cast<int>(font_atlas->cell_width);

  // アトラステクスチャは一次元的にしたいところがだが GPU の回路上, 縦,
  // 横の大きさに上限があるらしく二次元的に作る必要がある
  // あと縦とか横にに極端にでかいとキャッシュ効率が悪いらしい
  // ' ' から '~' まで
  for (int i = 0; i < 95; i++) {
    char c = static_cast<char>(i + ' ');
    // グリッド上の位置
    // col は最終列まで行ったら自動的に巻き戻される
    // row は最終列まで行ったら自動的に大きくなる
    int col = i % cols_per_row;
    int row = i / cols_per_row;
    helper.drawBitmap(c, &font_atlas->glyph_table[i], col, row);
  }

  TextureDesc desc;
  desc.format = TextureFormat::Mono;
  desc.drawable_flag = TextureDrawable::Disable;

  font_atlas->texture = device->createTexture(atlas_width, atlas_height, desc);
  if (font_atlas->texture == nullptr) {
    return nullptr;
  }

  font_atlas->texture->upload(helper.getBitmap(), atlas_width * atlas_height,
                              atlas_width * sizeof(std::uint8_t),
                              {0, 0, atlas_width, atlas_height});

  return font_atlas;
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

  float cw = this->cell_width;
  float ch = this->cell_height;

  size_t len = std::strlen(str);

  for (int i = 0; i < len; i++) {
    char c = str[i];
    GlyphUV uv = getGlyphUV(c);

    float x = start_x + i * cw;
    float y = start_y;

    // 1文字分の四角形
    VertexTex quad[6] = {
        {{x, y}, {uv.u_min, uv.v_min}, color},
        {{x + cw, y}, {uv.u_max, uv.v_min}, color},
        {{x, y + ch}, {uv.u_min, uv.v_max}, color},

        {{x, y + ch}, {uv.u_min, uv.v_max}, color},
        {{x + cw, y}, {uv.u_max, uv.v_min}, color},
        {{x + cw, y + ch}, {uv.u_max, uv.v_max}, color},
    };

    pass->drawVerticesTex(this->texture, quad, 6);
    }

  return true;
}
