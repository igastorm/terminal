#pragma once
#include "../Application/ImpApplication.hpp"
#include "CommonWindow.hpp"

template <class PlatformData, class PlatformApplicationData>
class ImpWindowTemplate : public CommonWindow {
protected:
  PlatformData data;
  ImpApplicationTemplate<PlatformApplicationData> *appInstance;

  bool setTitle(const char *) override;
  bool show() override;
  bool hide() override;

public:
  [[nodiscard]] PlatformData getPlatformData() const;

  ~ImpWindowTemplate() override;
};
