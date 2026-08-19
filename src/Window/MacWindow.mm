#include "../Application/ImpApplication.h"
#include "IApplication.h"
#include "IWindow.h"
#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#include <cstdlib>
#include <cstring>
#include <new>

// ----------------------------
// ウィンドウの閉じるボタンを押した
// 時のイベントを受け取るメソッド
// ----------------------------
@interface WindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) bool shouldClose;
@end

@implementation WindowDelegate
- (BOOL)windowShouldClose:(NSWindow *)sender {
  self.shouldClose = true;
  return YES;
}
@end

namespace {
// ----------------------------
// 具象クラス宣言
// ----------------------------
class ImpMacWindow : public IWindow {
private:
  int ref_count = 0;

  int addRef(void) override;
  int release(void) override;

public:
  static ImpMacWindow *createWindow(int, int, const char *);
};

int ImpMacWindow::addRef(void) { return ++this->ref_count; }

int ImpMacWindow::release(void) {
  if (--this->ref_count == 0) {
    this->~ImpMacWindow();
    free(this);
    return 0;
  }
  return this->ref_count;
}

ImpMacWindow *ImpMacWindow::createWindow(int width, int height,
                                         const char *title) {
  @autoreleasepool {
    return nullptr;
  }
}
} // namespace

IWindow *ImpApplication::createWindow(int width, int height,
                                      const char *title) {
  ImpMacWindow *window = ImpMacWindow::createWindow(width, height, title);
  return window;
}
