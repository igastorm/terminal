#pragma once
#include "../Application/ImpApplication.hpp"
#include "CommonWindow.hpp"

template <class PlatformData, class PlatformApplicationData>
class ImpWindow : public CommonWindow {
private:
  PlatformData data;
  ImpApplication<PlatformApplicationData> *appInstance;

  bool setTitle(const char *) override;
  bool show() override;
  bool hide() override;

public:
  static ImpWindow *createWindow(ImpApplication<PlatformApplicationData> *, int,
                                 int, const char *);
  void getPlatformData(const PlatformData **) const;

  ~ImpWindow() override;
};
