// Adapted from:
// Code Example for jolliFactory's Bi-color 16X16 LED Matrix Conway's Game of Life example 1.0
// and Adafruit NeoPixel Example 'simple'

#ifdef __AVR__
  #include <avr/power.h>
#endif

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#include <SPI.h>

#define Width  8
#define Height 8

#define PIN    4
#define PINB   LED_BUILTIN

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      (Width * Height)

// options
uint16_t DELAY          = 80;   // Delay between cycles (ms)
uint16_t MAXGENERATIONS = 100;    // Maximum number of cycles allowed before restarting
uint16_t MAXBLANK       = 5;     // Maximum number of blank cycles before restarting
uint16_t MAXSTATIC      = 5;     // Maximum number of cycles that are exactly the same before restarting

bool ADJBRIGHT          = false;    // Adjust Brightness based on neighbor count
bool ADJCOLOR           = true;   // Adjust Color based on neighbor count
uint16_t colorAdjust    = 30;     // if ADJCOLOR degrees of color rotation per neighbor
uint16_t maxBrightness  = 30;     // max brightness
uint16_t brightAdjust   = 5;     // if ADJCOLOR, asjust by this amount log
bool printmap           = false;   // if true print matrix map to serial

Adafruit_NeoMatrix pixels = Adafruit_NeoMatrix(4, 4, 2, 2, PIN,
  NEO_TILE_TOP   + NEO_TILE_LEFT   + NEO_TILE_ROWS   + NEO_TILE_PROGRESSIVE +
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
  NEO_GRB + NEO_KHZ800);

void hsv2rgb(unsigned int hue, unsigned int sat, unsigned int val, \
unsigned char * r, unsigned char * g, unsigned char * b, unsigned char maxBrightness );

// vars
unsigned int hue;
int idx            = 0;
int noOfGeneration = 0;
int iCount         = 0;
int allOffCount    = 0;   // tracking how many cycles all pixels are off
int sameCount      = 0;   // tracking how many cycles all pixels the same as the prior cycle
byte sameBuffer[NUMPIXELS];  

byte t1[16][16];   // main matrix                   
byte t2[16][16];   // next matrix
byte last[16][16]; // previous matrix

//**********************************************************************************************************************************************************
void setup() {
  Serial.begin(115200);
  Serial.println("starting....");
  delay(1000);
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code

  pixels.begin(); // This initializes the NeoPixel library.
  
  // initialize digital pin 13 as an output.
  pinMode(PINB, OUTPUT);

  // testmatrix();
  // randomize(t1);

  // cleart(t1);
  blinker(t1);
  // glider(t1);
  display(t1);
  delay(DELAY);
}

void glider(byte t1[16][16]){
  // Gliderpattern
  t1[1][0] = 1;
  t1[2][1] = 1;
  t1[0][2] = 1;
  t1[1][2] = 1;
  t1[2][2] = 1;
}

void pentomino(byte t1[16][16]){
  // R-pentomino
  t1[2][1] = 1;
  t1[3][1] = 1;
  t1[1][2] = 1;
  t1[2][2] = 1;
  t1[2][3] = 1;
}

// oscillators
void blinker(byte t1[16][16]){
  // @todo bytes index!!!!
  t1[1][0] = 1;
  t1[1][1] = 1;
  t1[1][2] = 1;
}

void toad(byte t1[16][16]){

}

void beacon(byte t1[16][16]){

}

// stills
void block(byte t1[16][16]){

}
void beehive(byte t1[16][16]){

}
void loaf(byte t1[16][16]){

}
void boat(byte t1[16][16]){

}
void tub(byte t1[16][16]){

}

void testmatrix(){
  pixels.clear();
  pixels.setPixelColor(0,pixels.Color(128,0,0));
  pixels.show();
  delay(500);
  for(int i=0; i < 64; i++){
    int x = i % Width;
    int y = (i - x) / Width;
    pixels.drawPixel( x,y, pixels.Color(128,0,128));
    // pixels.setPixelColor(i,pixels.Color(128,0,128));
    pixels.show();
    delay(100);
  }
  delay(1000);
}

//**********************************************************************************************************************************************************
void loop()
{

  static bool onoff = false;
  
  if (idx++ > MAXGENERATIONS || allOffCount > MAXBLANK || sameCount > MAXSTATIC)   //limit no. of generations for display
  {
    allOffCount    = 0;
    sameCount      = 0;
    hue            = random(360); // @todo custom start hue, maybe replace hue and conversion with colorwheel ?
    randomize(t1);   
    noOfGeneration = 0;
    idx=0;
  }

  #ifdef PINB
    // generation indicator
    onoff = !onoff;
    digitalWrite(PINB, onoff?HIGH:LOW);
  #endif

  compute_previous_generation(t1,t2);
  compute_neighbouring_cells(t1,t2);
  compute_next_generation(t1,t2);
  display(t1);
  delay(DELAY);
}

uint16_t getBrightness(uint16_t count){
  uint16_t bright = brightAdjust << count; // 2^nCount makes the scaling logrithmic, so the eye can see the differences
  if (bright > 255) bright = 255;
  return bright;
  // @todo nCount > 4 is > 255
  // log gamma stepping 4-7 steps ?
  // if (bright < 5) bright = 5; // @todo clamp min brightness in hsvrgb ?
}

//**********************************************************************************************************************************************************
void clear_window()
{
  pixels.clear();
  pixels.show();
}


//**********************************************************************************************************************************************************
void display(byte t1[16][16])
{
  byte i,j;

  int offCount = 0;
  byte newBuffer[NUMPIXELS];
  uint32_t c;

  // rows
  for(i=0; i<(Height); i++)
  {
    // columns
    for(j=0; j<Width; j++)
    {  
      uint32_t pixel = i*Width+j;
      // uint32_t pixel = j*height+i;
      byte nCount = countNeighbors(t1,i,j);

      Serial.print(pixel);
      if (t1[i][j])
      {
        newBuffer[pixel] = 1;
        byte r, g, b;
        unsigned int bright = maxBrightness;
        unsigned int xhue = hue;
        if(ADJBRIGHT){
          bright = getBrightness(nCount);
        }
        if(ADJCOLOR){
          xhue += nCount * colorAdjust; // degrees of color rotation per neighbor
          if (xhue >= 360) xhue -= 360;
        }
        hsv2rgb(xhue,255,255,&r,&g,&b,bright);
        c = pixels.Color(r,g,b);
        if(printmap) Serial.print("1 ");
        else Serial.println(": 1");
      }
      else
      {
        c = pixels.Color(0,0,0); // off color
        newBuffer[pixel] = 0;
        offCount++;
        if(printmap) Serial.print("0 ");
        else Serial.println(": 0");
      }
      pixels.drawPixel(j,i,c);
    }
    if(printmap) Serial.println();
  }
  pixels.show();
  if (offCount == NUMPIXELS) allOffCount++;
  if (memcmp(newBuffer,sameBuffer,NUMPIXELS) == 0) sameCount++;
  memcpy(sameBuffer,newBuffer,NUMPIXELS);
  
}

//**********************************************************************************************************************************************************
void compute_previous_generation(byte t1[16][16],byte t2[16][16])
{
  byte i,j;

  for(i=0;i<Height;i++)
  {
    for(j=0;j<Width;j++)
    {
      t2[i][j]=t1[i][j];
      last[i][j]=t1[i][j];
    }
  }
}



//**********************************************************************************************************************************************************
void compute_next_generation(byte t1[16][16],byte t2[16][16])
{
  byte i,j;

  for(i=0;i<Height;i++)
  {
    for(j=0;j<Width;j++)
    {
      t1[i][j]=t2[i][j];
    }
  }
  
  noOfGeneration++;
  Serial.print("Gen: ");
  Serial.println(noOfGeneration);
}



//**********************************************************************************************************************************************************
void compute_neighbouring_cells(byte t1[16][16],byte t2[16][16])   //To Re-visit - does not seems correct
{
  byte i,j,a;

  for(i=0;i<Height;i++)
  {
    for(j=0;j<Width;j++)
    {
      a = countNeighbors(t1,i,j);
      
      if((t1[i][j]==0)&&(a==3)){t2[i][j]=1;}                   // populate if 3 neighours around it
      if((t1[i][j]==1)&&((a==2)||(a==3))){t2[i][j]=1;}         // stay alive if 2 or 3 neigbours around it
      if((t1[i][j]==1)&&((a==1)||(a==0)||(a>3))){t2[i][j]=0;}  // die if only one neighbour or over-crowding with 4 or more neighours
    }
  }
}


// @todo optimize this ?
byte countNeighbors(byte t1[16][16],byte i, byte j)
{
  byte a;
  if((i==0)&&(j==0))
  {
    a=t1[i][j+1]+t1[i+1][j]+t1[i+1][j+1]+t1[i][Height-1]+t1[i+1][Height-1]+t1[Width-1][j]+t1[Width-1][j+1]+t1[Width-1][Height-1];
  }

  if((i!=0)&&(j!=0)&&(i!=(Width-1))&&(j!=(Height-1)))
  {
    a=t1[i-1][j-1]+t1[i-1][j]+t1[i-1][j+1]+t1[i][j+1]+t1[i+1][j+1]+t1[i+1][j]+t1[i+1][j-1]+t1[i][j-1];
  }
  
  if((i==0)&&(j!=0)&&(j!=(Height-1)))
  {
    a=t1[i][j-1]+t1[i+1][j-1]+t1[i+1][j]+t1[i+1][j+1]+t1[i][j+1]+t1[Width-1][j-1]+t1[Width-1][j]+t1[Width-1][j+1];
  }

  if((i==0)&&(j==(Height-1)))
  {
    a=t1[i][j-1]+t1[i+1][j-1]+t1[i+1][j]+t1[i][0]+t1[i+1][0]+t1[Width-1][0]+t1[Width-1][j]+t1[Width-1][j-1];
  }
  
  if((i==(Width-1))&&(j==0))
  {
    a=t1[i-1][j]+t1[i-1][j+1]+t1[i][j+1]+t1[i][Height-1]+t1[i-1][Height-1]+t1[0][j]+t1[0][j+1]+t1[0][Height-1];
  }
  
  if((i==(Width-1))&&(j!=0)&&(j!=(Height-1)))
  {
    a=t1[i][j-1]+t1[i][j+1]+t1[i-1][j-1]+t1[i-1][j]+t1[i-1][j+1]+t1[0][j]+t1[0][j-1]+t1[0][j+1];
  }
  
  if((i==(Width-1))&&(j==(Height-1)))
  {
    a=t1[i][j-1]+t1[i-1][j-1]+t1[i-1][j]+t1[0][j]+t1[0][j-1]+t1[i][0]+t1[i-1][0]+t1[0][0];
  }

  if((i!=0)&&(i!=(Width-1))&&(j==0))
  {
    a=t1[i-1][j]+t1[i-1][j+1]+t1[i][j+1]+t1[i+1][j+1]+t1[i+1][j]+t1[i][Height-1]+t1[i-1][Height-1]+t1[i+1][Height-1];
  }

  if((i!=0)&&(i!=(Width-1))&&(j==(Height-1)))
  {
    a=t1[i-1][j]+t1[i-1][j-1]+t1[i][j-1]+t1[i+1][j-1]+t1[i+1][j]+t1[i][0]+t1[i-1][0]+t1[i+1][0];
  }
  return a;
}


//**********************************************************************************************************************************************************
void randomize(byte t1[16][16])
{
  byte i,j;
  randomSeed(millis());
  for(i=0;i<Height;i++)
  {
    for(j=0;j<Width;j++)
    {
      t1[i][j]=random(2);
    }
  }
}

void cleart(byte t1[16][16])
{
  byte i,j;
  for(i=0;i<Height;i++)
  {
    for(j=0;j<Width;j++)
    {
      t1[i][j]=0;
    }
  }
}


/******************************************************************************
 * Tis function converts HSV values to RGB values, scaled from 0 to maxBrightness
 * 
 * The ranges for the input variables are:
 * hue: 0-360
 * sat: 0-255
 * lig: 0-255
 * 
 * The ranges for the output variables are:
 * r: 0-maxBrightness
 * g: 0-maxBrightness
 * b: 0-maxBrightness
 * 
 * r,g, and b are passed as pointers, because a function cannot have 3 return variables
 * Use it like this:
 * int hue, sat, val; 
 * unsigned char red, green, blue;
 * // set hue, sat and val
 * hsv2rgb(hue, sat, val, &red, &green, &blue, maxBrightness); //pass r, g, and b as the location where the result should be stored
 * // use r, b and g.
 * 
 * (c) Elco Jacobs, E-atelier Industrial Design TU/e, July 2011.
 * 
 *****************************************************************************/


void hsv2rgb(unsigned int hue, unsigned int sat, unsigned int val, \
              unsigned char * r, unsigned char * g, unsigned char * b, unsigned char maxBrightness ) { 
  unsigned int H_accent = hue/60;
  unsigned int bottom = ((255 - sat) * val)>>8;
  unsigned int top = val;
  unsigned char rising  = ((top-bottom)  *(hue%60   )  )  /  60  +  bottom;
  unsigned char falling = ((top-bottom)  *(60-hue%60)  )  /  60  +  bottom;

  switch(H_accent) {
  case 0:
    *r = top;
    *g = rising;
    *b = bottom;
    break;

  case 1:
    *r = falling;
    *g = top;
    *b = bottom;
    break;

  case 2:
    *r = bottom;
    *g = top;
    *b = rising;
    break;

  case 3:
    *r = bottom;
    *g = falling;
    *b = top;
    break;

  case 4:
    *r = rising;
    *g = bottom;
    *b = top;
    break;

  case 5:
    *r = top;
    *g = bottom;
    *b = falling;
    break;
  }
  // Scale values to maxBrightness
  *r = *r * maxBrightness/255;
  *g = *g * maxBrightness/255;
  *b = *b * maxBrightness/255;
}


