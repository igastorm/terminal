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
#include <new>

@interface WindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) ImpApplication<ImpApplicationData> *appInstance;
@property(nonatomic, assign) IWindow *window;
@end

@implementation WindowDelegate
// ----------------------------
// ウィンドウの閉じるボタンを押した
// 時のイベントを受け取るメソッド
// ----------------------------
- (BOOL)windowShouldClose:(NSWindow *)sender {
  Event event;
  event.type = EventType::WindowCloseRequest;
  event.window = self.window;
  self.appInstance->dispatchEvent(event);
  return NO;
}
@end

//  ----------------------------
//  具象クラス
//  ----------------------------
struct ImpWindowData {
  NSWindow *window = nil;
  WindowDelegate *delegate = nil;
};

using ImpMacWindow = ImpWindow<ImpWindowData>;

template <> ImpMacWindow::~ImpWindow<ImpWindowData>(void) {
  @autoreleasepool {
    if (this->data.window) {
      [this->data.window close];
      [this->data.window release];
      this->data.window = nil;
    }
    if (this->data.delegate) {
      [this->data.delegate release];
      this->data.delegate = nil;
    }
  }
}

template <> bool ImpMacWindow::setTitle(const char *title) {
  @autoreleasepool {
    NSString *ns_title = [NSString stringWithUTF8String:title];
    [this->data.window setTitle:ns_title];
    return true;
  }
}

template <> bool ImpMacWindow::show(void) {
  @autoreleasepool {
    [this->data.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    return true;
  }
}

template <> bool ImpMacWindow::hide(void) {
  @autoreleasepool {
    [this->data.window orderOut:nil];
    return true;
  }
}

template <>
template <>
ImpMacWindow *ImpMacWindow::createWindow<ImpApplicationData>(
    ImpApplication<ImpApplicationData> *appInstance, int width, int height,
    const char *title) {
  @autoreleasepool {
    ImpMacWindow *window =
        static_cast<ImpMacWindow *>(std::malloc(sizeof(ImpMacWindow)));
    if (window == nullptr) {
      std::perror("malloc failed (createWindow)");
      return nullptr;
    }

    window = new (window) ImpMacWindow;
    window->addRef();

    // ウィンドウを生成
    NSRect frame = NSMakeRect(0, 0, width, height);
    NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                           NSWindowStyleMaskMiniaturizable |
                           NSWindowStyleMaskResizable;
    window->data.window =
        [[NSWindow alloc] initWithContentRect:frame
                                    styleMask:styleMask
                                      backing:NSBackingStoreBuffered
                                        defer:NO];

    // 閉じられた時に自動リリースされないようにする
    window->data.window.releasedWhenClosed = NO;

    // 中央に配置
    [window->data.window center];

    // WndProc みたいなやつの設定
    window->data.delegate = [[WindowDelegate alloc] init];
    window->data.delegate.window = window;
    window->data.delegate.appInstance = appInstance;
    [window->data.window setDelegate:window->data.delegate];
    window->setTitle(title);
    window->show();

    return window;
  }
}
//} // namespace

template <>
IWindow *ImpApplication<ImpApplicationData>::createWindow(int width, int height,
                                                          const char *title) {
  ImpMacWindow *window = ImpMacWindow::createWindow(this, width, height, title);
  return window;
}
