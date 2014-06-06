#include "yuv4mpeg.h"
#include <stdexcept>

using namespace std;

yuv4mpeg_proxy::yuv4mpeg_proxy(istream &input, ostream &output):
    input(input), output(output)
{
    //Check for yuv4mpeg_proxy
    string data;
    input >> data;
    output << data << " ";

    std::cerr << data << "<read" << endl;
    if (data != "YUV4MPEG2") {
        throw std::runtime_error("yuv4mpeg_proxy Not a stream");
    }

    char c;
    input >> c;
    output << c;
    if (c != 'W') {
        throw std::runtime_error("yuv4mpeg_proxy File is wrong ! - Waiting for W");
    }
    input >> frame_w;
    output << frame_w << " ";

    input >> c;
    output << c;
    if (c != 'H') {
        throw std::runtime_error("yuv4mpeg_proxy File is wrong ! - Waiting for H");
        return;
    }
    input >> frame_h;
    output << frame_h << " ";

    frame_packed.resize(frame_w*frame_h*3/2);
    frame.resize(frame_w*frame_h*3);
    next(true);
}

void yuv4mpeg_proxy::next(bool first) {
    /*
      This way of reading a yuv4mpeg_proxy might be very easy..
      But is hardly correct, pretty much only works with mplayer..
      In the future I might update this to decode a real yuv4mpeg_proxy stream
     */

    //wait for first FRAME
    string dummy;
    do
    {
        input >> dummy;
        if (first) {
            if (dummy != "FRAME") {
                output << dummy << " ";
            } else {
                output << "XKoKuToru" << "\n";
            }
        }
        if (!input.good()) {
            return;
        }
    } while(dummy != "FRAME");
    //probably the "zero ? null-terminated"
    input.read(&ignore, 1); //first byte is nothing..
    if (!input.good()) {
        return;
    }
}

void yuv4mpeg_proxy::read() {
    if (!input.good()) {
        return;
    }

    //read the frame
    input.read(reinterpret_cast<char*>(frame_packed.data()), frame_packed.size());

    //unpack the frame
    const unsigned char* data_y = frame_packed.data();
    const unsigned char* data_u = frame_packed.data()+frame_w*frame_h;
    const unsigned char* data_v = frame_packed.data()+frame_w*frame_h+frame_w/2*frame_h/2;
    for(int y = 0; y < frame_h; ++y)
    for(int x = 0; x < frame_w; ++x) {
        frame[(x+y*frame_w)*3+0] = data_y[x+y*frame_w]; //Y
        frame[(x+y*frame_w)*3+1] = data_u[x/2+y/2*frame_w/2]; //U
        frame[(x+y*frame_w)*3+2] = data_v[x/2+y/2*frame_w/2]; //V
    }

    next();
}

void yuv4mpeg_proxy::write()  {
    //pack the frame
    unsigned char* data_y = frame_packed.data();
    unsigned char* data_u = frame_packed.data()+frame_w*frame_h;
    unsigned char* data_v = frame_packed.data()+frame_w*frame_h+frame_w/2*frame_h/2;
    for(int y = 0; y < frame_h; ++y)
    for(int x = 0; x < frame_w; ++x) {
        data_y[x+y*frame_w]       = frame[(x+y*frame_w)*3+0]; //Y
        data_u[x/2+y/2*frame_w/2] = frame[(x+y*frame_w)*3+1]; //U
        data_v[x/2+y/2*frame_w/2] = frame[(x+y*frame_w)*3+2]; //V
    }

    output << "FRAME";
    output.write(&ignore, 1); //nextline !
    output.write(reinterpret_cast<char*>(frame_packed.data()), frame_packed.size());
}

const vector<unsigned char>& yuv4mpeg_proxy::get() {
    read();
    return frame;
}
void yuv4mpeg_proxy::set(std::vector<unsigned char> new_frame) {
    frame = new_frame;
    write();
}

yuv4mpeg_proxy::~yuv4mpeg_proxy() {
}
