#include "CommonApplication.hpp"
#include <cstdio>
#include <cstdlib>

int CommonApplication::addRefBase() { return ++this->ref_count; }

int CommonApplication::release() {
  if (--this->ref_count == 0) {
    this->~CommonApplication();
    free(this);
    return 0;
  }
  return this->ref_count;
}

int CommonApplication::startApp(int argc, char **argv) {
  CommonApplication *appInstance = createPlatformApplication();
  if (appInstance == nullptr) {
    std::perror("Failed to initialize appInstance");
    return 1;
  }
  int ret = appMain(argc, argv, appInstance);
  appInstance->release();
  return ret;
}

int main(int argc, char **argv) { return CommonApplication::startApp(argc, argv); }
