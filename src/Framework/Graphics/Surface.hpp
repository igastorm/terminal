#pragma once
#include "../Object/Object.hpp"
#include "IGraphicsDevice.hpp"
#include "ISurface.hpp"

//  ========================================================
//
//  Surface
//
//  ========================================================

class Surface : public Object<ISurface> {
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
  int release() override;
  ~Surface();
};
