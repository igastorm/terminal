#pragma once
#include "IWindow.hpp"
#include <cstddef>

enum class EventType {
  None,
  WindowCloseRequest, // 閉じるボタンが押された
  WindowResize,       // ウィンドウがリサイズされた
  KeyDown,            // キー押下
  KeyUp,              // キー離脱
  TextInput,          // 文字列が確定入力された (UTF-8)
  MouseDown,
  MouseUp,
  MouseMove,
  MouseScroll,
};

enum class AppResult {
  Continue, // アプリを継続
  Quit      // アプリを終了する
};

struct Event {
  EventType type = EventType::None;
  IWindow *window = nullptr;

  union {
    struct {
      int width;
      int height;
    } resize;
    struct {
      char utf8[32];
      size_t len;
    } text;
    struct {
      int keyCode;
    } key;
    struct {
      int button;
      int x;
      int y;
    } mouse;
    struct {
      float deltaX;
      float deltaY;
    } scroll;
  };
};
