#ifndef ARGUM_NO_THROW
    //for testing let it throw exception rather than crash
    [[noreturn]] void reportInvalidArgument(const char * message);
    #define ARGUM_INVALID_ARGUMENT(message) reportInvalidArgument(message) 
#endif
