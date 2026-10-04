#pragma once
#include "../CommonGraphics.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

struct MacBitmapData {
  CGContextRef cg_context = nullptr;
};

class MacBitmap : public CommonBitmap {
private:
  MacBitmapData data = {};
  MacBitmap(CGContextRef, void *, std::size_t, std::size_t);

public:
  CGContextRef getCGContext() const;
  static MacBitmap *createMacBitmap(std::size_t, std::size_t);
  MacBitmap() = delete;
  ~MacBitmap();
};
