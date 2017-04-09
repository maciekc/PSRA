/*
 * MAT.h
 *
 *  Created on: 8 kwi 2017
 *      Author: Maciek
 */

#ifndef SRC_MAT_H_
#define SRC_MAT_H_

#include "header.h"


template <class T>
class MAT {
public:

	MAT();

	MAT(Mat * img, const int channels = 3);
	MAT(T * img, int c = 720, int r = 576, int channels = 3);
	MAT(int rows, int cols, int channels = 3);

	vector<T> operator()(int a, int b);

	T& operator()(int a, int b, int channel);

	void writeAt(int r, int c, vector<T>  v);

	void writeAt(int a, int b, T v);

	void writeAt(int a, int b, T v1, T v2, T v3);

	void copyTo(MAT<T> * dest);

	int rows() const {
		return r;
	}
	int cols() const {
		return c;
	}
	int size() const {
		return c * r;
	}
	T * img() const {
		return mat;
	}
	virtual ~MAT();

	void split(vector<MAT<T>*> * v);

	void convert2Mat(Mat * img);

private:

	int channels;
	int c;
	int r;
	T * mat;

};

template<class T>
MAT<T>::MAT(){

	channels = 3;
	c = 0;
	r = 0;
	mat = NULL;
}



template<class T>
MAT<T>::MAT(Mat * img, const int ch){

	channels = ch;
	c = img->cols;
	r = img->rows;
	mat = new T[r * c * channels];
	for(int i = 0; i < r; i++){
		for(int j = 0; j < c; j++){
			if (channels == 3){
				Vec<T, 3> p = img->at<Vec<T, 3>>(i,j);
				mat[(i*c + j) * channels] = p[0];
				mat[(i*c + j) * channels + 1] = p[1];
				mat[(i*c + j) * channels + 2] = p[2];
			}
			else{
				Vec<T, 1> p = img->at<Vec<T, 1>>(i,j);
				mat[(i*c + j) * channels] = p[0];
			}
		}
	}
}

template<class T>
MAT<T>::MAT(int rows, int cols, int ch){
	channels = ch;
	r = rows;
	c = cols;
	mat = new T[r * c * channels];

}

template <class T>
MAT<T>::MAT(T * img, int cols, int rows, int ch){

	channels = ch;
	r = rows;
	c = cols;
	mat = img;
}

template<class T>
MAT<T>::~MAT() {
	// TODO Auto-generated destructor stub
	delete [] mat;
}

//----------------------------------------------------------------------------------------------------------------
// przeładowany operatr do pobierania danych z konkretnego piksela

template<class T>
vector<T> MAT<T>::operator()(int a, int b){

	vector<T> p;

	for(int i =0; i< channels; i++)
		p.push_back(mat[(a * c + b) * channels + i]);

	return p;
}

template<class T>
T& MAT<T>::operator()(int a, int b, int d){

	//cout<< "cannels median" << channels << endl;
	return mat[(a * c + b) * channels + d];
}

//-------------------------------------------------------------------------------------------------------------------------
// przeładowane metody do zapisywania danych do pikseli
template<class T>
void MAT<T>::writeAt(int a, int b, vector<T>  v){
	if (channels != v.size()){
		cout<<"zly wymiar !!!" << endl;
		return;
	}
	else{
		for(int i =0; i< v.size(); i++)
			mat[(a * c + b) * channels + i] = (v)[i];
	}
}

template<class T>
void MAT<T>::writeAt(int a, int b, T v){

	for(int i =0; i< channels; i++){
		mat[(a * c + b) * channels + i] = v;
	}
}
template<class T>
void MAT<T>::writeAt(int a, int b, T v1, T v2, T v3){

		mat[(a * c + b) * channels] = v1;
		mat[(a * c + b) * channels + 1] = v2;
		mat[(a * c + b) * channels + 2] = v3;
}

//-----------------------------------------------------------------------------------------------------
// kopiowanie jednej  ramki do drugiej
template<class T>
void MAT<T>::copyTo(MAT<T> * dest){

	dest->c = c;
	dest->r = r;

	if (dest->mat != NULL)
		delete [] dest->mat;

	dest->mat = new T[c * r * channels];

	for(int i = 0; i < r; i++){
		for(int j = 0; j < c; j++){
			for(int k =0; k< channels; k++)
				dest->mat[(i*c + j) * channels + k] = this->mat[(i*c + j) * channels + k];
		}
	}
}

template<class T>
void MAT<T>::split(vector<MAT<T> *> *v){

	int r = this->rows();
	int c = this->cols();

	T * ch1 = new T[r * c];
	T * ch2 = new T[r * c];
	T * ch3 = new T[r * c];

	int j = 0;
	for (int i = 0; i < this->size() * channels; i+=3){
		ch1[j] = this->mat[i];
		ch2[j] = this->mat[i + 1];
		ch3[j] = this->mat[i + 2];
		j++;
	}
	MAT<T> * m1 = new MAT(ch1, 720, 576, 1);

	MAT<T> * m2 = new MAT(ch2, 720, 576, 1);
	MAT<T> * m3 = new MAT(ch3, 720, 576, 1);

	(*v)[0] = m1;
	(*v)[1] = m2;
	(*v)[2] = m3;
}

//------------------------------------------------------------------------------------------------------------------
// konwersja do cv::Mat

template<class T>
void MAT<T>::convert2Mat(Mat * img){

//	Vec<T, 3> p;
	for(int i = 0; i < this->rows(); i++){
		for(int j = 0; j <this->cols(); j++){
			vector<T> a = (*this)(i,j);
			if( this->channels == 3 ){
				Vec<T, 3> p = Vec<T, 3>(a[0], a[1], a[2]);
				img->at<vec_uchar_3>(i,j) = p;
			}
			else{
				Vec<T, 1> p = Vec<T, 1>(a[0]);
				img->at<Vec<T, 1>>(i,j) = p;
			}
		}
	}
}

#endif /* SRC_MAT_H_ */
