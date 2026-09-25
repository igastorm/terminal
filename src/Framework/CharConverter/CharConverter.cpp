#include "CharConverter.hpp"

// 以下をもとに実装
// https://ja.wikipedia.org/wiki/UTF-8
// https://ja.wikipedia.org/wiki/UTF-16
// https://ja.wikipedia.org/wiki/Unicode#サロゲートペア
// dst_cap は文字数単位
// サイズは要素単位
// 一文字分専用
// 別に文字列全体にも対応しているが code_point の容量チェックがめんどくさいので
// つまり最終引数は最後の文字のコードポイントを返す

CharConverter::Result
CharConverter::cvtUTF8ToUTF32(const uint8_t *src, std::size_t src_len,
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
    return CharConverter::Result::Error;
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
        return CharConverter::Result::Incomplete;
      }

      b1 = src[i + 1];
      if (!isContinuationByte(b1)) {
        // 2バイト目なのに前のバイトの続きじゃなかったらおかしい
        // 2バイト目に入るべき値の範囲外
        i++;
        return CharConverter::Result::Invalid;
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
        return CharConverter::Result::Incomplete;
      }

      b1 = src[i + 1];
      b2 = src[i + 2];
      if (!isContinuationByte(b1) || !isContinuationByte(b2)) {
        // 2バイト目以降なのに前のバイトの続きじゃなかったらおかしい
        // 2バイト目以降に入るべき値の範囲外
        i++;
        return CharConverter::Result::Invalid;
      }

      // UTF-16 で16ビットをはみ出す (サロゲートというらしい) 部分
      // UTF-32 への変換だとしてもそのまま UTF-8 にみられる場合は不正らしい
      // 0x10000 ~ 0x10FFFF の範囲である必要がある
      // 0xED 0xA0 ~
      if (b0 == 0xED && 0xA0 <= b1) {
        i++;
        return CharConverter::Result::Invalid;
      }

      // 0xE0 0x80 ~ 0x9F は本来1バイトの文字を3バイトで表してるから不正らしい
      if (b0 == 0xE0 && 0x80 <= b1 && b1 <= 0x9F) {
        i++;
        return CharConverter::Result::Invalid;
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
        return CharConverter::Result::Incomplete;
      }

      b1 = src[i + 1];
      b2 = src[i + 2];
      b3 = src[i + 3];
      if (!isContinuationByte(b1) || !isContinuationByte(b2) ||
          !isContinuationByte(b3)) {
        // 2バイト目以降なのに前のバイトの続きじゃなかったらおかしい
        // 2バイト目以降に入るべき値の範囲外
        i++;
        return CharConverter::Result::Invalid;
      }

      // 0xF0 0x80 ~ 0x8F は不正らしい
      if (b0 == 0xF0 && 0x80 <= b1 && b1 <= 0x8F) {
        i++;
        return CharConverter::Result::Invalid;
      }

      // 0xF4 0x90 ~ は不正らしい
      if (b0 == 0xF4 && 0x90 <= b1) {
        i++;
        return CharConverter::Result::Invalid;
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
      return CharConverter::Result::Invalid;
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
  return CharConverter::Result::Success;
}

CharConverter::Result
CharConverter::cvtUTF32ToUTF16(std::uint32_t code_point, uint16_t (&dst)[2],
                               std::size_t *out_utf16_len) {
  std::size_t utf16_len = 0;
  dst[0] = 0;
  dst[1] = 0;
  // サロゲート領域 (0xD800〜0xDFFF) 自体 と 0x10FFFF 超えは不正
  if ((code_point >= 0xD800 && code_point <= 0xDFFF) || code_point > 0x10FFFF) {
    if (out_utf16_len != nullptr) {
      *out_utf16_len = 0;
    }
    return CharConverter::Result::Invalid;
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

  return CharConverter::Result::Success;
}

// 直接変換用
CharConverter::Result CharConverter::cvtUTF8ToUTF16(
    const uint8_t *src, std::size_t src_len, uint16_t (&dst)[2],
    std::size_t *consumed_src_bytes, std::size_t *out_utf16_len,
    std::uint32_t *out_code_point) {
  dst[0] = 0;
  dst[1] = 0;
  std::uint32_t code_point = 0;
  CharConverter::Result res = CharConverter::cvtUTF8ToUTF32(
      src, src_len, consumed_src_bytes, &code_point);
  if (res != CharConverter::Result::Success) {
    return res;
  }

  if (out_code_point != nullptr) {
    *out_code_point = code_point;
  }

  return cvtUTF32ToUTF16(code_point, dst, out_utf16_len);
}
