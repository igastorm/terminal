#include "IApplication.hpp"
#include "ImpApplication.hpp"
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

ImpApplication *ImpApplication::init(void) {
  ImpApplication *app =
      static_cast<ImpApplication *>(std::malloc(sizeof(ImpApplication)));
  if (app == nullptr) {
    std::perror("malloc failed (Application)");
    return nullptr;
  }
  app = new (app) ImpApplication;
  app->addRef();
  // プラットフォーム依存部分の初期化
  if (!initPlatform()) {
    std::perror("initPlatform Failed");
    app->release();
    return nullptr;
  }
  return app;
}

int main(int argc, char **argv) {
  IApplication *appInstance = ImpApplication::init();
  if (appInstance == nullptr) {
    std::perror("Failed to initialize appInstance");
    return 1;
  }
  int ret = appMain(argc, argv, appInstance);
  appInstance->release();
  return ret;
}
