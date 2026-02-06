#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>


int main(int argc, char ** argv)
{
    #if defined (_WIN32)
        SetConsoleOutputCP(CP_UTF8);
    #endif
    
    setlocale(LC_ALL, "en_US.UTF-8");
    
    return doctest::Context(argc, argv).run();
}


