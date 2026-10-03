#pragma once
#include "../CommonApplication.hpp"
#include "IApplication.hpp"
#import <AppKit/AppKit.h>

@class AppDelegate;

struct ApplicationData {
  AppDelegate *appDelegate = nil;
  NSMenuItem *quit_item = nil;
};

class MacApplication : public CommonApplication {
private:
  ApplicationData data = {};
  IWindow *createWindow(int, int, const char *) override;
  IGraphicsDevice *createGraphicsDevice() override;
  bool run(const char *, IAppHandler *) override;
  void postEvent() override;

public:
  bool initPlatform();

  void terminate() override;

  void dispatchEvent(const Event &);
};
