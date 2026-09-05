#pragma once
#include "IGraphicsDevice.hpp"
#include "ISurface.hpp"

//class CommonRenderPass : public IRenderPass {
//  private:
//    int ref_count = 0;
//
//  public:
//    int addRef() override;
//    int release() override;
//    virtual ~CommonRenderPass();
//};

class CommonSurface : public ISurface {
private:
  int ref_count = 0;

public:
  int addRef() override;
  int release() override;
  virtual ~CommonSurface() = default;
};

class CommonGraphicsDevice : public IGraphicsDevice {
private:
  int ref_count = 0;

public:
  int addRef() override;
  int release() override;
  virtual ~CommonGraphicsDevice() = default;
};
