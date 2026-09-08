#pragma once
#include "IGraphicsDevice.hpp"
#include "ISurface.hpp"
#include "ITexture.hpp"

enum class BindObject { none, window, texture };

class CommonRenderPass : public IRenderPass {
private:
  int ref_count = 0;

public:
  [[deprecated(
      "Should be used as a temporary object on the stack within `render()`")]]
  int addRef() override;

  [[deprecated(
      "Should be used as a temporary object on the stack within `render()`")]]
  int release() override;

  virtual ~CommonRenderPass() = default;
};

class CommonTexture : public ITexture {
private:
  int ref_count = 0;

public:
  int addRef() override;
  int release() override;
  virtual ~CommonTexture() = default;
};

class CommonSurface : public ISurface {
private:
  int ref_count = 0;

protected:
  BindObject bind_flag = BindObject::none;

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
