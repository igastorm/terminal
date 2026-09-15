#pragma once
#include "../Application/ApplicationTemplate.hpp"
#include "CommonWindow.hpp"

template <class PlatformData, class PlatformApplicationData>
class WindowTemplate : public CommonWindow {
protected:
  PlatformData data;
  ApplicationTemplate<PlatformApplicationData> *appInstance;

  bool setTitle(const char *) override;
  bool show() override;
  bool hide() override;

public:
  [[nodiscard]] PlatformData getPlatformData() const;

  ~WindowTemplate() override;
};
