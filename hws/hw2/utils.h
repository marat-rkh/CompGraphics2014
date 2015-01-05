#ifndef UTILS_H
#define UTILS_H

#include <exception>
#include <string>

class msg_exception : public std::exception {
    std::string what_;
public:
    msg_exception(std::string const& msg): what_(msg) {}

    const char* what() const throw() { return what_.c_str(); }
};

#endif // UTILS_H
