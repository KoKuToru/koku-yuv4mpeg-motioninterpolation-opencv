#ifndef POPEN_ISTREAM_H
#define POPEN_ISTREAM_H

#include <istream>

class popen_istream: public std::istream
{
    private:
        std::streambuf* buf;
    public:
        popen_istream(std::string command, std::string mode);
        ~popen_istream();

        popen_istream(const popen_istream& other) = delete;
        void operator=(const popen_istream& other) = delete;
};

class popen_ostream: public std::ostream
{
    private:
        std::streambuf* buf;
    public:
        popen_ostream(std::string command, std::string mode);
        ~popen_ostream();

        popen_ostream(const popen_ostream& other) = delete;
        void operator=(const popen_ostream& other) = delete;
};

#endif // POPEN_ISTREAM_H
