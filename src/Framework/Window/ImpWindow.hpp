#pragma once
#include "../Application/ImpApplication.hpp"
#include "CommonWindow.hpp"

template <class PlatformData, class PlatformApplicationData>
class ImpWindowTemplate : public CommonWindow {
protected:
  PlatformData data;
  ImpApplication<PlatformApplicationData> *appInstance;

  bool setTitle(const char *) override;
  bool show() override;
  bool hide() override;

public:
  static ImpWindowTemplate *createWindow(ImpApplication<PlatformApplicationData> *, int,
                                 int, const char *);
  PlatformData getPlatformData() const;

  ~ImpWindowTemplate() override;
};
