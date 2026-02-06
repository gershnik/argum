#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>


int main(int argc, char ** argv)
{
    #if defined (_WIN32)
        SetConsoleOutputCP(CP_UTF8);
        setlocale(LC_ALL, ".UTF-8");
    #else
        setlocale(LC_ALL, "");
    #endif
    
    return doctest::Context(argc, argv).run();
}


