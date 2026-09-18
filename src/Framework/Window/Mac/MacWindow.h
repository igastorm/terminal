#pragma once
#include "../Application/Mac/MacApplication.h"
#include "../WindowTemplate.hpp"
#include "IWindow.hpp"
#import <AppKit/AppKit.h>

// ----------------------------
// キー入力と画面描画イベント
// ----------------------------
@interface WindowView : NSView <NSTextInputClient>
@property(nonatomic, assign) MacApplication *appInstance;
@property(nonatomic, assign) IWindow *iwindow;
@end

// ----------------------------
// ウィンドウデリゲート
// ----------------------------
@interface WindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) MacApplication *appInstance;
@property(nonatomic, assign) IWindow *iwindow;
@end

@interface CocoaWindow : NSWindow
@end

struct WindowData {
  CocoaWindow *window = nil;
  WindowDelegate *delegate = nil;
  WindowView *view = nil;
  bool resizing = false;
};

using Window = WindowTemplate<WindowData, ApplicationData>;

class MacWindow : public Window {
public:
  MacWindow(IApplication *);
  ~MacWindow() = default;
  void notifyResizing(bool);
  [[nodiscard]] static MacWindow *createWindow(Application *, int, int,
                                               const char *);
};
