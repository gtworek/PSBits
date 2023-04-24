//Will work on M5 Atom S3

#include <M5AtomS3.h>

int32_t mpx1 = 32;
int32_t mpx2 = 64;
int32_t mpx3 = 96;
int32_t mpy1 = 32;
int32_t mpy2 = 64;
int32_t mpy3 = 96;
int32_t pixSize = 16;
uint32_t pixColor = WHITE;

void drawPix1()
{
  M5.Lcd.fillRect((int32_t)(mpx1 - (pixSize / 2)), (int32_t)(mpy1 - (pixSize / 2)), pixSize, pixSize, pixColor);
}

void drawPix2()
{
  M5.Lcd.fillRect((int32_t)(mpx1 - (pixSize / 2)), (int32_t)(mpy2 - (pixSize / 2)), pixSize, pixSize, pixColor);
}

void drawPix3()
{
  M5.Lcd.fillRect((int32_t)(mpx1 - (pixSize / 2)), (int32_t)(mpy3 - (pixSize / 2)), pixSize, pixSize, pixColor);
}

void drawPix4()
{
  M5.Lcd.fillRect((int32_t)(mpx2 - (pixSize / 2)), (int32_t)(mpy2 - (pixSize / 2)), pixSize, pixSize, pixColor);
}

void drawPix5()
{
  M5.Lcd.fillRect((int32_t)(mpx3 - (pixSize / 2)), (int32_t)(mpy1 - (pixSize / 2)), pixSize, pixSize, pixColor);
}

void drawPix6()
{
  M5.Lcd.fillRect((int32_t)(mpx3 - (pixSize / 2)), (int32_t)(mpy2 - (pixSize / 2)), pixSize, pixSize, pixColor);
}

void drawPix7()
{
  M5.Lcd.fillRect((int32_t)(mpx3 - (pixSize / 2)), (int32_t)(mpy3 - (pixSize / 2)), pixSize, pixSize, pixColor);
}

void drawPat1()
{
  drawPix4();
}

void drawPat2()
{
  drawPix1();
  drawPix7();
}

void drawPat3()
{
  drawPix1();
  drawPix4();
  drawPix7();
}

void drawPat4()
{
  drawPix1();
  drawPix3();
  drawPix5();
  drawPix7();

}

void drawPat5()
{
  drawPix1();
  drawPix3();
  drawPix4();
  drawPix5();
  drawPix7();
}

void drawPat6()
{
  drawPix1();
  drawPix2();
  drawPix3();
  drawPix5();
  drawPix6();
  drawPix7();
}

void setup() 
{
  M5.begin(true, true, true, true);
  M5.Lcd.begin();
}

void loop() 
{
  M5.update();
  randomSeed(M5.Btn.lastChange());
  if (M5.Btn.wasPressed())
  {
    M5.Lcd.fillRect(0, 0, 128, 128, WHITE);
  }

  if (M5.Btn.wasReleased())
  {
    int iRandom = random(1, 7);
    M5.Lcd.fillRect(0, 0, 128, 128, BLACK);
    switch (iRandom) 
    {
      case 1 : 
        drawPat1();
        break;
      case 2 : 
        drawPat2();
        break;
      case 3 : 
        drawPat3();
        break;
      case 4 : 
        drawPat4();
        break;
      case 5 : 
        drawPat5();
        break;
      case 6 : 
        drawPat6();
        break;
    }
  }

  delay(50);
}
