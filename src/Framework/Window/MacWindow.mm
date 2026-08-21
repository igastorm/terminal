#include "../Application/ImpApplication.hpp"
#include "IWindow.hpp"
#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

// ----------------------------
// ウィンドウの閉じるボタンを押した
// 時のイベントを受け取るメソッド
// ----------------------------
@interface WindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) ImpApplication *appInstance;
@property(nonatomic, assign) IWindow *window;
@end

@implementation WindowDelegate
- (BOOL)windowShouldClose:(NSWindow *)sender {
  Event event;
  event.type = EventType::WindowCloseRequest;
  event.window = self.window;
  self.appInstance->dispatchEvent(event);
  return NO;
}
@end

namespace {
// ----------------------------
// 具象クラス宣言
// ----------------------------
class ImpMacWindow : public IWindow {
private:
  int ref_count = 0;
  NSWindow *window = nil;
  WindowDelegate *delegate = nil;

  int addRef(void) override;
  int release(void) override;

  bool setTitle(const char *) override;
  bool show(void) override;
  bool hide(void) override;

public:
  static ImpMacWindow *createWindow(ImpApplication *, int, int, const char *);
  ~ImpMacWindow(void);
};

ImpMacWindow::~ImpMacWindow(void) {
  @autoreleasepool {
    if (this->window) {
      [this->window close];
      [this->window release];
      this->window = nil;
    }
    if (this->delegate) {
      [this->delegate release];
      this->delegate = nil;
    }
  }
}

int ImpMacWindow::addRef(void) { return ++this->ref_count; }

int ImpMacWindow::release(void) {
  if (--this->ref_count == 0) {
    this->~ImpMacWindow();
    free(this);
    return 0;
  }
  return this->ref_count;
}

bool ImpMacWindow::setTitle(const char *title) {
  @autoreleasepool {
    NSString *ns_title = [NSString stringWithUTF8String:title];
    [this->window setTitle:ns_title];
    return true;
  }
}

bool ImpMacWindow::show(void) {
  @autoreleasepool {
    [this->window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    return true;
  }
}

bool ImpMacWindow::hide(void) {
  @autoreleasepool {
    [this->window orderOut:nil];
    return true;
  }
}

ImpMacWindow *ImpMacWindow::createWindow(ImpApplication *appInstance, int width,
                                         int height, const char *title) {
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
    window->window =
        [[NSWindow alloc] initWithContentRect:frame
                                    styleMask:styleMask
                                      backing:NSBackingStoreBuffered
                                        defer:NO];

    // 閉じられた時に自動リリースされないようにする
    window->window.releasedWhenClosed = NO;

    // 中央に配置
    [window->window center];

    // WndProc みたいなやつの設定
    window->delegate = [[WindowDelegate alloc] init];
    window->delegate.window = window;
    window->delegate.appInstance = appInstance;
    [window->window setDelegate:window->delegate];
    window->setTitle(title);
    window->show();

    return window;
  }
}
} // namespace

IWindow *ImpApplication::createWindow(int width, int height,
                                      const char *title) {
  ImpMacWindow *window = ImpMacWindow::createWindow(this, width, height, title);
  return window;
}
