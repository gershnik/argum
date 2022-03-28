#ifndef HEADER_TEST_COMMON_H_INCLUDED
#define HEADER_TEST_COMMON_H_INCLUDED

#define ARGUM_INVALID_ARGUMENT(message) reportInvalidArgument(message) 

#include <argum/common.h>

#ifndef ARGUM_NO_THROW
    [[noreturn]] inline void reportInvalidArgument(const char * message) {
        throw invalid_argument(message);
    }
#else
    [[noreturn]] inline void reportInvalidArgument(const char * message) {
        fprintf(stderr, "%s\n", message); 
        std::terminate();
    }
#endif


#if defined(ARGUM_USE_EXPECTED) 
    #define ARGUM_EXPECTED_VALUE(x) (x).value()
#else
    #define ARGUM_EXPECTED_VALUE(x) x
#endif

#endif 