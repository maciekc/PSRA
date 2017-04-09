#include "header.h"
#include "MAT.h"
#include <iostream>

using namespace cv;
using namespace std;

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

inline void median(MAT<uchar> * model, MAT<uchar> * medianModel, int size){

	int threshold = size*size / 2;
	int r = size / 2;
	for(int i = 0; i< model->rows(); i++){
		for(int j = 0; j< model->cols(); j++){
			if((i >= r) && (i < model->rows() - r-1) && (j >= r) && (j < model->cols() - r-1)){

				int suma = 0;
				int piksel = 0;
				for(int k = i - r; k <= i+r; k++)
					for(int t = j -r; t <= j+r; t++){

						if(int((*model)(k, t, 0)) != 0)
							suma += 1;
						if (suma >= threshold){
							piksel = 255;
							break;
						}
					}
				(*medianModel).writeAt(i,j, uchar(piksel));
			}
			else{
				(*medianModel).writeAt(i,j, ((*model)(i,j)));
			}
		}
	}
}

inline void RGB2YCBCR(MAT<uchar> * image, MAT<float> * img_YCbCr, MAT<uchar> * test){
	const float a11 = 0.299;
	const float a12 = 0.587;
	const float a13 = 0.114;
	const float a21 = -0.168736;
	const float a22 = -0.331264;
	const float a23 = 0.5;
	const float a31 = 0.5;
	const float a32 = -0.418688;
	const float a33 = -0.081312;

	vector<uchar> p;
	vector<float> f;

	for(int i = 0; i < image->rows(); i++){
		for(int j = 0; j < image->cols(); j++){
			p = (*image)(i,j);
			float p0 = float(p[0]);
			float p1 = float(p[1]);
			float p2 = float(p[2]);
			f[0] = uchar(a11 * p0 + a12 * p1 + a13 * p2);
			f[2] = uchar(a21 * p0 + a22 * p1 + a23 * p2 + float(128.0));
		    f[1] = uchar(a31 * p0 + a32 * p1 + a33 * p2 + float(128.0));

			//f[1] = (p[0] - f[0])*0.713 + 128;
			//f[2] = (p[2] - f[0])*0.564 + 128;

		    (*img_YCbCr).writeAt(i,j, f);

		    vector<uchar> t = (*test)(i,j);
		}
    }
}

inline int countStaticPixels(MAT<uchar> * model_bin, MAT<uchar> * diff, const int x, const int y, const int width, const int height){
	int result = 0;
	for(int i = x; i < x + height; i++){
		for(int j= y; j < y + width; j++){
			if( ((*model_bin)(i,j))[0] == 255 && ((*diff)(i,j))[0] == 255)
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

	object * newObject = new object;
	newObject->id = id;
	newObject->label_box = labelBox;
	newObject->area = labelBox.width * labelBox.height;
	newObject->visible = visible;
	newObject->counter = counter;
	return newObject;
}

inline int commonArea(const object * a, const object * b){

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

	for(vector<object*>::iterator k = tempStaticObject->begin(); k != tempStaticObject->end(); ++k)
		delete (*k);
	tempStaticObject->clear();
}

inline void removeInvisible(vector<object *> * v){
	vector <object*> n;
	for(int i = 0; i< v->size(); i++){
		if((*v)[i]->visible == false){
			delete (*v)[i];
			(*v).erase((*v).begin() + i);
		}
	}
}

void drawBox(MAT<uchar> * image, const vector<object *> * v){
	//cout<< "funkcja drawbox\n";
	box rec;
	for(int i =0 ;i < v->size(); i++){
		rec = (*v)[i]->label_box;

		int left_most_y = rec.top_y;
		int top_x = rec.top_x;
		int width = rec.width;
		int height = rec.height;

		vector<uchar> color(3);
		if((*v)[i]->counter > NO_MOVEMENT_TH)
			color = {0,0,255};
		else
			color = {255,0,0};
		//odfiltrowanie najmniejszych box - ów

		for(int c = top_x; c < top_x + height; c++){
			(*image).writeAt(c, left_most_y, color);
			(*image).writeAt(c, left_most_y + width, color);
		}
		for(int r = left_most_y; r < left_most_y + width; r++){
			(*image).writeAt(top_x, r, color);
			(*image).writeAt(top_x + height, r, color);
		}
	}
}

//------------------------------------------------------------------------------------------------------------------
//
int main( int argc, char** argv )
{

    Mat image0;
    MAT<uchar> *model = new MAT<uchar>();

    MAT<uchar> *model_bin1 = new MAT<uchar>();
    //opozniony obraz
    MAT<uchar> *img_delay = new MAT<uchar>();
    //roznica dwoch kolejnych klatek
    MAT<uchar> *diff_img = new MAT<uchar>();
    MAT<uchar> *model_YCRCB = new MAT<uchar>();
    MAT<uchar> *FDMASK = new MAT<uchar>();
    string dir = "D:\\studia\\IV_rok\\sem_8\\PSRA\\3\\";
    //string dir = "/home/lsriw/video_datasets/pets_2006/S1-T1-C/video/pets2006/S1-T1-C/3/";
    string file = "S1-T1-C.";
    string ext = ".jpeg";
    string path;

	vector<object *> staticObject;
	vector<object *> tempStaticObject;

//
////        namedWindow( "Display model", WINDOW_AUTOSIZE );// Create a window for display.
//    namedWindow( "Display model_bin", WINDOW_AUTOSIZE );// Create a window for display.
////        namedWindow( "Display diff", WINDOW_AUTOSIZE );// Create a window for display.
////        namedWindow( "Display YCRCB", WINDOW_AUTOSIZE );// Create a window for display.
//		namedWindow( "Display YCBCR", WINDOW_AUTOSIZE );// Create a window for display.
//        namedWindow( "Display image", WINDOW_AUTOSIZE );// Create a window for display.


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

        image0 = imread(path, CV_LOAD_IMAGE_COLOR);   // Read the file
        //cout<<path<<endl;

        if(! image0.data )                              // Check for invalid input
        {
            cout <<  "Could not open or find the image" << std::endl ;
            return -1;
        }

        Mat img_YCBCR0(image0.size(), CV_8SC3);

        cvtColor(image0, img_YCBCR0,  COLOR_RGB2YCrCb);

        MAT<uchar> *image = new MAT<uchar>(&image0, 3);
        MAT<uchar> *img_YCBCR = new MAT<uchar>(&img_YCBCR0, 3);
        cout<<"tu0 " << i << "\n"<<endl;


        //--------------------------------------------------------------------------------------------------------
        // konwersja RGB -> YCBCR
        //RGB2YCBCR(&image, &img_YCBCR, &img_YCBCR2);

		MAT<uchar> *medianModel = new MAT<uchar>(img_YCBCR->rows(), img_YCBCR->cols(), 1);
		cout<<"tu 2\n"<<endl;
	    Mat labels(img_YCBCR0.size(), CV_8SC3);
	    Mat stats(img_YCBCR0.size(), CV_32S);
	    Mat centroid(img_YCBCR0.size(), CV_32S);
	    Mat model2(img_YCBCR0.size(), CV_32S);

        if(i == START_FRAME){
        	img_YCBCR->copyTo(model);
            model->copyTo(model_bin1);
            model->copyTo(model_YCRCB);
            img_YCBCR->copyTo(FDMASK);
            img_YCBCR->copyTo(img_delay);
            image->copyTo(diff_img);
        }

        else{
            for(int j = 0; j < img_YCBCR->rows(); j++){
                for(int k = 0; k < img_YCBCR->cols(); k++){
                    vector<uchar> p = (*img_YCBCR)(j,k); // piksel obecnej ramki w ycbcr

                    vector<uchar> m = (*model_YCRCB)(j,k); //piksel obecnego modelu

					//skadowe piksela z ramki opoznionej
					int pd0 = (*img_delay)(j,k)[0];
					int pd1 = (*img_delay)(j,k)[1];
					int pd2 = (*img_delay)(j,k)[2];

					//-------------------------------------------------------------------------------------------------------
				   //binaryzacja
					int diff = (1.5 * abs(m[0] - p[0]) + 2 * (abs(m[1] - p[1]) + abs(m[2] - p[2])) > TH_BIN) ? 255:0;

					//int diff_delay = (1.5 * abs(p[0] - pd0) + 2 * (abs(p[1] - pd1) + abs(p[2] - pd2)) > TH_BIN) ? 255:0;

					int diff_delay = ((p[0] - pd0) > DIFF_TH) ? 255:0;
					(*model_bin1).writeAt(j,k, diff);
					//--------------------------------------------------------------------------------------------------------
					// aktualizacja modelu tła
					if(diff == 0 && diff_delay == 0){
	                    float p1 = float(p[0]) * (1 - alpha);
	                    float p2 = float(p[1]) * (1 - alpha);
	                    float p3 = float(p[2]) * (1 - alpha);

	                    float m1 = float(m[0]) * alpha;
	                    float m2 = float(m[1]) * alpha;
	                    float m3 = float(m[2]) * alpha;

	                    (*model).writeAt(j,k, p1+m1, p2+m2, p3+m3);
					}

					//-------------------------------------------------------------------------------------------------------
					//tworzenie roznicy pomiedzy dwoma ramkami
					(*FDMASK).writeAt(j,k, uchar(diff_delay));
					(*img_delay).writeAt(j,k, (*img_YCBCR)(j,k));
                }
            }

            vector<MAT<uchar> *> rgbChannels(3);
            //split(model_bin1, rgbChannels);
            (*model_bin1).split(&rgbChannels);

            median((rgbChannels[0]), medianModel, 5);

            Mat medianModel_Mat(medianModel->rows(), medianModel->cols(), CV_8SC1);

            (*medianModel).convert2Mat(&medianModel_Mat);

            cout << medianModel_Mat.channels() << " " << medianModel_Mat.rows << endl;
            int t = connectedComponentsWithStats(medianModel_Mat, labels, stats, centroid,8,CV_32S);

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

                        //policzenie statycznych pikseli
                        int staticPixels = countStaticPixels(model_bin1, FDMASK, top_x, left_most_y, width, height);

                        if((float(staticPixels)/float(width*height)) < MOVIN_PIXELS_TH){
                            object * newObject = createObject(0, true, 0, top_x, left_most_y, width, height);
                            tempStaticObject.push_back(newObject);
                        }
                    }
                    //FDmask - moMask - roznica pomiedzy maskami
                    //model_bin1 - fgMask - model po binaryzacji
                }
            }

            delete rgbChannels[0];
			delete rgbChannels[1];
			delete rgbChannels[2];
			rgbChannels.clear();
        }

        for(int ii = 0; ii < staticObject.size(); ii++)
        	staticObject[ii]->visible = false;

        //przypisanie nowych obiektów do juz istniejacych lub stworzenie nowych obiektow statycznych
        assignToObject(&staticObject, &tempStaticObject);

        //usuniece niewidoczych obiektów
        removeInvisible(&staticObject);

        drawBox(image, &staticObject);

        Mat model_bin_show(model_bin1->rows(), model_bin1->cols(), CV_8SC3);
        Mat img_YCBCR_show(img_YCBCR->rows(), img_YCBCR->cols(), CV_8SC3);
        Mat image_show(image->rows(), image->cols(), CV_8SC3);


        (*model_bin1).convert2Mat(&model_bin_show);

        (*img_YCBCR).convert2Mat(&img_YCBCR_show);
        (*image).convert2Mat(&image_show);

//        imshow( "Display model", model );
        imshow( "Display model_bin", model_bin_show );
//        imshow( "Display diff", FDMASK );
//        imshow( "Display YCRCB", img_YCBCR );
		imshow( "Display YCRCB", img_YCBCR_show);
        imshow( "Display image", image_show );


        //-------------------------------------------------------------------------------------------------------------------------------
        //
        //usuwanie
        delete image;
        delete medianModel;
        delete img_YCBCR;


        waitKey(2);
    }

    delete model;
	delete model_bin1;
	delete model_YCRCB;
	delete img_delay;
	delete diff_img;
	delete FDMASK;

    waitKey(0);                                          // Wait for a keystroke in the window
    return 0;
}
