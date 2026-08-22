#include "CommonApplication.hpp"
#include <cstdio>
#include <cstdlib>

int CommonApplication::addRef(void) { return ++this->ref_count; }

int CommonApplication::release(void) {
  if (--this->ref_count == 0) {
    this->~CommonApplication();
    free(this);
    return 0;
  }
  return this->ref_count;
}

int CommonApplication::startApp(int argc, char **argv) {
  CommonApplication *appInstance = CommonApplication::init();
  if (appInstance == nullptr) {
    std::perror("Failed to initialize appInstance");
    return 1;
  }
  int ret = appMain(argc, argv, appInstance);
  appInstance->release();
  return ret;
}

int main(int argc, char **argv) { return CommonApplication::startApp(argc, argv); }
