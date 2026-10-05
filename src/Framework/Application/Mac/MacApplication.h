#pragma once
#include "../Application.hpp"
#include "IApplication.hpp"
#import <AppKit/AppKit.h>

@class AppDelegate;

struct ApplicationData {
  AppDelegate *appDelegate = nil;
  NSMenuItem *quit_item = nil;
};

class MacApplication : public Application {
private:
  ApplicationData data = {};
  bool run(const char *, IAppHandler *) override;
  void postEvent() override;

public:
  MacApplication();
  ~MacApplication();
  bool initPlatform();

  void terminate() override;

  static void dispatchEvent(const Event &);
};
