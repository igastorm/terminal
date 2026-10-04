#pragma once
#include "IRenderPass.hpp"

//  ========================================================
//
//  Render Pass
//
//  ========================================================

class RenderPass : public IRenderPass {
private:
  int ref_count = 0;

public:
  [[deprecated(
      "Should be used as a temporary object on the stack within `render()`")]]
  int addRef() override;

  [[deprecated(
      "Should be used as a temporary object on the stack within `render()`")]]
  int release() override;

  ~RenderPass() = default;
};