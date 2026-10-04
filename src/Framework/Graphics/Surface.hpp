#pragma once
#include "ISurface.hpp"
#include "IGraphicsDevice.hpp"

//  ========================================================
//
//  Surface
//
//  ========================================================

class Surface : public ISurface {
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

  Surface(IGraphicsDevice *, IObject *);

public:
  int addRef() override;
  int release() override;
  ~Surface();
};