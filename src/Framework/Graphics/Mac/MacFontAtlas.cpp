#include "MacFontAtlas.hpp"
#include "CharConverter.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new> // IWYU pragma: keep
#include <wchar.h>

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

  CharConverter::Result result = CharConverter::cvtUTF8ToUTF16(
      reinterpret_cast<const uint8_t *>(chracter), std::strlen(chracter),
      unichar_c, &consumed, &len, &code_point);

  if (result != CharConverter::Result::Success) {
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
    UniChar replacement_char = 0x25A1;
    if (!CTFontGetGlyphsForCharacters(font, &replacement_char, &glyph, 1)) {
      return false;
    }
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
                                               float font_size,
                                               int atlash_width,
                                               int atlash_height) {
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

  int atlas_width = atlash_width;
  int atlas_height = atlash_height;
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
    CharConverter::Result cvt_result = CharConverter::cvtUTF8ToUTF16(
        reinterpret_cast<const std::uint8_t *>(&c),
        sizeof(c) / sizeof(std::uint8_t), unichar_c, &consumed, &len,
        &code_point);
    if (cvt_result != CharConverter::Result::Success) {
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
    // 本来 0.0f ~ 1.0f に正規化されるが
    // ampler_desc.normalizedCoordinates = NO;
    // によってピクセル座標で渡せる
    font_atlas->glyph_table[i].u_min = x;
    font_atlas->glyph_table[i].v_min = y;
    font_atlas->glyph_table[i].u_max = x + font_atlas->cell_width;
    font_atlas->glyph_table[i].v_max = y + font_atlas->cell_height;
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

template <> GlyphUV FontAtlas::getGlyphUV(wchar_t code_point) {
  // ASCII の範囲はそのまま配列から即座に返す
  if (0x20 <= code_point && code_point <= 0x7E) {
    return this->glyph_table[code_point - 0x20];
  }

  // ハッシュテーブルから探す
  const HashEntry *entry = this->findEntry(static_cast<uint32_t>(code_point));
  if (entry != nullptr) {
    // キャッシュヒット
    return entry->glyph_table;
  }

  // キャッシュに無かった場合 (事前 preload 漏れなど)
  // ここで flash や upload をするとバグる
  return {};
}

template <> bool FontAtlas::updateGlyphCache(const wchar_t code_point) {
  // キャッシュフラッシュ関数
  auto flash = [this]() -> void {
    std::memset(this->glyph_hash_table, 0, sizeof(this->glyph_hash_table));
    this->rewindCursor();
  };

  // ASCII は何もしない
  if (0x20 <= code_point && code_point <= 0x7E) {
    return true;
  }

  // すでにキャッシュにあれば, 何もしない
  if (this->findEntry(code_point) != nullptr) {
    return true;
  }

  int cols = wcwidth(static_cast<wchar_t>(code_point));
  if (cols <= 0) {
    cols = 1;
  }
  const float char_width = this->cell_width * cols;
  const int atlas_width = this->texture->getWidth();
  const int atlas_height = this->texture->getHeight();

  // 横幅チェック (はみ出すなら改行)
  if (this->cursor_x + char_width > atlas_width) {
    this->cursor_x = 0.0f;
    this->cursor_y += this->cell_height;
  }

  // 高さチェック (はみ出すなら容量不足だからフラッシュ)
  if (this->cursor_y + this->cell_height > atlas_height) {
    flash();
  }

  // 空きをさがす
  size_t start_idx = this->hashCodepoint(code_point);
  size_t idx = start_idx;
  while (this->glyph_hash_table[idx].codepoint != 0) {
    idx = (idx + 1) & (HashEntry::HASH_SIZE - 1);
    if (idx == start_idx) {
      // テーブル満タンなら flash して最初から
      flash();
      idx = this->hashCodepoint(code_point);
      break;
    }
  }

  // entry は作業する要素へのポインタ
  HashEntry *entry = &this->glyph_hash_table[idx];

  // 焼き付け
  const size_t on_demand_bitmap_size =
      (this->cell_width * 2) * this->cell_height;
  std::memset(this->on_demand_bitmap_data, 0, on_demand_bitmap_size);
  CellSize cell_size = MacFontAtlasHelper::getCellSize(this->data.font);

  UniChar unichar_c[2] = {};
  std::size_t utf16_len = 0;
  if (CharConverter::cvtUTF32ToUTF16(code_point, unichar_c, &utf16_len) !=
      CharConverter::Result::Success) {
    return false;
  }

  if (!MacFontAtlasHelper::drawBitmap(this->data.ctx, this->data.font,
                                      cell_size, unichar_c, utf16_len,
                                      char_width, this->cell_height, 0, 0)) {
    return false;
  }

  // texure 上のカーソル位置に焼く
  // 非 ASCII 半角文字問題
  // 作業領域の幅が全角文字の幅固定なので bytes_per_row
  // はセル幅の2倍にする必要がある 例えば半角文字をオンデマンドキャッシュする時
  // 作業ビットマップの左半分にだけ描画される
  // その際, 右側は黒のまま
  // それを線形に戻すと横幅バイトごとに白の部分と黒の部分が交互に現れることになる
  // (山と谷)
  // ここで bytes_per_row を半角の幅にしてしまうと偶数行目が黒になる
  // そこで bytes_per_row
  // を全角の幅にすると元のビットマップ領域と同じようにテクスチャへ焼かれる
  // しかし実際は半角なので右半分が無駄になる
  // そこで領域指定で半角の幅にすれば自動的に線形にしたときの偶数番目に現れる黒の塊がカットされる
  const size_t stride = this->cell_width * 2.0f;
  if (!this->texture->upload(
          this->on_demand_bitmap_data, stride * this->cell_height, stride,
          {static_cast<int>(this->cursor_x), static_cast<int>(this->cursor_y),
           static_cast<int>(char_width),
           static_cast<int>(this->cell_height)})) {
    return false;
  }

  // 7. エントリに UV を登録し、カーソルを進める
  entry->codepoint = code_point;
  entry->glyph_table.u_min = this->cursor_x;
  entry->glyph_table.v_min = this->cursor_y;
  entry->glyph_table.u_max = this->cursor_x + char_width;
  entry->glyph_table.v_max = this->cursor_y + this->cell_height;

  this->cursor_x += char_width;

  return true;
}

template <> bool FontAtlas::preloadGlyphs32(const wchar_t *str) {
  if (str == nullptr) {
    return false;
  }
  size_t remaining = wcslen(str);
  while (remaining > 0) {
    size_t consumed = 0;
    uint32_t code_point = *str;

    // u_max と v_max が 0.0f だったらエラーとみなせる
    const GlyphUV uv = this->getGlyphUV(code_point);

    if (uv.u_max == 0.0f || uv.v_max == 0.0f) {
      return false;
    }

    consumed++;
    str += consumed;
    remaining -= consumed;
  }

  return true;
}

// 基本的な文字列描画をテストするデモ関数
// 実際にターミナルで文字を描画するにはエスケープシーケンスはバッファの途切れを意識した実装を
// main.cpp 側で行う必要がある (そこまで Framework
// 側で実装するのは適切でないと考える)
template <>
bool FontAtlas::drawText(IRenderPass *pass, const char *str, float start_x,
                         float start_y, std::uint32_t color) {
  float current_x = start_x;
  const float y = start_y;
  const uint8_t *ptr = reinterpret_cast<const uint8_t *>(str);
  size_t remaining = std::strlen(str);

  while (remaining > 0) {
    size_t consumed = 0;
    uint32_t code_point = 0;

    // UTF-8 コードから UTF-32 コードを取得
    CharConverter::Result cvt_result =
        CharConverter::cvtUTF8ToUTF32(ptr, remaining, &consumed, &code_point);
    if (cvt_result != CharConverter::Result::Success) {
      // 壊れた文字はスキップ (置換文字にするのもあり)
      ptr++;
      remaining--;
      continue;
    }

    this->updateGlyphCache(code_point);
    const GlyphUV uv = this->getGlyphUV(code_point);

    // uv から計算すればそのままサイズがわかる
    float cw = uv.u_max - uv.u_min;
    float ch = uv.v_max - uv.v_min;

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
