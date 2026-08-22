#include "MacApplication.h"
#include "ImpApplication.hpp"
#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#include <cstdio>
#include <cstdlib>
#include <new>

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property(nonatomic, assign) IApplication *appInstance;
@property(nonatomic, assign) IAppHandler *handler;
@end

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)notification {
  // onInit 呼び出し
  if (!self.handler->onInit(self.appInstance)) {
    // onInit が失敗したらアプリを終了させる
    [NSApp terminate:nil];
  }
}

// Cmd+Q や Dock からの終了要求が来たときに呼ばれるらしい
- (NSApplicationTerminateReply)applicationShouldTerminate:
    (NSApplication *)sender {
  // 自動で終了せずに自前の処理を経由させる
  self.appInstance->terminate();
  // Cocoa による終了処理をキャンセル
  return NSTerminateCancel;
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender
                    hasVisibleWindows:(BOOL)hasVisibleWindows {
  return NO;
}
@end

using ImpMacApplicaton = ImpApplication<ImpApplicationData>;

template <> bool ImpMacApplicaton::initPlatform(void) {
  @autoreleasepool {
    // NSApplication の初期化（決まり文句らしい？）
    [NSApplication sharedApplication];
    // Dock アイコンやメニューバーを持つアプリとして設定する意味らしい
    // Cocoa 側で extern NSApplication *NSApp;
    // が宣言されているらしいのでそのまま使える
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    this->data.appDelegate = [[AppDelegate alloc] init];
    this->data.appDelegate.appInstance = this;
    [NSApp setDelegate:this->data.appDelegate];
    return true;
  }
}

template <> void ImpMacApplicaton::terminate(void) {
  @autoreleasepool {
    [NSApp stop:nil];
    NSEvent *event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                        location:NSMakePoint(0, 0)
                                   modifierFlags:0
                                       timestamp:0
                                    windowNumber:0
                                         context:nil
                                         subtype:0
                                           data1:0
                                           data2:0];
    [NSApp postEvent:event atStart:YES];
  }
}

template <> void ImpMacApplicaton::dispatchEvent(const Event &event) {
  if (this->handler != nullptr) {
    if (this->handler->onEvent(this, event) == AppResult::Continue) {
      return;
    }
  }
  this->terminate();
}

template <> bool ImpMacApplicaton::run(IAppHandler *handler) {
  @autoreleasepool {
    this->handler = handler;
    this->data.appDelegate.handler = handler;

    [NSApp run];

    // Cmd+Q だとここには戻らずに applicationShouldTerminate へ飛ぶ
    // NSTerminateCancel によってここに戻ってくるっぽい
    if (this->handler != nullptr) {
      this->handler->onQuit(this);
      this->handler = nullptr;
    }

    [this->data.appDelegate release];
    this->data.appDelegate = nil;
    return true;
  }
}

// Common だがここで実装しないと Cocoa の初期化が呼べない気がする
// あと startApp から呼ぶため
CommonApplication *CommonApplication::init(void) {
  ImpMacApplicaton *app =
      static_cast<ImpMacApplicaton *>(std::malloc(sizeof(ImpMacApplicaton)));
  if (app == nullptr) {
    std::perror("malloc failed (Application)");
    return nullptr;
  }
  app = new (app) ImpMacApplicaton;
  app->addRef();
  // プラットフォーム依存部分の初期化
  if (!app->initPlatform()) {
    std::perror("initPlatform Failed");
    app->release();
    return nullptr;
  }
  return app;
}
