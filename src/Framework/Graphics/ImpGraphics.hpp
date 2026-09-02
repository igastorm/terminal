#pragma once
#include "../Application/ImpApplication.hpp"
#include "CommonGraphics.hpp"

template <class PlatformData>
class ImpGraphicsDevice : public CommonGraphicsDevice {
private:
  PlatformData data;

  ISurface *createSurface(int, int) override;
  bool render(ISurface *, RenderCallBack, void *) override;

public:
  template <class PlatformApplicationData>
  static ImpGraphicsDevice *
  createGraphicsDevice(ImpApplication<PlatformApplicationData> *);
  virtual ~ImpGraphicsDevice() override;
};

template <class PlatformData> class ImpSurfaceTemplate : public CommonSurface {
protected:
  PlatformData data;

  bool bindToWindow(IWindow *) override;
  void unbindWindow() override;

  ImpSurfaceTemplate() = default;

public:
  ~ImpSurfaceTemplate() override;
};
