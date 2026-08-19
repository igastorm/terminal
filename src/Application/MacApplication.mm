#include "ImpApplication.hpp"
#import <Cocoa/Cocoa.h>

bool ImpApplication::initPlatform(void) {
  @autoreleasepool {
    // NSApplication の初期化（決まり文句らしい？）
    [NSApplication sharedApplication];
    // Dock アイコンやメニューバーを持つアプリとして設定する意味らしい
    // Cocoa 側で extern NSApplication *NSApp;
    // が宣言されているらしいのでそのまま使える
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    return true;
  }
}
