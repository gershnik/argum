#include <argum/argum.h>

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"


int main(int argc, char ** argv)
{
    #if defined (_WIN32)
        SetConsoleOutputCP(CP_UTF8);
    #endif

    return Catch::Session().run( argc, argv );
}


