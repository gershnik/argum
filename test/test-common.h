#ifndef ARGUM_NO_THROW
    //for testing let it throw exception rather than crash
    [[noreturn]] void reportInvalidArgument(const char * message);
    #define ARGUM_INVALID_ARGUMENT(message) reportInvalidArgument(message) 
#endif


#if defined(ARGUM_USE_EXPECTED) || defined(ARGUM_NO_THROW)
    #define ARGUM_EXPECTED_VALUE(x) (x).value()
#else
    #define ARGUM_EXPECTED_VALUE(x) x
#endif