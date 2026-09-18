#pragma once
#include "IApplication.hpp"
#include "../ApplicationTemplate.hpp"
#import <AppKit/AppKit.h>

@class AppDelegate;

struct ApplicationData {
  AppDelegate *appDelegate = nil;
  NSMenuItem* quit_item = nil;
};

using Application = ApplicationTemplate<ApplicationData>;

class MacApplication : public Application {
public:
  bool initPlatform();
};
