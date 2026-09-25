#pragma once
#include <cstddef>
#include <cstdint>

class CharConverter {
public:
  enum class Result {
    Success,    // 正常に1文字デコードできた
    Incomplete, // バイトが途中で切れている (次の read() のデータを待つべき)
    Invalid,    // 明らかな不正 (0xFF など。1バイト読み飛ばして '' を出すべき)
    Error       // 引数がおかしい
  };

  static Result cvtUTF8ToUTF32(const std::uint8_t *src, std::size_t src_len,
                        std::size_t *consumed_src_bytes,
                        std::uint32_t *out_code_point);
  static Result cvtUTF32ToUTF16(std::uint32_t code_point, uint16_t (&dst)[2],
                         std::size_t *out_utf16_len);
  static Result cvtUTF8ToUTF16(const uint8_t *src, std::size_t src_len,
                        uint16_t (&dst)[2], std::size_t *consumed_src_bytes,
                        std::size_t *out_utf16_len,
                        std::uint32_t *out_code_point);

  CharConverter() = delete;
  ~CharConverter() = default;
};
