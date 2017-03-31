#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "opencv2/imgcodecs.hpp"
#include <opencv2/core/utility.hpp>
#include <iostream>
#include <string>
#include <cmath>

using namespace cv;
using namespace std;

#define AREA_TH 500 //minimalna powierzchnia bbox


typedef Vec<uchar,3> vec_uchar_3;

int main( int argc, char** argv )
{


    Mat image;
    Mat model;
    Mat model_bin1;
    Mat img_YCBCR;
    //opozniony obraz
    Mat img_delay;
    //roznica dwoch kolejnych klatek
    Mat diff_img;
    Mat model_YCRCB;
    Mat FDMASK;
    string dir = "D:\\studia\\IV_rok\\sem_8\\PSRA\\3\\";
    //string dir = "/home/lsriw/video_datasets/pets_2006/S1-T1-C/video/pets2006/S1-T1-C/3/";
    string file = "S1-T1-C.";
    string ext = ".jpeg";
    string path;
    int treshold = 150;
    float alpha = 0.984375;

    for(int i = 0; i<3020; i++)
    {
        if(i<10)
            path = dir + file + "0000" + to_string(i) + ext;
        else if(i < 100)
            path = dir + file + "000" + to_string(i) + ext;

        else if(i < 1000)
           path = dir + file + "00" + to_string(i) + ext;
        else
            path = dir + file + "0" + to_string(i) + ext;

        image = imread(path, CV_LOAD_IMAGE_COLOR);   // Read the file
        //cout<<path<<endl;

        if(! image.data )                              // Check for invalid input
        {
            cout <<  "Could not open or find the image" << std::endl ;
            return -1;
        }
        //usleep(1000);
//        namedWindow( "Display window", WINDOW_AUTOSIZE );// Create a window for display.
//        imshow( "Display window", image );


        cvtColor(image, img_YCBCR,  COLOR_RGB2YCrCb);

	    Mat labels(img_YCBCR.size(), CV_8SC3);
	    Mat stats(img_YCBCR.size(), CV_32S);
	    Mat centroid(img_YCBCR.size(), CV_32S);
	    Mat model2(img_YCBCR.size(), CV_32S);

        if(i == 0){
            img_YCBCR.copyTo(model);
            model.copyTo(model_bin1);
            //model_bin1.copyTo(labels);
            model.copyTo(model_YCRCB);
            //img_YCBCR.copyTo(FGMASK);
            img_YCBCR.copyTo(FDMASK);
            //model.convertTo(model_bin1, CV_8SC3);
            img_YCBCR.copyTo(img_delay);
            image.copyTo(diff_img);

        }

        else{
            for(int j = 0; j < img_YCBCR.rows; j++)
                for(int k = 0; k < img_YCBCR.cols; k++)
                {
                    vec_uchar_3 p = img_YCBCR.at<vec_uchar_3>(j,k);
                    //cout << int(p[0]) << " ";
                    float p1 = float(p[0]) * (1 - alpha);
                    float p2 = float(p[1]) * (1 - alpha);
                    float p3 = float(p[2]) * (1 - alpha);

                    vec_uchar_3 m = model_YCRCB.at<vec_uchar_3>(j,k);

                    float m1 = float(m[0]) * alpha;
                    float m2 = float(m[1]) * alpha;
                    float m3 = float(m[2]) * alpha;

                   vec_uchar_3 mn;

                   mn[0] = p1+m1;
                   mn[1] = p2+m2;
                   mn[2] = p3+m3;

					//skadowe piksela z ramki opoznionej
					int pd0 = img_delay.at<vec_uchar_3>(j,k)[0];
					int pd1 = img_delay.at<vec_uchar_3>(j,k)[1];
					int pd2 = img_delay.at<vec_uchar_3>(j,k)[2];

				   //binaryzacja
					//int diff = (abs(mn[0] - p[0]) + abs(mn[1] - p[1]) + abs(mn[2] - p[2]) > treshold) ? 255:0;
					int diff = (1.5 * abs(mn[0] - p[0]) + 2 * (abs(mn[1] - p[1]) + abs(mn[2] - p[2])) > treshold) ? 255:0;

					int diff_delay = (1.5 * abs(p[0] - pd0) + 2 * (abs(p[1] - pd1) + abs(p[2] - pd2)) > treshold) ? 255:0;

					model_bin1.at<vec_uchar_3>(j,k) = vec_uchar_3(diff, diff, diff);
					//FGMASK.at<vec_uchar_3>(j,k) = vec_uchar_3(diff,diff,diff);

					if(diff == 0 && diff_delay == 0)
						model.at<vec_uchar_3>(j,k) = mn;

					//tworzenie roznicy pomiedzy dwoma ramkami
					FDMASK.at<vec_uchar_3>(j,k) = vec_uchar_3(abs(pd0 - p[0]), abs(pd1 - p[1]), abs(pd2 - p[2]));
					img_delay.at<vec_uchar_3>(j,k) = img_YCBCR.at<vec_uchar_3>(j,k);

					}
					vector<Mat> rgbChannels(3);
					split(model_bin1, rgbChannels);
					//cout<< model2.channels()<<endl;

					//int t = connectedComponents(rgbChannels[0], labels, 8);
					int t = connectedComponentsWithStats(rgbChannels[0], labels, stats, centroid,8,CV_32S);

					labels.convertTo(labels, CV_8SC3);
					labels = labels * 10;
//					labels.convertTo(labels, CV_32S);


					if (t > 0) //label nr 0 to tło
						for(int i = 1; i<t; i++){
							//TODO wyswietlenie ramek z labels
							double x = centroid.at<double>(i, 0);
							double y = centroid.at<double>(i, 1);

							int top_y = stats.at<int>(i, CC_STAT_TOP);
							int left_most_x = stats.at<int>(i, CC_STAT_LEFT);
							int x_width = stats.at<int>(i, CC_STAT_WIDTH);
							int y_width = stats.at<int>(i, CC_STAT_HEIGHT);

							//odfiltrowanie najmniejszych box - ów
							if (x_width * y_width > AREA_TH){
								for(int c = left_most_x; c < left_most_x + x_width; c++){
									image.at<vec_uchar_3>(top_y, c) = vec_uchar_3(0,0,255);
									image.at<vec_uchar_3>(top_y + y_width, c) = vec_uchar_3(0,0,255);
								}
								for(int r = top_y; r < top_y + y_width; r++){
									image.at<vec_uchar_3>(r, left_most_x) = vec_uchar_3(0,0,255);
									image.at<vec_uchar_3>(r, left_most_x + x_width) = vec_uchar_3(0,0,255);
								}

							}

						}

        }

//        namedWindow( "Display model", WINDOW_AUTOSIZE );// Create a window for display.
//        imshow( "Display model", model );
////
//        namedWindow( "Display model_bin", WINDOW_AUTOSIZE );// Create a window for display.
//        imshow( "Display model_bin", model_bin1 );
//
//        namedWindow( "Display diff", WINDOW_AUTOSIZE );// Create a window for display.
//        imshow( "Display diff", FDMASK );
//
//        namedWindow( "Display YCRCB", WINDOW_AUTOSIZE );// Create a window for display.
//        imshow( "Display YCRCB", img_YCBCR );

		namedWindow( "Display image", WINDOW_AUTOSIZE );// Create a window for display.
		imshow( "Display YCRCB", image);

        namedWindow( "Display labels", WINDOW_AUTOSIZE );// Create a window for display.

        imshow( "Display labels", labels );

        waitKey(2);



    }


    waitKey(0);                                          // Wait for a keystroke in the window
    return 0;
}
