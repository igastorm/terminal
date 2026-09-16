#pragma once
#include "../Application/ApplicationTemplate.hpp"
#include "../Application/MacApplication.h"
#include "IWindow.hpp"
#include "WindowTemplate.hpp"
#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>

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
  MacWindow(IApplication*);
  ~MacWindow() = default;
  void notifyResizing(bool);
  [[nodiscard]] static MacWindow *
  createWindow(Application *, int, int,
               const char *);
};
