#include "../Application/ImpApplication.hpp"
#include "../Application/MacApplication.h"
#include "IWindow.hpp"
#include "ImpWindow.hpp"
#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

// C++ 側から呼ばれる可能性があるもの意外 ([NSApp run])
// の中からしか呼ばれないものは @autoreleasepool がいらないらしい　

// ----------------------------
// キー入力と画面描画
// ----------------------------
@interface WindowView : NSView <NSTextInputClient>
@property(nonatomic, assign) ImpApplication<ImpApplicationData> *appInstance;
@property(nonatomic, assign) IWindow *iwindow;
@end

@implementation WindowView
- (void)sendEventHelper:(const char *)utf8 {
  Event event;
  event.type = EventType::TextInput;
  event.window = self.iwindow;
  event.text.utf8 = utf8;
  event.text.len = std::strlen(utf8);
  self.appInstance->dispatchEvent(event);
}

// この View はキーボードフォーカスを受け取れるかという問い合わせに対して YES
// を返す
- (BOOL)acceptsFirstResponder {
  return YES;
}

// フォーカスが自分に移ったことを承諾
- (BOOL)becomeFirstResponder {
  return YES;
}

// WM_PAINT みたいなやつ
// 初回表示やリサイズ時に OS から描画要求が来たとき
- (void)drawRect:(NSRect)dirtyRect {
  Event event;
  event.type = EventType::WindowExpose;
  event.window = self.iwindow;
  self.appInstance->dispatchEvent(event);
}

// ----------------------------
// キー入力
// ----------------------------

// ----------------------------
// キーが押されたとき
// ----------------------------
- (void)keyDown:(NSEvent *)event {
  // 入力から修飾キー (押されていれば) を除いたキーを取り出す
  // 例えば Crtl+C だったら "c" が返ってくる
  // マルチバイト文字とかもあるから文字列で受け取るらしい
  NSString *chars = [event charactersIgnoringModifiers];
  if (chars.length > 0) {
    // 先頭の文字を取り出す (なんか UTF-16 らしい)
    // 普通 UTF-8 だと思うが従うしかない
    unichar c = [chars characterAtIndex:0];

    // Ctrl と 他の修飾キーが押されているとき (ただし Cmd キーは除く)
    if ((event.modifierFlags & NSEventModifierFlagControl) &&
        !(event.modifierFlags & NSEventModifierFlagCommand)) {
      // 以下では Ctrl キーと組み合わせたコードを生成する
      // 下記サイトを参考
      // https://www.dojeun.com/contentsview.php?listid=00016
      // とりあえず ASCII コード表にあるものだけ
      if ((c >= '@' && c <= '_') || (c >= 'a' && c <= 'z')) {
        char ctrl_code = (char)(c & 0x1F);

        // onEvent とか C++ 側のコードはコールベースで Cocoa
        // 側から呼ばれるのでスタック上のポインタを渡しても安全なはず
        const char buffer[2] = {ctrl_code, '\0'};
        [self sendEventHelper:buffer];
        return;
      }
    } else if (c >= NSF1FunctionKey && c <= NSF12FunctionKey) {
      const char *seq = CommonWindow::fkeys[c - NSF1FunctionKey];
      [self sendEventHelper:seq];
      return;
    }
  }
  // 通常文字や日本語 IME、特殊キーは Cocoa のテキスト入力エンジンに解釈させる
  // これにより下の insertText: や doCommandBySelector: が呼ばれるらしい
  [self interpretKeyEvents:[NSArray arrayWithObject:event]];
}

// ----------------------------
// 通常の文字または IME で入力が確定したとき
// ----------------------------
//  id は万能ポインタらしい (多分 void* っぽいやつ)
// NSStinrg* ではないのは NSAttributedString* がくる場合があるかららしい
// NSAttributedString は色情報とかを含んでいるらしい
// 多分 word とかにコピペすると色とかが保持される仕組みに対応するやつ
- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
  NSString *str = nil;

  // string の型が NSStinrg* か NSAttributedString* を判別する
  if ([string isKindOfClass:[NSAttributedString class]]) {
    // 純粋な文字列だけ取り出してそのポインタをコピー (中身はコピーしない)
    str = [(NSAttributedString *)string string];
  } else {
    // そのままポインタをコピー (中身はコピーしない)
    str = (NSString *)string;
  }

  if (str != nil && str.length > 0) {
    // ここでやっと UTF-8 にもって来れる
    const char *utf8 = [str UTF8String];

    // ここもスタック上のポインタを渡しても安全なはず
    [self sendEventHelper:utf8];
  }
}

// ----------------------------
// 矢印キーとかが入力されたとき
// ----------------------------
- (void)doCommandBySelector:(SEL)selector {
  // ポインタ先の文字列が const
  // ポインタ自体は付け替え可能
  // ポインタを const にするには char* const とする
  // (割と記法がややこしいのでメモ)
  const char *seq = nullptr;

  // switch を使いたいところだが無理っぽい
  if (selector == @selector(insertNewline:)) {
    seq = "\r"; // Enter
  } else if (selector == @selector(deleteBackward:)) {
    seq = "\x7f"; // Backspace
  } else if (selector == @selector(insertTab:)) {
    seq = "\t"; // Tab
  } else if (selector == @selector(cancelOperation:)) {
    seq = "\x1b"; // ESC
  } else if (selector == @selector(moveUp:)) {
    seq = "\033[A"; // ↑
  } else if (selector == @selector(moveDown:)) {
    seq = "\033[B"; // ↓
  } else if (selector == @selector(moveRight:) || selector == @selector
                                                      (moveForward:)) {
    seq = "\033[C"; // →
  } else if (selector == @selector(moveLeft:) || selector == @selector
                                                     (moveBackward:)) {
    seq = "\033[D"; // ←
  } else if (selector == @selector(deleteForward:)) {
    seq = "\033[3~"; // Delete
  }

  if (seq != nullptr) {
    // ここもスタック上のポインタを渡しても安全なはず
    [self sendEventHelper:seq];
  }
}

// ----------------------------
// NSTextInputClient のメソッド
// は実装必須らしい
// ----------------------------

// ----------------------------
// 未確定の文字のプレビューをどこに
// 表示するか
// ----------------------------
- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selectedRange
     replacementRange:(NSRange)replacementRange {
}

// ----------------------------
// キャンセル
// ----------------------------
- (void)unmarkText {
}

// ----------------------------
// 未確定文字があるかを IME に
// 伝える
// ----------------------------
- (NSRange)selectedRange {
  return NSMakeRange(NSNotFound, 0);
}

// ----------------------------
// 未確定テキストの開始位置と文字数
// を返す
// ----------------------------
- (NSRange)markedRange {
  return NSMakeRange(NSNotFound, 0);
}

// ----------------------------
// マウスとかで選択されているテキスト
// を返す
// ----------------------------
- (BOOL)hasMarkedText {
  return NO;
}

// ----------------------------
// 画面の指定された位置にどの文字が
// あるかを IME に伝える
// ----------------------------
- (nullable NSAttributedString *)
    attributedSubstringForProposedRange:(NSRange)range
                            actualRange:(nullable NSRangePointer)actualRange {
  return nil;
}

// ----------------------------
// 未確定文字列に対して色とかの装飾
// に対応しているかを配列で伝える
// ----------------------------
- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText {
  return [NSArray array];
}

// ----------------------------
// 変換候補のポップアップをどこに
// 表示するか
// ----------------------------
- (NSRect)firstRectForCharacterRange:(NSRange)range
                         actualRange:(nullable NSRangePointer)actualRange {
  return NSZeroRect;
}

// ----------------------------
// 画面上のクリックされた位置が
// 何文字目の位置かを IME に伝える
// ----------------------------
- (NSUInteger)characterIndexForPoint:(NSPoint)point {
  return 0;
}
@end

// ----------------------------
// ウィンドウデリゲート
// ----------------------------
@interface WindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) ImpApplication<ImpApplicationData> *appInstance;
@property(nonatomic, assign) IWindow *iwindow;
@end

@implementation WindowDelegate
// ----------------------------
// ウィンドウの閉じるボタンを押した
// 時のイベントを受け取るメソッド
// ----------------------------
- (BOOL)windowShouldClose:(NSWindow *)sender {
  Event event;
  event.type = EventType::WindowCloseRequest;
  event.window = self.iwindow;
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
  WindowView *view = nil;
};

using ImpMacWindow = ImpWindow<ImpWindowData>;

template <> ImpMacWindow::~ImpWindow<ImpWindowData>() {
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

template <> bool ImpMacWindow::show() {
  @autoreleasepool {
    [this->data.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    return true;
  }
}

template <> bool ImpMacWindow::hide() {
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
    // ウィンドウデリゲート
    window->data.delegate = [[WindowDelegate alloc] init];
    window->data.delegate.iwindow = window;
    window->data.delegate.appInstance = appInstance;
    [window->data.window setDelegate:window->data.delegate];

    // ビュー
    window->data.view = [[WindowView alloc] initWithFrame:frame];
    window->data.view.iwindow = window;
    window->data.view.appInstance = appInstance;
    [window->data.window setContentView:window->data.view];

    // あった方がいいらしい
    // 確実に view にフォーカスを当てるためらしい
    [window->data.window makeFirstResponder:window->data.view];

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
