#ifndef _AH_EXTRA_H
#define _AH_EXTRA_H

#define BLACK "\x1B[30;1m"
#define RED "\x1B[31;1m"
#define GREEN "\x1B[32;1m"
#define YELLOW "\x1B[33;1m"
#define BLUE "\x1B[34;1m"
#define MAGENTA "\x1B[35;1m"
#define CYAN "\x1B[36;1m"
#define WHITE "\x1B[37;1m"
#define RESET "\x1B[0m"

#define AH_ENUM_LIST(_) _,
#define AH_STRING_LIST(_) #_,

#define AH_DEFINE_ASSOCIATIVE_ENUM(enum_name, array_name, def_name) \
    enum enum_name { def_name(AH_ENUM_LIST) }; \
    static const char *array_name[] = { def_name(AH_STRING_LIST) };

#define AH_STRINGIZE_(_) #_
#define AH_STRINGIZE(_) AH_STRINGIZE_(_)

#ifdef AH_DEBUG
#define AH_PRINT(...) printf(__VA_ARGS__)
#define AH_PRINTX(...) printf(BLUE "[" __FILE__ ":" AH_STRINGIZE(__LINE__) "] " RESET __VA_ARGS__)
#else
#define AH_PRINT(...)
#define AH_PRINTX(...)
#endif

#endif
