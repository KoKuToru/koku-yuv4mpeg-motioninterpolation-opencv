#include "popen_istream.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include "pstream.h"

popen_istream::popen_istream(std::string command, std::string mode): std::istream(new redi::basic_pstreambuf<char>(command.c_str(), std::ios_base::out| std::ios_base::in))  {
}

popen_istream::~popen_istream() {
    if (this->buf != nullptr) {
        delete this->buf;
        this->buf = nullptr;
    }
}

popen_ostream::popen_ostream(std::string command, std::string mode): std::ostream(new redi::basic_pstreambuf<char>(command.c_str(), std::ios_base::out| std::ios_base::in))  {
}

popen_ostream::~popen_ostream() {
    if (this->buf != nullptr) {
        delete this->buf;
        this->buf = nullptr;
    }
}
