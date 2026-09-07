#pragma once
#include "../Application/ImpApplication.hpp"
#include "../Application/MacApplication.h"
#include "IWindow.hpp"
#include "ImpWindow.hpp"
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
@property(nonatomic, assign) ImpApplication<ImpApplicationData> *appInstance;
@property(nonatomic, assign) IWindow *iwindow;
@end

// ----------------------------
// ウィンドウデリゲート
// ----------------------------
@interface WindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) ImpApplication<ImpApplicationData> *appInstance;
@property(nonatomic, assign) IWindow *iwindow;
@end

@interface CocoaWindow : NSWindow
@end

struct ImpWindowData {
  CocoaWindow *window = nil;
  WindowDelegate *delegate = nil;
  WindowView *view = nil;
  bool resizing = false;
};

using ImpWindow = ImpWindowTemplate<ImpWindowData, ImpApplicationData>;

class ImpMacWindow : public ImpWindow {
public:
  ImpMacWindow() = default;
  ~ImpMacWindow() = default;
  void notifyResizing(bool);
};
