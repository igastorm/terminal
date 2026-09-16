#pragma once
#include "CommonWindow.hpp"

template <class PlatformData, class PlatformApplicationData>
class WindowTemplate : public CommonWindow {
protected:
  PlatformData data;

  bool setTitle(const char *) override;
  bool show() override;
  bool hide() override;

  WindowTemplate(IApplication *appInstance) : CommonWindow(appInstance) {}

public:
  [[nodiscard]] PlatformData getPlatformData() const;

  ~WindowTemplate() override;
};
