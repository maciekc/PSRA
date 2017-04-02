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
#define START_FRAME 0 // ramka startowa
#define TH_BIN 150// prog binaryzacji
#define DIFF_TH 30
#define MOVIN_PIXELS_TH 0.01
#define NO_MOVEMENT_TH 20 // liczba ramek po uplywie ktorych obiekt jest klasyfikowany jako bagaz

typedef Vec<uchar,3> vec_uchar_3;
typedef Vec<float, 3> vec_float_3;

typedef struct{
	int width;
	int height;
	//wsp lewego gornego rogu
	int top_x;
	int top_y;
}box;
typedef struct object{
	static int staticObjects;
	int id;
	box label_box;
	int area;
	bool visible;
	int counter;
}object;

int object::staticObjects = 1;
//---------------------------------------------------------------------------------------------------------------
//deklaracja wlasnych funkcji

inline void median(Mat * model, Mat * medianModel, int size){
	//cout<< "funkcja median\n";
	int threshold = size*size / 2;
	int r = size / 2;
	for(int i = 0; i< model->rows; i++){
		for(int j = 0; j< model->cols; j++){
			if((i >= r) && (i < model->rows - r-1) && (j >= r) && (j < model->cols - r-1)){
				int suma = 0;
				int piksel = 0;
				for(int k = i - r; k <= i+r; k++)
					for(int t = j -r; t <= j+r; t++){
						if(int(model->at<uchar>(k,t)) != 0)
							suma += 1;
						if (suma >= threshold){
							piksel = 255;
							break;
						}
					}
				medianModel->at<uchar>(i,j) = uchar(piksel);
			}
			else
				medianModel->at<uchar>(i,j) = model->at<uchar>(i,j);
			//cout << i << " " << j << " " << int(medianModel->at<uchar>(i,j)) << endl;
		}
	}
}

inline void RGB2YCBCR(const Mat * image, Mat * img_YCbCr, Mat * test){
	const float a11 = 0.299;
	const float a12 = 0.587;
	const float a13 = 0.114;
	const float a21 = -0.168736;
	const float a22 = -0.331264;
	const float a23 = 0.5;
	const float a31 = 0.5;
	const float a32 = -0.418688;
	const float a33 = -0.081312;

	vec_uchar_3 p;
	vec_uchar_3 f;

	for(int i = 0; i < image->rows; i++){
		for(int j = 0; j < image->cols; j++){
			p = image->at<vec_uchar_3>(i,j);
			float p0 = float(p[0]);
			float p1 = float(p[1]);
			float p2 = float(p[2]);
			f[0] = uchar(a11 * p0 + a12 * p1 + a13 * p2);
			f[2] = uchar(a21 * p0 + a22 * p1 + a23 * p2 + float(128.0));
		    f[1] = uchar(a31 * p0 + a32 * p1 + a33 * p2 + float(128.0));

			//f[1] = (p[0] - f[0])*0.713 + 128;
			//f[2] = (p[2] - f[0])*0.564 + 128;

		    img_YCbCr->at<vec_uchar_3>(i,j) = f;

		    vec_uchar_3 t = test->at<vec_uchar_3>(i,j);
		}
    }
}

inline int countStaticPixels(const Mat * model_bin, const Mat * diff, const int x, const int y, const int width, const int height){
	int result = 0;
	for(int i = x; i < x + height; i++){
		for(int j= y; j < y + width; j++){
			if(model_bin->at<uchar>(i,j) == 255 && diff->at<uchar>(i,j) == 255)
				result++;
		}
	}
	return result;
}

inline object * createObject(int id, bool visible, int counter, int x, int y, int width, int height){

	box labelBox;
	labelBox.top_x = x;
	labelBox.top_y = y;
	labelBox.width = width;
	labelBox.height = height;

	object * newObject = new object;
	newObject->id = id;
	newObject->label_box = labelBox;
	newObject->area = labelBox.width * labelBox.height;
	newObject->visible = visible;
	newObject->counter = counter;
	return newObject;
}

inline object * createObject(int id, bool visible, int counter, box labelBox){

	//cout << "create " << labelBox.top_x << " " << labelBox.top_y << endl;

	object * newObject = new object;
	newObject->id = id;
	newObject->label_box = labelBox;
	newObject->area = labelBox.width * labelBox.height;
	newObject->visible = visible;
	newObject->counter = counter;
	return newObject;
}

inline int commonArea(const object * a, const object * b){
	//cout<< "funkcja commonarea\n";
	int result = 0;
	int w, h;
	if(((b->label_box.top_x < a->label_box.top_x)&&(b->label_box.top_x + b->label_box.height > a->label_box.top_x)) || ((a->label_box.top_x < b->label_box.top_x)&&(a->label_box.top_x + a->label_box.height > b->label_box.top_x))){
		if(((b->label_box.top_y < a->label_box.top_y)&&(b->label_box.top_y + b->label_box.width > a->label_box.top_y)) || (((a->label_box.top_y < b->label_box.top_y))&&(a->label_box.top_y + a->label_box.width > b->label_box.top_y))){
			if(a->label_box.top_x > b->label_box.top_x)
				w = b->label_box.top_x + b->label_box.height - a->label_box.top_x;
			else
				w = a->label_box.top_x + a->label_box.height - b->label_box.top_x;

			if(a->label_box.top_y > b->label_box.top_y)
				h = b->label_box.top_y + b->label_box.width - a->label_box.top_y;
			else
				h = a->label_box.top_y + a->label_box.width - b->label_box.top_y;

			result = w*h;
		}
    }
	return result;
}

void assignToObject(vector<object *> *staticObject, vector<object *> * tempStaticObject){
	//cout<< "funkcja assign\n";
	for(int i = 0; i< tempStaticObject->size(); i++){
		int best = -1;
		int maxIntersection = 0;

		for(int j = 0; j<staticObject->size(); j++){
			int common_Area = commonArea((*staticObject)[j],(*tempStaticObject)[i]);

			float intersectionRatio = float(common_Area) / float((*staticObject)[j]->area);

			float areaRatio = float((*tempStaticObject)[i]->area) / float((*staticObject)[j]->area);

			if(maxIntersection < intersectionRatio && areaRatio > 0.8 && areaRatio < 1.2){
				maxIntersection = intersectionRatio;
				best = j;
			}
		}

		//jesli nie przypisano zadnego to mamy nowy obiekt statyczny
		if (best == -1){
			object *newStaticObject = createObject(object::staticObjects, false, 0, ((*tempStaticObject)[i]->label_box));
			object::staticObjects += 1;
			staticObject->push_back(newStaticObject);
		}
		//jesli nie wykryto nowego obiektu to aktualizacja tego ktory pasuje najbardziej
		else{
			(*staticObject)[best]->visible = true;
			(*staticObject)[best]->area = (*tempStaticObject)[i]->area;
			(*staticObject)[best]->label_box = (*tempStaticObject)[i]->label_box;
			(*staticObject)[best]->counter += 1;
		}
	}

	//wykasowanie wektora z tymczasowymi elemetami
//	for(int i = 0; i< tempStaticObject->size(); i++)
//		delete (*tempStaticObject)[i];
//
	for(vector<object*>::iterator k = tempStaticObject->begin(); k != tempStaticObject->end(); ++k)
		delete (*k);
	tempStaticObject->clear();
}

inline void removeInvisible(vector<object *> * v){
	//cout<< "funkcja removeInvisible\n";
	vector <object*> n;
	for(int i = 0; i< v->size(); i++){
		if((*v)[i]->visible == false){
			delete (*v)[i];
			(*v).erase((*v).begin() + i);
		}
		//if((*v)[i]->visible == true)
		//	n.push_back((*v)[i]);
		//else
		//	delete (*v)[i];
	}
//	v->clear();
//	delete v;
//	//vector<object *> * v = new vector<object *>;
//	v = &n;
}

void drawBox(Mat * image, const vector<object *> * v){
	//cout<< "funkcja drawbox\n";
	box rec;
	for(int i =0 ;i < v->size(); i++){
		rec = (*v)[i]->label_box;

		int left_most_y = rec.top_y;
		int top_x = rec.top_x;
		int width = rec.width;
		int height = rec.height;

		vec_uchar_3 color;
		if((*v)[i]->counter > NO_MOVEMENT_TH)
			color = vec_uchar_3(0,0,255);
		else
			color = vec_uchar_3(255,0,0);
		//odfiltrowanie najmniejszych box - ów

		for(int c = top_x; c < top_x + height; c++){
			image->at<vec_uchar_3>(c, left_most_y) = color;
			image->at<vec_uchar_3>(c, left_most_y + width) = color;
		}
		for(int r = left_most_y; r < left_most_y + width; r++){
			image->at<vec_uchar_3>(top_x, r) = color;
			image->at<vec_uchar_3>(top_x + height, r) = color;
		}
//        namedWindow( "Display image", WINDOW_AUTOSIZE );// Create a window for display.
//
//        imshow( "Display image", *image );
	}
}

//------------------------------------------------------------------------------------------------------------------
//
int main( int argc, char** argv )
{
    Mat image;
    Mat model;
    Mat model_bin1;
    //Mat img_YCBCR;
    //opozniony obraz
    Mat img_delay;
    //roznica dwoch kolejnych klatek
    Mat diff_img;
    Mat model_YCRCB;
    Mat FDMASK;
    string dir = "3/"; // "D:\\studia\\IV_rok\\sem_8\\PSRA\\3\\";
    //string dir = "/home/lsriw/video_datasets/pets_2006/S1-T1-C/video/pets2006/S1-T1-C/3/";
    string file = "S1-T1-C.";
    string ext = ".jpeg";
    string path;

	vector<object *> staticObject;
	vector<object *> tempStaticObject;

//        namedWindow( "Display model", WINDOW_AUTOSIZE );// Create a window for display.
    namedWindow( "Display model_bin", WINDOW_AUTOSIZE );// Create a window for display.
//        namedWindow( "Display diff", WINDOW_AUTOSIZE );// Create a window for display.
//        namedWindow( "Display YCRCB", WINDOW_AUTOSIZE );// Create a window for display.
		namedWindow( "Display YCBCR", WINDOW_AUTOSIZE );// Create a window for display.
        namedWindow( "Display image", WINDOW_AUTOSIZE );// Create a window for display.

	// stringstream ss; //MOCK

	object::staticObjects = 1;

    float alpha = 0.984375;

    for(int i = START_FRAME; i<3020; i++){
//        ss.seekp(0); //MOCK
//        ss << i; //MOCK
//
//        if(i<10) path = dir + file + "0000" + ss.str() + ext; //MOCK
//        else if(i<100) path = dir + file + "000" + ss.str() + ext; //MOCK
//        else if(i<1000) path = dir + file + "00" + ss.str() + ext; //MOCK
//        else path = dir + file + "0" + ss.str() + ext; //MOCK

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

        //Mat img_YCBCR2;
        Mat img_YCBCR(image.size(), CV_8SC3);

        cvtColor(image, img_YCBCR,  COLOR_RGB2YCrCb);

        //--------------------------------------------------------------------------------------------------------
        // konwersja RGB -> YCBCR
        //RGB2YCBCR(&image, &img_YCBCR, &img_YCBCR2);

		Mat medianModel(img_YCBCR.size(), CV_8SC1);

		//cout<< int(img_YCBCR.at<vec_uchar_3>(100,100)[0])<<" "<< int(img_YCBCR.at<vec_uchar_3>(100,100)[0])<<" "<< int(img_YCBCR.at<vec_uchar_3>(100,100)[0])<<endl;
		//cout<< int(img_YCBCR2.at<vec_uchar_3>(100,100)[0])<<" "<< int(img_YCBCR2.at<vec_uchar_3>(100,100)[0])<<" "<< int(img_YCBCR2.at<vec_uchar_3>(100,100)[0])<<endl;

	    Mat labels(img_YCBCR.size(), CV_8SC3);
	    Mat stats(img_YCBCR.size(), CV_32S);
	    Mat centroid(img_YCBCR.size(), CV_32S);
	    Mat model2(img_YCBCR.size(), CV_32S);

        if(i == START_FRAME){
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
            for(int j = 0; j < img_YCBCR.rows; j++){
                for(int k = 0; k < img_YCBCR.cols; k++){
                    vec_uchar_3 p = img_YCBCR.at<vec_uchar_3>(j,k); // piksel obecnej ramki w ycbcr

                    vec_uchar_3 m = model_YCRCB.at<vec_uchar_3>(j,k); //piksel obecnego modelu

					//skadowe piksela z ramki opoznionej
					int pd0 = img_delay.at<vec_uchar_3>(j,k)[0];
					int pd1 = img_delay.at<vec_uchar_3>(j,k)[1];
					int pd2 = img_delay.at<vec_uchar_3>(j,k)[2];

					//-------------------------------------------------------------------------------------------------------
				   //binaryzacja
					int diff = (1.5 * abs(m[0] - p[0]) + 2 * (abs(m[1] - p[1]) + abs(m[2] - p[2])) > TH_BIN) ? 255:0;

					//int diff_delay = (1.5 * abs(p[0] - pd0) + 2 * (abs(p[1] - pd1) + abs(p[2] - pd2)) > TH_BIN) ? 255:0;

					int diff_delay = ((p[0] - pd0) > DIFF_TH) ? 255:0;
					model_bin1.at<vec_uchar_3>(j,k) = vec_uchar_3(diff, diff, diff);

					//--------------------------------------------------------------------------------------------------------
					// aktualizacja modelu tła
					if(diff == 0 && diff_delay == 0){
	                    float p1 = float(p[0]) * (1 - alpha);
	                    float p2 = float(p[1]) * (1 - alpha);
	                    float p3 = float(p[2]) * (1 - alpha);

	                    float m1 = float(m[0]) * alpha;
	                    float m2 = float(m[1]) * alpha;
	                    float m3 = float(m[2]) * alpha;

	                    model.at<vec_uchar_3>(j,k) = vec_uchar_3(p1+m1,p2+m2,p3+m3);
					}

					//-------------------------------------------------------------------------------------------------------
					//tworzenie roznicy pomiedzy dwoma ramkami
					//FDMASK.at<vec_uchar_3>(j,k) = vec_uchar_3(abs(pd0 - p[0]), abs(pd1 - p[1]), abs(pd2 - p[2]));
					FDMASK.at<vec_uchar_3>(j,k) = vec_uchar_3(diff_delay,diff_delay,diff_delay);
					img_delay.at<vec_uchar_3>(j,k) = img_YCBCR.at<vec_uchar_3>(j,k);
                }
            }

            vector<Mat> rgbChannels(3);
            split(model_bin1, rgbChannels);

            median(&(rgbChannels[0]), &medianModel, 5);
            //medianBlur(rgbChannels[0], medianModel, 5);
            int t = connectedComponentsWithStats(medianModel, labels, stats, centroid,8,CV_32S);

            labels.convertTo(labels, CV_8SC3);
            labels = labels * 10;

            if (t > 0){ //label nr 0 to tło
                for(int i = 1; i<t; i++){
                    //TODO wyswietlenie ramek z labels
                    double x = centroid.at<double>(i, 0);
                    double y = centroid.at<double>(i, 1);

                    int top_x = stats.at<int>(i, CC_STAT_TOP);
                    int left_most_y = stats.at<int>(i, CC_STAT_LEFT);
                    int height = stats.at<int>(i, CC_STAT_HEIGHT);
                    int width = stats.at<int>(i, CC_STAT_WIDTH);

                    //odfiltrowanie najmniejszych box - ów
                    if (width * height > AREA_TH){
//								for(int c = left_most_x; c < left_most_x + x_width; c++){
//									image.at<vec_uchar_3>(top_y, c) = vec_uchar_3(0,0,255);
//									image.at<vec_uchar_3>(top_y + y_width, c) = vec_uchar_3(0,0,255);
//								}
//								for(int r = top_y; r < top_y + y_width; r++){
//									image.at<vec_uchar_3>(r, left_most_x) = vec_uchar_3(0,0,255);
//									image.at<vec_uchar_3>(r, left_most_x + x_width) = vec_uchar_3(0,0,255);
//								}

                        //policzenie statycznych pikseli
                        int staticPixels = countStaticPixels(&model_bin1, &FDMASK, top_x, left_most_y, width, height);

                        if((float(staticPixels)/float(width*height)) < MOVIN_PIXELS_TH){
                            object * newObject = createObject(0, true, 0, top_x, left_most_y, width, height);
                            tempStaticObject.push_back(newObject);
                        }
                    }
                    //FDmask - moMask - roznica pomiedzy maskami
                    //model_bin1 - fgMask - model po binaryzacji
                }
            }
        }

        for(int ii = 0; ii < staticObject.size(); ii++)
        	staticObject[ii]->visible = false;

        //przypisanie nowych obiektów do juz istniejacych lub stworzenie nowych obiektow statycznych
        assignToObject(&staticObject, &tempStaticObject);

        //usuniece niewidoczych obiektów
        removeInvisible(&staticObject);

        drawBox(&image, &staticObject);

//        imshow( "Display model", model );
        imshow( "Display model_bin", model_bin1 );
//        imshow( "Display diff", FDMASK );
//        imshow( "Display YCRCB", img_YCBCR );
		imshow( "Display YCRCB", img_YCBCR);
        imshow( "Display image", image );

        waitKey(2);
    }
    waitKey(0);                                          // Wait for a keystroke in the window
    return 0;
}
