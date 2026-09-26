#pragma once
#include "IApplication.hpp"
#include "IFontAtlas.hpp"
#include "IGraphicsDevice.hpp"
#include "ISurface.hpp"
#include "ITexture.hpp"

//  ========================================================
//
//  Render Pass
//
//  ========================================================

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

  ~CommonRenderPass() = default;
};

//  ========================================================
//
//  Font Atals
//
//  ========================================================

struct HashEntry {
  static constexpr size_t HASH_SIZE = 1024;
  std::uint32_t codepoint = 0;
  GlyphUV glyph_table = {};
};

class CommonFontAtlas : public IFontAtlas {
private:
  int ref_count = 0;

protected:
  std::uint8_t *on_demand_bitmap_data = nullptr;
  GlyphUV glyph_table[95] = {};
  HashEntry glyph_hash_table[HashEntry::HASH_SIZE] = {};
  float cell_width = 0.0f;
  float cell_height = 0.0f;
  int cols_per_row = 0;

  int atlas_width = 0;
  int atlas_height = 0;

  IGraphicsDevice *device = nullptr;
  ITexture *texture = nullptr;

  float cursor_x = 0.0f;
  float cursor_y = 0.0f;

  bool rewindCursor();
  size_t hashCodepoint(std::uint32_t) const;
  const HashEntry* findEntry(uint32_t code_point) const;

  CommonFontAtlas(IGraphicsDevice *);

public:
  ITexture* getTexture() override;
  int addRef() override;
  int release() override;
  ~CommonFontAtlas();
};

//  ========================================================
//
//  Texture
//
//  ========================================================

class CommonTexture : public ITexture {
private:
  int ref_count = 0;

protected:
  IGraphicsDevice *device = nullptr;
  TextureFormat format = TextureFormat::Color;
  int width = 0;
  int height = 0;

  CommonTexture(IGraphicsDevice *, int, int, TextureFormat);

public:
  int addRef() override;
  int release() override;

  TextureFormat getFormat() override;

  int getWidth() override;
  int getHeight() override;

  ~CommonTexture();
};

//  ========================================================
//
//  Surface
//
//  ========================================================

class CommonSurface : public ISurface {
private:
  int ref_count = 0;

protected:
  IGraphicsDevice *device = nullptr;
  union {
    IObject *window_or_texture = nullptr;
    // Window 用と Texture 用で実装が分かれるので共用体で OK
    IWindow *window;
    ITexture *texture;
  };

  CommonSurface(IGraphicsDevice *, IObject *);

public:
  int addRef() override;
  int release() override;
  ~CommonSurface();
};

//  ========================================================
//
//  Graphics Device
//
//  ========================================================

class CommonGraphicsDevice : public IGraphicsDevice {
private:
  int ref_count = 0;

protected:
  IApplication *appInstance = nullptr;

public:
  int addRef() override;
  int release() override;
  CommonGraphicsDevice(IApplication *);
  ~CommonGraphicsDevice();
};
