#ifndef HEADER_TEST_COMMON_H_INCLUDED
#define HEADER_TEST_COMMON_H_INCLUDED

#define ARGUM_CUSTOM_TERMINATE

#include <argum/common.h>

#ifndef ARGUM_NO_THROW

    #include <stdexcept>

    [[noreturn]] inline void Argum::terminateApplication(const char * message) {
        throw std::invalid_argument(message);
    }
#else
    
    [[noreturn]] inline void Argum::terminateApplication(const char * message) {
        #ifndef NDEBUG
            assert(message && false);
            abort();
        #else
            fprintf(stderr, "%s\n", message); 
            fflush(stderr); 
            std::terminate(); 
        #endif
    }
#endif


#if defined(ARGUM_USE_EXPECTED) 
    #define ARGUM_EXPECTED_VALUE(x) (x).value()
#else
    #define ARGUM_EXPECTED_VALUE(x) x
#endif

#endif 