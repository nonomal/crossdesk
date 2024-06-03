#ifdef _WIN32
#ifdef REMOTE_DESK_DEBUG
#pragma comment(linker, "/subsystem:\"console\"")
#else
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif
#endif

#include "log.h"
#include "main_window.h"

int main(int argc, char *argv[]) {
  LOG_INFO("Remote desk");
  MainWindow main_window;

  main_window.Run();

  return 0;
}