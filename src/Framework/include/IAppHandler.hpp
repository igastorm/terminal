#pragma once

#include "Event.hpp"

class IApplication;

class IAppHandler {
public:
    virtual ~IAppHandler() = default;

    // 起動時
    virtual bool onInit(IApplication* app) = 0;

    // 毎フレーム呼ばれる更新 & 描画ループ
    // virtual AppResult onIterate(IApplication* app) = 0;

    // イベント発生時に呼ばれる
    virtual AppResult onEvent(IApplication* app, const Event& event) = 0;

    // アプリ終了時
    virtual void onQuit(IApplication* app) = 0;
};