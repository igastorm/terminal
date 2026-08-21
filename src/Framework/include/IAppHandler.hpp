#pragma once

#include "Event.hpp"

class IApplication;

class IAppHandler {
public:
  virtual ~IAppHandler() = default;

  // 起動時
  virtual bool onInit(IApplication *app) = 0;

  // 毎フレーム呼ばれる更新 & 描画ループ
  // WM_PAINT のように画面描画すべきイベントがならそのイベントが来た時と PTY
  // が更新された時だけ描画すればいいのでターミナルアプリでは不要かも
  // PTY は別スレッドで動かすつもり
  //
  // virtual AppResult
  // onIterate(IApplication* app) = 0;

  // イベント発生時に呼ばれる
  virtual AppResult onEvent(IApplication *app, const Event &event) = 0;

  // アプリ終了時
  virtual void onQuit(IApplication *app) = 0;
};
