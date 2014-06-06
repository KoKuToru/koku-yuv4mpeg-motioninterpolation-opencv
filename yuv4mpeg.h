#ifndef YUV4MPEG_H
#define YUV4MPEG_H

#include <vector>
#include <iostream>

//Decodes the yuv4mpeg stream
class yuv4mpeg_proxy
{
    private:
        std::vector<unsigned char> frame_packed;
        std::vector<unsigned char> frame;

        int frame_w;
        int frame_h;

        std::istream& input;
        std::ostream& output;

        void next(bool first=false);
        void read();
        void write();

        char ignore;

    public:
        yuv4mpeg_proxy(std::istream& input, std::ostream& output);
        ~yuv4mpeg_proxy();

        int width()  { return frame_w; }
        int height() { return frame_h; }

        //Get the frame data
        const std::vector<unsigned char>& get();
        void set(std::vector<unsigned char> new_frame);
};

#endif
