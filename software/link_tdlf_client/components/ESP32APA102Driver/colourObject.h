#ifndef colourObject_h
#define colourObject_h

struct colourObject
{
	unsigned char** _primaryColours;
    unsigned char _colourTable[3][3];
    short int _modifierTable[2][3];
    unsigned char _colourBlockCount;
    unsigned short int _coloursPerBlock;
    unsigned short int _bandWidth;
    unsigned char _cnt;
    unsigned short int _primColIndex;
};


#endif