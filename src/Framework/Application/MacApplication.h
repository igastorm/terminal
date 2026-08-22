#pragma once
#include "IApplication.hpp"
#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

@class AppDelegate;

struct ImpApplicationData {
  AppDelegate *appDelegate;
};
