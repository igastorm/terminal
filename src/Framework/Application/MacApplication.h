#pragma once
#include "IApplication.hpp"
#include "ImpApplication.hpp"
#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

@class AppDelegate;

struct ImpApplicationData {
  AppDelegate *appDelegate;
};

using ImpApplication = ImpApplicationTemplate<ImpApplicationData>;

class MacApplication : public ImpApplication {
public:
  bool initPlatform();
};
