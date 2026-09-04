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

using ImpMacApplicaton = ImpApplication<ImpApplicationData>;

template <> int ImpMacApplicaton::addRef();
template <> bool ImpMacApplicaton::initPlatform();
template <> void ImpMacApplicaton::terminate();
template <> void ImpMacApplicaton::dispatchEvent(const Event &event);
template <> bool ImpMacApplicaton::run(IAppHandler *handler);
