#include "oflow.h"

Mat transformFlow(Mat from, Mat flow, double f) {
    int upscale = 3; //removes integer-steps in warp
    Size original_size = from.size();
    Mat result = Mat::zeros(original_size*upscale, from.type());
    for(int y = 0; y < result.size().height; ++y)
    for(int x = 0; x < result.size().width; ++x) {
        Point2i f_xy = flow.at<Point2f>(y/upscale, x/upscale)*f*upscale;
        f_xy.x = x+f_xy.x;
        f_xy.y = y+f_xy.y;
        //set color of pixel:
        if (f_xy.x > 0 && f_xy.y > 0 && f_xy.x < result.size().width && f_xy.y < result.size().height) {
            result.at<unsigned char>(f_xy.y, f_xy.x) = from.at<unsigned char>(y/upscale, x/upscale);
             if (result.at<unsigned char>(f_xy.y, f_xy.x) == 0) {
                  result.at<unsigned char>(f_xy.y, f_xy.x) = 1;
             }
        }
    }

    //fix image
    bool fixed;
    do {
        Mat tmp = result.clone();
        fixed = true;
        for(int y = 0; y < result.size().height; ++y) {
            for(int x = 0; x < result.size().width; ++x) {
                if (result.at<unsigned char>(y, x) != 0) continue;

                int x_low = x-1;
                int x_high = x+1;
                int y_low = y-1;
                int y_high = y+1;
                if (x_low < 0) x_low = 0;
                if (y_low < 0) y_low = 0;
                if (x_high > result.size().width -1) x_high = result.size().width -1 ;
                if (y_high > result.size().height-1) y_high = result.size().height-1;
                int p[] = {
                    result.at<unsigned char>(y_low , x_low), result.at<unsigned char>(y_low , x), result.at<unsigned char>(y_low , x_high),
                    result.at<unsigned char>(y     , x_low), result.at<unsigned char>(y     , x), result.at<unsigned char>(y     , x_high),
                    result.at<unsigned char>(y_high, x_low), result.at<unsigned char>(y_high, x), result.at<unsigned char>(y_high, x_high)
                };

                int c = 0;
                int res = 0;
                for(int i = 0; i < 9; ++i) {
                    if (p[i] != 0) {
                        c += 1;
                        res += p[i];
                    }
                }

                if (c > 0) {
                    tmp.at<unsigned char>(y, x) = res/c;
                }

                fixed = false;
            }
        }
        result = tmp;
    } while(!fixed);

    //resize
    //cv::GaussianBlur(result, result, Size(0, 0), 1);
    cv::resize(result, result, original_size, 0, 0, INTER_AREA);

    return result;
}

std::tuple<Mat, std::tuple<Mat, Mat>> dualTransformFlow_plusFix(Mat from, Mat to, std::tuple<Mat, Mat> flow, std::tuple<Mat, Mat>* flow_fix, double alpha, double beta, double gamma) {
    //trans normal
    Mat a = transformFlow(from, std::get<0>(flow), alpha);
    Mat b = transformFlow(to  , std::get<1>(flow), beta);

    std::tuple<Mat, Mat> flow_fix_shadow;
    if (flow_fix == nullptr) {
        //optical flow again, move by 50% should perfectly match now !
        //should remove blur
        Mat a_n;
        Mat b_n;
        int scaler = 3;
        cv::resize(a, a_n, Size(a.size().width/scaler, a.size().height/scaler));
        cv::resize(b, b_n, Size(b.size().width/scaler, b.size().height/scaler));
        flow_fix_shadow = dualOpticalFlow(a_n, b_n, nullptr, true);
        std::get<0>(flow_fix_shadow) = scaleOpticalFlow(std::get<0>(flow_fix_shadow), a.size());
        std::get<1>(flow_fix_shadow) = scaleOpticalFlow(std::get<1>(flow_fix_shadow), b.size());
    } else {
         flow_fix_shadow = *flow_fix;
    }

    //trans with fix
    a = transformFlow(a, std::get<0>(flow_fix_shadow), 0.5);
    b = transformFlow(b, std::get<1>(flow_fix_shadow), 0.5);

    //mix
    Mat c;
    addWeighted(a, gamma, b, 1.0-gamma, 0, c);
    return std::make_tuple(b, flow_fix_shadow);
}

Mat dualTransformFlow(Mat from, Mat to, std::tuple<Mat, Mat> flow, double alpha, double beta, double gamma) {
    Mat a = transformFlow(from, std::get<0>(flow), alpha);
    Mat b = transformFlow(to  , std::get<1>(flow), beta);
    Mat c;
    addWeighted(a, gamma, b, 1.0-gamma, 0, c);
    return c;
}

Mat scaleOpticalFlow(Mat flow, Size s) {
    Mat tmp = flow.clone();
    for(int y = 0; y < tmp.size().height; ++y)
    for(int x = 0; x < tmp.size().width; ++x) {
        Point2f f_xy = tmp.at<Point2f>(y, x);
        f_xy.x = f_xy.x*s.width /flow.size().width;
        f_xy.y = f_xy.y*s.height/flow.size().height;
        tmp.at<Point2f>(y, x) = f_xy;
    }
    cv::resize(tmp, tmp, s, 0, 0, INTER_CUBIC);
    return tmp;
}

void blur_xy(Mat &p, float sigma) {
    cv::Mat xy[2];
    cv::split(p, xy);
    GaussianBlur(xy[0], xy[0], Size(0, 0), sigma);
    GaussianBlur(xy[1], xy[1], Size(0, 0), sigma);
    cv::merge(xy, 2, p);
}

Mat opticalFlow(Mat from, Mat to, Mat *prev_flow, bool quick) {
    static Ptr<DenseOpticalFlow> fl = createOptFlow_DualTVL1();
    if (quick) {
        fl->setDouble("tau", 0.25);
        fl->setDouble("lambda", 0.15);
        fl->setDouble("theta", 0.125);
        fl->setInt("nscales", 2);
        fl->setInt("warps",   2);
        fl->setDouble("epsilon", 0.01);
        fl->setInt("iterations", 125);
    } else {
        fl->setDouble("tau", 0.25);
        fl->setDouble("lambda", 0.15);
        fl->setDouble("theta", 0.3);
        fl->setInt("nscales", 5);
        fl->setInt("warps",   5);
        fl->setDouble("epsilon", 0.01);
        fl->setInt("iterations", 300);
    }

    Mat flow;
    if (prev_flow != nullptr) {
        fl->setBool("useInitialFlow", true);
        flow = *prev_flow;
    } else {
        fl->setBool("useInitialFlow", false);
    }

    fl->calc(from, to, flow);
    blur_xy(flow, 3.0);

    return flow;
}

std::tuple<Mat, Mat> dualOpticalFlow(Mat from, Mat to, std::tuple<Mat, Mat>* prev_flow, bool quick) {
    Mat flow_A, flow_B;
    if (prev_flow != nullptr) {
        std::tuple<Mat, Mat> prev = *prev_flow;
        flow_A = opticalFlow(from, to  , &std::get<0>(prev), quick);
        flow_B = opticalFlow(to  , from, &std::get<1>(prev), quick);
    }
    else {
        flow_A = opticalFlow(from, to  , nullptr, quick);
        flow_B = opticalFlow(to  , from, nullptr, quick);
    }
    return std::make_tuple(flow_A, flow_B);
}
