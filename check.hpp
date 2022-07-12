#ifndef CHECK_HPP
#define CHECK_HPP 1

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

namespace do_not_use_this {
    inline void error(const char *file, int line) {
        auto tmp = errno;//fprintf may fail, so we preserve errno
        fprintf(stderr, "%s (line %d) :", file, line);
        errno = tmp;
        perror(nullptr);
        exit(EXIT_FAILURE);
    }

    inline int check(int p, const char *file, int line) {
        if (p < 0) error(file, line);
        return p;
    }

    template<typename T>
    inline T *check(T *p, const char *file, int line) {
        if (p == nullptr) error(file, line);
        return p;
    }

    bool in_range(int value){
        return false;
    }

    bool in_range(int value, int val1){
        return  value == val1;
    }

    template<typename... Ts>
    bool in_range(int value, int val1, Ts... vals){
        return in_range(value, val1) || in_range(value, vals...);
    }

    template<typename... TErrors>
    inline int check_except(int p, const char *file, int line, TErrors... allowed_errors){
        if(p < 0 && !in_range(errno, allowed_errors...))
            return check(p, file, line);
        return p;
    }

}

//USE ONLY THIS MACRO
//Example: int fd = check(open("file", O_CREAT|O_RDWR, S_IRWXU));
#define check(x) do_not_use_this::check(x, __FILE__, __LINE__ )

#define check_except(x, ...) do_not_use_this::check_except(x, __FILE__, __LINE__, __VA_ARGS__ )

//https://en.cppreference.com/w/cpp/preprocessor/replace#Predefined_macros

// if you wish to avoid macro (set C++20 in CMakeLists or in Project settings in VS)
//https://en.cppreference.com/w/cpp/utility/source_location

#endif // !CHECK_HPP