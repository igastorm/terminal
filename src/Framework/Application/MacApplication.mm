#include "MacApplication.h"
#include "ImpApplication.hpp"
#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#include <cstdio>
#include <cstdlib>
#include <new>

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property(nonatomic, assign) ImpApplication<ImpApplicationData> *appInstance;
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
  // OS に最小化の復元やウィンドウ前面化を任せる
  if (hasVisibleWindows == YES) {
    return YES;
  }

  Event event;
  event.type = EventType::AppReopen;
  self.appInstance->dispatchEvent(event);
  return NO;
}
@end

template <> int ImpMacApplicaton::addRef() {
  // 恐らく appDelegate
  // は他のオブジェクトから参照することはないと思われるので retain しない
  /*
  if (this->data.appDelegate != nil) {
    [this->data.appDelegate retain];
  }
  */
  return this->addRefBase();
}

template <> bool ImpMacApplicaton::initPlatform() {
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

    // メニューバーの登録 (強参照だから所有権が引き継がれる)
    // autorelase は autoreleasepool を活用するため
    // 手動で参照カウントを減らすこともできるけどめんどくさい
    NSMenu *main_menu = [[[NSMenu alloc] init] autorelease];
    NSMenuItem *menu_item = [[[NSMenuItem alloc] init] autorelease];
    [main_menu addItem:menu_item];

    NSMenu *menu = [[[NSMenu alloc] init] autorelease];
    NSMenuItem *quit_item =
        [[[NSMenuItem alloc] initWithTitle:@"Quit"
                                    action:@selector(terminate:)
                             keyEquivalent:@"q"] autorelease];

    [menu addItem:quit_item];
    [menu_item setSubmenu:menu];

    // 最終的に NSApp が所有する
    [NSApp setMainMenu:main_menu];

    return true;
  }
}

template <> void ImpMacApplicaton::terminate() {
  @autoreleasepool {
    // ループが生きている間に実行しないとリークっぽくなる
    this->handler->onQuit(this);
  }

  // イベントキューに終了処理を加えることで遅延解放を確実に待ったあとでアプリケーションを終了できる
  // Objective-C のラムダ構文は奇妙なので f 版で C++ のラムダ式をブチ込むハック
  // キャプチャなしなら関数ポインタにキャストできるらしい
  dispatch_async_f(dispatch_get_main_queue(), nullptr,
                   [](void *context) -> void {
                     @autoreleasepool {
                       [NSApp stop:nil];

                       // stop を確実に効かせるためのダミーイベントを投げる
                       // stop
                       // で即座に停止するわけではなく次のイベントを処理した時に終了するらしい
                       NSEvent *event = [NSEvent
                           otherEventWithType:NSEventTypeApplicationDefined
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
                   });
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
  this->handler = handler;
  this->data.appDelegate.handler = handler;
  @autoreleasepool {
    [NSApp run];
  }
  // Cmd+Q だとここには戻らずに applicationShouldTerminate へ飛ぶ
  // NSTerminateCancel によってここに戻ってくるっぽい
  // しかし, ループが生きている間に実行しないとリークっぽくなるの Terminate
  // に引っ越し
  /*
  if (this->handler != nullptr) {
    @autoreleasepool {
      this->handler->onQuit(this);
    }
    this->handler = nullptr;
  }
  */

  @autoreleasepool {
    [this->data.appDelegate release];
    this->data.appDelegate = nil;
  }
  return true;
}

// Common だがここで実装しないと Cocoa の初期化が呼べない気がする
// あと startApp から呼ぶため
CommonApplication *createPlatformApplication() {
  ImpMacApplicaton *app =
      static_cast<ImpMacApplicaton *>(std::malloc(sizeof(ImpMacApplicaton)));
  if (app == nullptr) {
    std::perror("malloc failed (Application)");
    return nullptr;
  }
  app = new (app) ImpMacApplicaton;

  // この関数で生成するので ref_count を加算するだけ
  app->addRef();

  // プラットフォーム依存部分の初期化
  if (!app->initPlatform()) {
    std::perror("initPlatform Failed");
    app->release();
    return nullptr;
  }
  return app;
}
