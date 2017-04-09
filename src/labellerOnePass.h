#pragma once

//#include "globals.h"

//#include "xbasic_types.h"
//#include "xil_io.h"

#define MAX_LABELS 16048 // ???
//#define DEBUG

typedef unsigned char u8;

// Objekt odzworujacy to co mozna uzyskac w PL
struct objectPL {
    unsigned int m00;                   //!< pole
	unsigned int m10;                   //!< momnet 10
	unsigned int m01;                   //!< moment 01
	unsigned int ymin;                  //!< wsp. bbox
	unsigned int ymax;                  //!< wsp. bbox
	unsigned int xmin;                  //!< wsp. bbox
	unsigned int xmax;                  //!< wsp. bbox
	unsigned int noOfMovingPixels;      //!< liczba ruchomych pikseli
};

//! Klasa indeksacji jednoprzebiegowej
class labellerOnePass
{
public:
	labellerOnePass(int XX, int YY);            //!< konstruktor
   	~labellerOnePass();                         //!< detruktor

    //! Krok indeksacji
	void step(const u8 *binaryImage, const u8 *moMask);
	//! Pobranie parametrow
	void getParameters(unsigned int &noObjects, objectPL *objects) ;

#ifdef DEBUG
	void getImage(u8 *labelImage);		        //!< tymczasowa funkcja pobrania obrazu etykiet                                                                                        //
#endif

private:

	unsigned int  *mpui_mergeTable;		    //!< tablica sklejen etykiet
	unsigned int **mpui_mergeChain;         //!< stos do obsługi konflikow
	unsigned int   mui_mergeChainIndex;     //!< indeks stosu

	objectPL *mp_objects;                   //!< kontener na obiekty





#ifdef DEBUG
	u8* m_labelImage;                       //!< tymczasowy obraz etykiet
#endif

	unsigned int mui_currentLabel;          //!< biezaca etykieta
	unsigned int *mpui_labelLineBuffer;     //!< bufor linii


	unsigned int mui_XX;                    //! szerokosc obrazu
	unsigned int mui_YY;                    //! wysokosc obrazu

};

