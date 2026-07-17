#ifdef _WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // Forward to regular main
    extern int main(int, char**);
    return main(__argc, __argv);
}
#endif
