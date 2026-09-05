#pragma once
#include "../Application/ImpApplication.hpp"
#include "CommonGraphics.hpp"

template <class PlatformData, class PlatformApplicationData>
class ImpSurfaceTemplate : public CommonSurface {
protected:
  PlatformData data;
  // Surface の所有者は Device だからより上位の appInstance を参照する必要はない
  // ImpApplication<PlatformApplicationData> *appInstance;

  bool bindToWindow(IWindow *) override;
  void unbindWindow() override;

  ImpSurfaceTemplate() = default;

public:
  ~ImpSurfaceTemplate() override;
};

template <class PlatformData, class PlatformApplicationData>
class ImpGraphicsDevice : public CommonGraphicsDevice {
private:
  PlatformData data;
  ImpApplication<PlatformApplicationData> *appInstance;

  ISurface *createSurface(int, int) override;
  bool render(ISurface *, RenderCallBack, void *) override;

public:
  static ImpGraphicsDevice *
  createGraphicsDevice(ImpApplication<PlatformApplicationData> *);
  virtual ~ImpGraphicsDevice() override;
  
  void getPlatformData(const PlatformData **) const;
};
