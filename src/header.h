#ifndef header_cpp
#define header_cpp

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "opencv2/imgcodecs.hpp"
#include <opencv2/core/utility.hpp>
#include <iostream>
#include <string>
#include <cmath>
#include <vector>

using namespace cv;
using namespace std;

#define AREA_TH 400 //minimalna powierzchnia bbox - CHANGE
#define START_FRAME 0 // ramka startowa
#define TH_BIN 150// prog binaryzacji
#define DIFF_TH 30
#define MOVIN_PIXELS_TH 0.01     //CHANGE
#define NO_MOVEMENT_TH 20 // liczba ramek po uplywie ktorych obiekt jest klasyfikowany jako bagaz

typedef Vec<uchar,3> vec_uchar_3;
typedef Vec<float, 3> vec_float_3;

#endif
