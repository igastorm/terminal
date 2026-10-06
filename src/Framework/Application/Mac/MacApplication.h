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
  
  MacApplication();
  ~MacApplication();
  
 	MacApplication(const MacApplication&) = delete;
	MacApplication& operator=(const MacApplication&) = delete;

public:
  bool initPlatform();

  void terminate() override;

  void dispatchEvent(const Event &);
  static MacApplication* getAppInstance();
};
