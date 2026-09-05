# ---------------------------------------------------------------------------
# 任意のバイナリファイルを配列としてヘッダに変換する
#
#   cmake -DINPUT=<in> -DOUTPUT=<out.h> -DVARNAME=<ident> -P bin2hpp.cmake
#
# 生成物:
#   static const unsigned char <VARNAME>[]     … 中身
#   static const unsigned long <VARNAME>_size  … バイト数
#
# ---------------------------------------------------------------------------

if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT OR NOT DEFINED VARNAME)
  message(
    FATAL_ERROR
    "bin2hpp.cmake: INPUT / OUTPUT / VARNAME を指定してください")
endif()

if(NOT EXISTS "${INPUT}")
  message(FATAL_ERROR "bin2hpp.cmake: 入力が見つかりません: ${INPUT}")
endif()

# ファイル全体を 16 進文字列として読む
file(READ "${INPUT}" hex_string HEX)

string(LENGTH "${hex_string}" hex_length)
math(EXPR byte_count "${hex_length} / 2")

if(byte_count EQUAL 0)
  message(FATAL_ERROR "bin2hpp.cmake: 入力が空です: ${INPUT}")
endif()

# 12バイト分 (24文字)ごとに改行＋インデントを挿入 (空白2)
# "." は任意の文字という意味らしい
string(
  REGEX REPLACE "(........................)" "\\1\n  "
  hex_lines "${hex_string}")

# "0a1b" → "0x0a,0x1b,"
# 入力: ${hex_string}
# 検索パターン: "([0-9a-f][0-9a-f])":
#   16進数2文字（1バイト分）にマッチ
#   () で囲んでマッチした2文字がグループ1として保存
# 置換パターン: "0x\\1,"
#   \\1 (グループ1) の前後に 0x と , を付加
# 結果 (body)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," body "${hex_lines}")

get_filename_component(input_name "${INPUT}" NAME)

set(content
"/* -----------------------------------------------------------------
 *   source : ${input_name}
 *   bytes  : ${byte_count}
 * ----------------------------------------------------------------- */
#pragma once

inline constexpr unsigned char ${VARNAME}[] = {
${body}
};

inline constexpr unsigned long ${VARNAME}_size = ${byte_count}UL;
")

file(WRITE "${OUTPUT}" "${content}")
message(STATUS "bin2hpp: ${input_name} -> ${OUTPUT} (${byte_count} bytes)")
