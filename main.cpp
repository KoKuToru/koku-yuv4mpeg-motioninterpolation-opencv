#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include "popen_istream.h"
#include "yuv4mpeg.h"
#include "oflow.h"

using namespace std;

void process(istream &input, ostream &output) {
    int f = 0;
    bool first = true;
    yuv4mpeg_proxy proxy(input, output);
    Mat mat_y[3];
    Mat mat_u[3];
    Mat mat_v[3];
    mat_y[0] = Mat::zeros(proxy.height(), proxy.width(), CV_8U);
    mat_u[0] = Mat::zeros(proxy.height(), proxy.width(), CV_8U);
    mat_v[0] = Mat::zeros(proxy.height(), proxy.width(), CV_8U);
    mat_y[1] = Mat::zeros(proxy.height(), proxy.width(), CV_8U);
    mat_u[1] = Mat::zeros(proxy.height(), proxy.width(), CV_8U);
    mat_v[1] = Mat::zeros(proxy.height(), proxy.width(), CV_8U);
    mat_y[2] = Mat::zeros(proxy.height(), proxy.width(), CV_8U);
    mat_u[2] = Mat::zeros(proxy.height(), proxy.width(), CV_8U);
    mat_v[2] = Mat::zeros(proxy.height(), proxy.width(), CV_8U);
    while(input.good() && output.good()) {
        int f_prev = (f+1)%2; //which is also the next to write...
        int f_now  = f;
        f = (f+1)%2;

        std::vector<unsigned char> frame;
        //for(int i = 0; i < 10; ++i) //skip
            frame = proxy.get();

        //copy frame into opencv..
        for(int y = 0, i = 0; y < proxy.height(); ++y)
        for(int x = 0; x < proxy.width(); ++x, i+=3) {
            mat_y[f_now].at<unsigned char>(y, x) = frame[i+0];
            mat_u[f_now].at<unsigned char>(y, x) = frame[i+1];
            mat_v[f_now].at<unsigned char>(y, x) = frame[i+2];
        }

        if (!first) {
            static int frame_nr = 0;
#define USE_FLOW_FIX
#define RESIZE
#ifdef RESIZE
            Mat mat_y_a_small;
            Mat mat_y_b_small;
#ifndef USE_FLOW_FIX
            int scaler = 3;
#else
            int scaler = 5;//8
#endif
            cv::resize(mat_y[f_prev], mat_y_a_small, Size(proxy.width()/scaler, proxy.height()/scaler));
            cv::resize(mat_y[f_now ], mat_y_b_small, Size(proxy.width()/scaler, proxy.height()/scaler));
            auto flow = dualOpticalFlow(mat_y_a_small, mat_y_b_small);
            std::get<0>(flow) = scaleOpticalFlow(std::get<0>(flow), mat_y[f_prev].size());
            std::get<1>(flow) = scaleOpticalFlow(std::get<1>(flow), mat_y[f_now ].size());
            //blur  ?
            blur_xy(std::get<0>(flow), scaler*2);
            blur_xy(std::get<1>(flow), scaler*2);
#else
            auto flow = dualOpticalFlow(mat_y[f_prev], mat_y[f_now]);
#endif

            //write first frame
            for(int y = 0, i = 0; y < proxy.height(); ++y)
            for(int x = 0; x < proxy.width(); ++x, i+=3) {
                frame[i+0] = mat_y[f_prev].at<unsigned char>(y, x);
                frame[i+1] = mat_u[f_prev].at<unsigned char>(y, x);
                frame[i+2] = mat_v[f_prev].at<unsigned char>(y, x);
            }
            proxy.set(frame);

            //write 50% interpolation
            int max_j = 2;
            for(int j = 1; j<max_j; ++j) {
                double f = j/double(max_j);

#ifdef USE_FLOW_FIX
                std::tuple<Mat, Mat> fixed_flow;
                std::tie(mat_y[2], fixed_flow) = dualTransformFlow_plusFix(mat_y[f_prev], mat_y[f_now], flow, nullptr    , f, 1.0-f, 1.0-f);
                std::tie(mat_u[2], fixed_flow) = dualTransformFlow_plusFix(mat_u[f_prev], mat_u[f_now], flow, &fixed_flow, f, 1.0-f, 1.0-f);
                std::tie(mat_v[2], fixed_flow) = dualTransformFlow_plusFix(mat_v[f_prev], mat_v[f_now], flow, &fixed_flow, f, 1.0-f, 1.0-f);
#else
                mat_y[2] = dualTransformFlow(mat_y[f_prev], mat_y[f_now], flow, f, 1.0-f, 1.0-f);
                mat_u[2] = dualTransformFlow(mat_u[f_prev], mat_u[f_now], flow, f, 1.0-f, 1.0-f);
                mat_v[2] = dualTransformFlow(mat_v[f_prev], mat_v[f_now], flow, f, 1.0-f, 1.0-f);
#endif

                for(int y = 0, i = 0; y < proxy.height(); ++y)
                    for(int x = 0; x < proxy.width(); ++x, i+=3) {
                        frame[i+0] = mat_y[2].at<unsigned char>(y, x);
                        frame[i+1] = mat_u[2].at<unsigned char>(y, x);
                        frame[i+2] = mat_v[2].at<unsigned char>(y, x);
                    }
                proxy.set(frame);
            }
        } else {
            first = false;
        }
    }
}

//no idea why the standart doesn't have such function
bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t last_pos = 0;
    do {
        size_t start_pos = str.find(from, last_pos);
        if(start_pos == std::string::npos)
            return false;
        str.replace(start_pos, from.length(), to);
        last_pos = start_pos + to.size();
    } while(true);
    return true;
}

int main(int argc_, char** argv_) {
    //convert C to C++ (why is this not in the standart ?)
    vector<string> arg(argc_);
    for(int i = 0; i < arg.size(); ++i) {
        arg[i] = argv_[i];
    }
    /* CMD PARSE */
    bool noaudio = find(arg.begin(), arg.end(), "-noaudio") != arg.end();
    string input;
    bool usecin  = [&]() {
        auto it = find(arg.begin(), arg.end(), "-i");
        if (it == arg.end()) {
            return true;
        }
        ++it;
        if (it == arg.end()) {
            return false;
        }
        if (*it == "-") {
            return true;
        }
        input = *it;
        return false;
    }();
    string output;
    bool useffplay = false;
    bool usecout = [&]() {
        auto it = find(arg.begin(), arg.end(), "-o");
        if (it == arg.end()) {
            return true;
        }
        ++it;
        if (it == arg.end()) {
            return false;
        }
        if (*it == "-") {
            return true;
        }
        if (*it == "!") {
            useffplay = true;
            return false;
        }
        input = *it;
        return false;
    }();
    bool showhelp = (arg.size() == 1) || (find(arg.begin(), arg.end(), "-h") != arg.end()) || (find(arg.begin(), arg.end(), "-help") != arg.end());
    /* CMD PARSE END */

    if (showhelp) {
        cerr << "Parameters: " << endl;
        cerr << "    -i <file>    If <file> is \"-\" it will read from stdin" << endl;
        cerr << "                 else it will use mplayer to encode video" << endl;
        cerr << "    -o <file>    If <file> is \"-\" it will write to stdout" << endl;
        cerr << "                 If <file> is \"!\" it will use ffplay to display video" << endl;
        cerr << "                 else it will write raw yuv4mpeg to <file>" << endl;
        cerr << "    -noaudio     If mplayer is used for playback, dont play sound" << endl;
        cerr << "                 This will playback/convert video as fast as possible" << endl;
        cerr << "    -help        Shows this message" << endl;
        return -1;
    }

    //escape input file name
    replace(input, " ", "\\ ");

    if (usecin && usecout) {
        //read from stdin
        //write to stdout
        process(cin, cout);
    } else if (usecin && useffplay) {
        //read from stdin
        //write to ffplay
        popen_ostream out("ffplay -loglevel panic -i -", "w");
        process(cin, out);
    } else if (usecin) {
        //read from stdin
        //write to file
        ofstream out(output, ios::binary|ios::out);
        process(cin, out);
    } else if (usecout) {
        //read from file (using mplayer)
        //write to stdout
        if (noaudio) {
            popen_istream in("mplayer "+input+" -ss 00:30:00 -ass -really-quiet -nosound -benchmark -vo yuv4mpeg:file=-","r");
            process(in, cout);
        } else {
            popen_istream in("mplayer "+input+" -ss 00:30:00 -ass -really-quiet -vo yuv4mpeg:file=-","r"); //maybe don't allow framedrop
            process(in, cout);
        }
    } else if (useffplay) {
        //read from file
        //write to ffplay
        popen_ostream out("ffplay -loglevel panic -i -", "w");
        //ofstream out("/media/RAID/test.yuv");
        if (noaudio) {
            popen_istream in("mplayer "+input+" -ss 00:30:00 -ass -really-quiet -nosound -benchmark -vo yuv4mpeg:file=-","r");
            process(in, out);
        } else {
            popen_istream in("mplayer "+input+" -ss 00:30:00 -ass -really-quiet -vo yuv4mpeg:file=-","r"); //maybe don't allow framedrop
            process(in, out);
        }
    }
}
