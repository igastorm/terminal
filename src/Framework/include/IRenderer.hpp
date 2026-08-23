#pragma once
#include "IObject.hpp"
#include "IWindow.hpp"
#include <cstdint>

class IRenderer : public IObject {
public:
  virtual ~IRenderer() = default;

  // 以下みたいなアーキテクチャだと begin と end が不要になる
  // render() は引数に関数ポインタを持つ
  // 描画処理は render に渡す関数に集約する
  // render() は内部で begin と end の間で与えられた関数ポインタを実行する

  // 描画フレーム開始 (対象ウィンドウを指定してスワップチェーンを取得)
  virtual bool beginFrame(IWindow *window) = 0;

  // 画面クリア (RGBA)
  virtual void clear(std::uint32_t) = 0;

  // 描画フレーム終了 (Present / 画面フリップ)
  virtual void endFrame() = 0;
};
