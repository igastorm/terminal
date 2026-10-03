#pragma once
#include "../Application/Mac/MacApplication.h"
#include "../CommonWindow.hpp"
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

class MacWindow : public CommonWindow {
private:
  WindowData data = {};

  bool setTitle(const char *) override;
  bool show() override;
  bool hide() override;

public:
  [[nodiscard]] WindowData getPlatformData() const;
  MacWindow(IApplication *);
  ~MacWindow();
  void notifyResizing(bool);
  [[nodiscard]] static MacWindow *createWindow(MacApplication *, int, int,
                                               const char *);
};
