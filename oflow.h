#ifndef OFLOW_H
#define OFLOW_H

#include <opencv2/opencv.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <string>
#include <stdexcept>
#include <string.h>

using namespace std;
using namespace cv;

Mat transformFlow(Mat from, Mat flow, double f=1.0);
Mat dualTransformFlow(Mat from, Mat to, std::tuple<Mat, Mat> flow, double alpha=0.0, double beta=1.0, double gamma=1.0);
Mat scaleOpticalFlow(Mat flow, Size s);
Mat opticalFlow(Mat from, Mat to, Mat *prev_flow = nullptr, bool quick = false);
std::tuple<Mat, Mat> dualOpticalFlow(Mat from, Mat to, std::tuple<Mat, Mat>* prev_flow=nullptr, bool quick = false);
std::tuple<Mat, std::tuple<Mat, Mat>> dualTransformFlow_plusFix(Mat from, Mat to, std::tuple<Mat, Mat> flow, std::tuple<Mat, Mat>* flow_fix = nullptr, double alpha=0.0, double beta=1.0, double gamma=1.0);
void blur_xy(Mat &p, float sigma);

#endif // OFLOW_H
