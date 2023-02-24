#define NUMPIXELS        1
long    LEDBlingMills;
Adafruit_NeoPixel pixels(NUMPIXELS, 8, NEO_GRB + NEO_KHZ800);
// the setup routine runs once when you press reset:
void setuprgbLed() {
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(100); // not so bright
}
// the loop routine runs over and over again forever:
void TurnOnRGBBlue() {
  pixels.fill(0x0000FF);
  pixels.show();
}
void TurnOnRGBRed() {
  pixels.fill(0xC70039);
  pixels.show();
}
void TurnOnRGBGreen() {
  pixels.fill(0x7CFC00);
  pixels.show();
}
void TurnOffRGBled(){
  pixels.fill(0x000000);
  pixels.show();
}
void blinkLed(int ms,int led){
  if ( ( millis() - LEDBlingMills ) >= ms ){
  switch(led){
    case 1:
      TurnOnRGBBlue();
      break;
    case 2:
      TurnOnRGBRed();
      break;
    case 3:
      TurnOnRGBGreen();
      break;
  }
 }
 if( ( millis() - LEDBlingMills )  >= (ms*2) ){
    LEDBlingMills = millis();
    TurnOffRGBled();
 }
}
