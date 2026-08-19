#include "IApplication.h"
#include "ImpApplication.h"
#include <cstdio>
#include <cstdlib>
#include <new>

int ImpApplication::addRef(void) { return ++this->ref_count; }

int ImpApplication::release(void) {
  if (--this->ref_count == 0) {
    this->~ImpApplication();
    free(this);
    return 0;
  }
  return this->ref_count;
}

ImpApplication *ImpApplication::Init(void) {
  ImpApplication *app =
      static_cast<ImpApplication *>(std::malloc(sizeof(ImpApplication)));
  if (app == nullptr) {
    std::perror("malloc failed (Application)");
    return nullptr;
  }
  app = new (app) ImpApplication;
  app->addRef();
  // ここに NSApp とかを書く
  // 別で MacInit.mm みたいな感じで作る必要がある
  return app;
}

int main(int argc, char **argv) {
  IApplication *appInstance = ImpApplication::Init();
  if (appInstance == nullptr) {
    return 1;
  }
  int ret = appMain(argc, argv, appInstance);
  appInstance->release();
  return ret;
}
