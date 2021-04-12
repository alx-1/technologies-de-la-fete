#include "colourObject.h"
#include <stdlib.h>
#include <stdio.h> 
#include <math.h>

void initSimpleColourObject(struct colourObject *thisObject, unsigned char maxValue)
{
  thisObject->_colourBlockCount = 6;
  thisObject->_primaryColours = (unsigned char **)malloc(thisObject->_colourBlockCount*sizeof(unsigned char*));
  for(thisObject->_cnt=0; thisObject->_cnt<thisObject->_colourBlockCount; thisObject->_cnt++)
  {
	thisObject->_primaryColours[thisObject->_cnt] = (unsigned char *)malloc(3*sizeof(unsigned char));
  }
  for(thisObject->_cnt=0; thisObject->_cnt<3; thisObject->_cnt++)
  {
    thisObject->_colourTable[thisObject->_cnt][0] = 0;
    thisObject->_colourTable[thisObject->_cnt][1] = 0;
    thisObject->_colourTable[thisObject->_cnt][2] = 0;
  }
  for(thisObject->_cnt=0; thisObject->_cnt<2; thisObject->_cnt++)
  {
    thisObject->_modifierTable[thisObject->_cnt][0] = 0;
    thisObject->_modifierTable[thisObject->_cnt][1] = 0;
    thisObject->_modifierTable[thisObject->_cnt][2] = 0;
  }    
  thisObject->_coloursPerBlock = maxValue;
  thisObject->_bandWidth = thisObject->_coloursPerBlock * thisObject->_colourBlockCount;
  //Fill out default primary colours
  //[[255,0,0],[255,255,0],[0,255,0],[0,255,255],[0,0,255],[255,0,255]]
  thisObject->_primaryColours[0][0] = maxValue;
  thisObject->_primaryColours[0][1] = 0;
  thisObject->_primaryColours[0][2] = 0;
  
  thisObject->_primaryColours[1][0] = maxValue;
  thisObject->_primaryColours[1][1] = maxValue;
  thisObject->_primaryColours[1][2] = 0;
  
  thisObject->_primaryColours[2][0] = 0;
  thisObject->_primaryColours[2][1] = maxValue;
  thisObject->_primaryColours[2][2] = 0;
  
  thisObject->_primaryColours[3][0] = 0;
  thisObject->_primaryColours[3][1] = maxValue;
  thisObject->_primaryColours[3][2] = maxValue;
  
  thisObject->_primaryColours[4][0] = 0;
  thisObject->_primaryColours[4][1] = 0;
  thisObject->_primaryColours[4][2] = maxValue;
  
  thisObject-> _primaryColours[5][0] = maxValue;
  thisObject->_primaryColours[5][1] = 0;
  thisObject->_primaryColours[5][2] = maxValue;
}

void initComplexColourObject(struct colourObject *thisObject, unsigned char maxValue, unsigned char colourBlockCount, unsigned char *rgbColourArray)
{
  thisObject->_colourBlockCount = colourBlockCount;
  thisObject->_primaryColours = (unsigned char**)malloc(thisObject->_colourBlockCount*sizeof(unsigned char*));
  for(thisObject->_cnt=0; thisObject->_cnt<thisObject->_colourBlockCount; thisObject->_cnt++)
  {
	thisObject->_primaryColours[thisObject->_cnt] = (unsigned char*)malloc(3*sizeof(unsigned char));
  }
  //Init the Colour Table
  for(thisObject->_cnt=0; thisObject->_cnt<3; thisObject->_cnt++)
  {
    thisObject->_colourTable[thisObject->_cnt][0] = 0;
    thisObject->_colourTable[thisObject->_cnt][1] = 0;
    thisObject->_colourTable[thisObject->_cnt][2] = 0;
  }
  //Init the Modofier Table
  for(thisObject->_cnt=0; thisObject->_cnt<2; thisObject->_cnt++)
  {
    thisObject->_modifierTable[thisObject->_cnt][0] = 0;
    thisObject->_modifierTable[thisObject->_cnt][1] = 0;
    thisObject->_modifierTable[thisObject->_cnt][2] = 0;
  }
  //number of colours between 1 block and its neighbour    
  thisObject->_coloursPerBlock = maxValue;
  //Total number of colours in this spectrum
  thisObject->_bandWidth = thisObject->_coloursPerBlock * thisObject->_colourBlockCount;
  //Fill out primary colours based on array passed
  for(thisObject->_cnt=0; thisObject->_cnt<colourBlockCount; thisObject->_cnt++)
  {
    thisObject->_primaryColours[thisObject->_cnt][0] = rgbColourArray[thisObject->_cnt*3];
    thisObject->_primaryColours[thisObject->_cnt][1] = rgbColourArray[(thisObject->_cnt*3)+1];
    thisObject->_primaryColours[thisObject->_cnt][2] = rgbColourArray[(thisObject->_cnt*3)+2];
  }
}

void gradientGenerator(struct colourObject *thisObject, unsigned short int colourIndex, unsigned short int bandWidth)
{
  for(thisObject->_cnt=0; thisObject->_cnt<3; thisObject->_cnt++)
  {
    //fill modifier
    if(thisObject->_colourTable[1][thisObject->_cnt]>thisObject->_colourTable[0][thisObject->_cnt]) { thisObject->_modifierTable[0][thisObject->_cnt]=1; }
    else if(thisObject->_colourTable[1][thisObject->_cnt]<thisObject->_colourTable[0][thisObject->_cnt]) { thisObject->_modifierTable[0][thisObject->_cnt]=-1; }
    else if(thisObject->_colourTable[1][thisObject->_cnt]==thisObject->_colourTable[0][thisObject->_cnt]) { thisObject->_modifierTable[0][thisObject->_cnt]=0; }

    //fill step value
    if(thisObject->_modifierTable[0][thisObject->_cnt]==1)
    {
      thisObject->_modifierTable[1][thisObject->_cnt] = thisObject->_colourTable[1][thisObject->_cnt] - thisObject->_colourTable[0][thisObject->_cnt];
    }
    else if(thisObject->_modifierTable[0][thisObject->_cnt]==-1)
    {
      thisObject->_modifierTable[1][thisObject->_cnt] = thisObject->_colourTable[0][thisObject->_cnt] - thisObject->_colourTable[1][thisObject->_cnt];
    }
    else if(thisObject->_modifierTable[0][thisObject->_cnt]==0)
    {
      thisObject->_modifierTable[1][thisObject->_cnt] = 0;
    }
    //calculate current gradient between 2 based on the percentile index
    thisObject->_colourTable[2][thisObject->_cnt] = thisObject->_colourTable[0][thisObject->_cnt] + (((float)thisObject->_modifierTable[1][thisObject->_cnt]*((float)colourIndex/(float)bandWidth))*thisObject->_modifierTable[0][thisObject->_cnt]);
  }
}

void getColour(struct colourObject *thisObject, short int colourIndex, unsigned char *colourBlock)
{
  thisObject->_primColIndex = colourIndex/thisObject->_coloursPerBlock ;
  thisObject->_colourTable[0][0] = thisObject->_primaryColours[thisObject->_primColIndex][0]; 
  thisObject->_colourTable[0][1] = thisObject->_primaryColours[thisObject->_primColIndex][1];
  thisObject->_colourTable[0][2] = thisObject->_primaryColours[thisObject->_primColIndex][2];
  
  thisObject->_colourTable[1][0] = thisObject->_primaryColours[(thisObject->_primColIndex+1)%thisObject->_colourBlockCount][0]; 
  thisObject->_colourTable[1][1] = thisObject->_primaryColours[(thisObject->_primColIndex+1)%thisObject->_colourBlockCount][1]; 
  thisObject->_colourTable[1][2] = thisObject->_primaryColours[(thisObject->_primColIndex+1)%thisObject->_colourBlockCount][2];

  gradientGenerator(thisObject, colourIndex%thisObject->_coloursPerBlock, thisObject->_coloursPerBlock);
  
  colourBlock[0] = thisObject->_colourTable[2][0];
  colourBlock[1] = thisObject->_colourTable[2][1];
  colourBlock[2] = thisObject->_colourTable[2][2];
}
