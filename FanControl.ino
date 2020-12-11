#include <SoftwareSerial.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <ky-040.h>   // Encoder Lbi


#define VersionString F("Fan Control V0.4 (c) by chessi soft")
#define VersionStringSmall F("** FanControl V1.0 **")
/*
  Software serial multple serial test
 */

// Translation from 0..100 (101 Values!) Percent to ADC 0..225 0=> Hight rotation 255=> Low rotation!
byte const FanLut[] PROGMEM  = { 
  225,224,223,221,220,219,218,217,216,215,
  214,213,212,210,209,207,206,205,203,202,
  200,199,196,194,192,190,188,186,184,182,
  180,177,175,172,170,167,165,162,160,157,
  154,151,149,146,143,140,136,132,128,126,
  122,118,115,113,110,108,106,104,101,100, 
   99, 98, 98, 97, 96, 95, 94, 93, 92, 90, 
   88, 84, 81, 78, 73, 70, 64, 61, 58, 55,
   52, 48, 45, 42, 39, 36, 33, 30, 27, 24,
   21, 19, 16, 14, 12, 10,  8,  4,  2,  0,
   0
};



//===============
// SSD1306 
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
//===============
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//===============
// Encoder
//===============
#define ENCODER_CLK1         2      // This pin must have a minimum 0.47 uF capacitor
#define ENCODER_DT1          3      // data pin
#define ENCODER_SW1          4      // switch pin (active LOW)
#define MAX_ROTARIES1        3      // defines two rotaries for this encoder
ky040 encoder1(ENCODER_CLK1, ENCODER_DT1, ENCODER_SW1, MAX_ROTARIES1 );
#define ROTARY_ID1           1      // Idle Speed
#define ROTARY_ID2           2      // Boost Speed
#define ROTARY_ID3           3      // Boost Balance

//===============
// External Boost Switch
//===============
#define BoostSwitchPin      5       // If low active Boost Mode!


//===============
// PWM Outputs for Fan Group 1 and 2
//===============
#define FanGroup_1_Pin  9
#define FanGroup_2_Pin  10

//===============
// Store Fan Settings
//===============
struct strut_PersistendData
{
  byte fanGroup_1_lowPercent;
  byte fanGroup_1_highPercent;
  byte fanGroup_2_lowPercent;
  byte fanGroup_2_highPercent;
  int  fanBalance;
} PersistendData;
int eeAddress=0; // Byteoffset where the persistent Data are located in EEPROM


//===============
// It's the nano build in red LED. Blinks in idle loop to show alive state
//===============
#define BOARD_LED           13      // I'm alive blinker

void Error ( int encoderNum ) {
  
    Serial.print(F("ERROR creating encoder ")); Serial.print(encoderNum); Serial.println("!");
    Serial.println(F("Possible malloc error, the rotary ID is incorrect, or too many rotaries"));
    while(1);   // Halt and catch fire
}


void FanPowerBoost()
{
    
    byte v = pgm_read_byte_near( FanLut + PersistendData.fanGroup_1_highPercent);
    analogWrite(FanGroup_1_Pin, v );   
    v = pgm_read_byte_near( FanLut + PersistendData.fanGroup_2_highPercent);
    analogWrite(FanGroup_2_Pin, v );   
}

void FanPowerIdle()
{
    byte v = pgm_read_byte_near( FanLut + PersistendData.fanGroup_1_lowPercent);
    analogWrite(FanGroup_1_Pin, v );   
    v = pgm_read_byte_near( FanLut + PersistendData.fanGroup_2_lowPercent);
    analogWrite(FanGroup_2_Pin, v );   
}

void FanPower(bool isBooAcive)
{
  if( isBooAcive )
     FanPowerBoost();
  else
     FanPowerIdle();
}

void ReadData()
{
    EEPROM.get(eeAddress, PersistendData);
    // Check boundary....
    PersistendData.fanGroup_1_lowPercent  = PersistendData.fanGroup_1_lowPercent  > 100 ? 0 : PersistendData.fanGroup_1_lowPercent;
    PersistendData.fanGroup_1_highPercent = PersistendData.fanGroup_1_highPercent > 100 ? 0 : PersistendData.fanGroup_1_highPercent;
    PersistendData.fanGroup_2_lowPercent  = PersistendData.fanGroup_2_lowPercent  > 100 ? 0 : PersistendData.fanGroup_2_lowPercent;
    PersistendData.fanGroup_2_highPercent = PersistendData.fanGroup_2_highPercent > 100 ? 0 : PersistendData.fanGroup_2_highPercent;
  
    PersistendData.fanBalance = PersistendData.fanBalance >  50 ? 0 : PersistendData.fanBalance;
    PersistendData.fanBalance = PersistendData.fanBalance < -50 ? 0 : PersistendData.fanBalance;
}

void SaveData()
{
  EEPROM.put(eeAddress, PersistendData);
}

#define Display_Mode_Low 0 
#define Display_Mode_High 1
#define Display_Mode_Balance 2 
#define Display_Mode_Boost 3 


void ShowDisplay(bool isBoostActive, bool isBalanceActive)
{
    char TextBuffer[16];
    
    display.clearDisplay();
    display.setTextSize(2);             // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);        // Draw white text

    if( isBoostActive  )
    {
      display.setCursor(26,0);   
      display.print(F(" Low/"));
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
      display.print(F("Pow"));
      display.setTextColor(SSD1306_WHITE);        // Draw white text
    }
    else
    {
      display.setCursor(26,0);   
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
      display.print(F(" Low/"));
      display.setTextColor(SSD1306_WHITE);        // Draw white text
      display.print(F("Pow"));
    }

    display.setCursor(0,20);   
    sprintf( TextBuffer, "F1:%3d/%3d",PersistendData.fanGroup_1_lowPercent,PersistendData.fanGroup_1_highPercent);
    display.println(TextBuffer );

    display.setCursor(0,40);   
    sprintf( TextBuffer, "F2:%3d/%3d",PersistendData.fanGroup_2_lowPercent,PersistendData.fanGroup_2_highPercent);
    display.println(TextBuffer );

    display.setCursor(0,58);   
    if( isBalanceActive  )
      display.println(F("**********") );
    
    if( false  )
    {
      display.setCursor(0,0);   
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
      display.println(F("  Balance ") );
      display.setTextColor(SSD1306_WHITE);        // Draw white text
    }

    display.display(); // Update Screenbuffer

}

bool CheckAndActivateBoostSwitch ( void ) {
  if ( digitalRead(BoostSwitchPin) == false ) 
  {
    // Button pressed. Wait for button release.
    //while ( digitalRead(BoostSwitchPin) == false ) ;
    // We may get here and its still bouncing.
    delay(100);
    return true;
  }
  return false;
}


void setup() 
{
  // Read back data of last execution from EEPROM
  ReadData();

  // First of all set Pin Mode for the Fan driver, otherwise they go to top speed until
  // display is initialized!
  //===============
  // PIN9 PWM output FAN-Group-1
  pinMode(FanGroup_1_Pin,OUTPUT);
  // PIN10 PWM output FAN-Group-2
  pinMode(FanGroup_2_Pin,OUTPUT);
  FanPowerIdle();
  //===============

  pinMode(BoostSwitchPin,INPUT_PULLUP);
  pinMode(BOARD_LED,OUTPUT);
  
  // Default is 0x03 for Timer-1 => 490Hz at PIN9 and PIN10
  // We change that setting to 31200Hz to reduce the PWM noise on FAN DC motor
  // New Setting is 0x01
  // To modify Timer-1 the delay functions are not affected!
  TCCR1B = TCCR1B & 0b11111000 | 0x01;  
    
  // Open serial communications and wait for port to open:
  Serial.begin(57600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println(VersionString);

  // Add the two rotaries for the first encoder.
  // Define a rotary to go from 0 to 100 in increments of 5. Initialize it to last stored. No rollover
   Serial.print(F("Init Rotary 1.."));
   if ( ! encoder1.AddRotaryCounter(ROTARY_ID1, PersistendData.fanGroup_1_lowPercent, 0, 100, 5, false ) ) {
    Error(1);
  }
  Serial.println(F("..[ok]"));
  
  // Define a rotary to go from 0 to 100 in increments of 5. Initialize it to last stored. No rollover
  Serial.print(F("Init Rotary 2.."));
  if ( ! encoder1.AddRotaryCounter(ROTARY_ID2, PersistendData.fanGroup_1_highPercent, 0, 100, 5, false ) ) {
    Error(1);
  }
  Serial.println(F("..[ok]"));

  // Define a rotary to go from -100 to 100 in increments of 5. Initialize it to last stored. No rollover
  Serial.print(F("Init Rotary 3.."));
  if ( ! encoder1.AddRotaryCounter(ROTARY_ID3, PersistendData.fanBalance, -50, 50, 1, false ) ) {
    Error(1);
  }
  Serial.println(F("..[ok]"));

  
  // Make ROTARY_ID1 active (responds to encoder shaft movements)
  encoder1.SetRotary(ROTARY_ID1);

  Serial.print(F("Init OLED Disp."));
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  Serial.println(F("..[ok]"));


  display.display();
  delay(2000); // Pause for 2 second
  
  // Clear the buffer
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(VersionStringSmall);
  display.println(F("(c) '20 by Chessi Soft"));
  display.display();
  delay(1000); // Pause for 1 second

}

 


void loop() 
{ 
    static bool firstTime = true;   // Force an initial display of rotary values
    static bool BoostMode = true;   // 
    static bool BalanceMode = false;
    static int dispMode=Display_Mode_Low;

    
    digitalWrite(BOARD_LED,millis() % 1000 > 500);  // Blink I'm alive   

    // On each encoder switch press, toggle between the two rotaries.
    // Use the IsActive call 
    if ( encoder1.SwitchPressed() ) {
        //Serial.print("Encoder 1 switch pressed. ");
        if ( encoder1.IsActive(ROTARY_ID1) || encoder1.IsActive(ROTARY_ID2) ) {
            encoder1.SetRotary(ROTARY_ID3);
            BalanceMode = true;
            Serial.println(F("Balance Mode Active."));
        }
//        else if ( encoder1.IsActive(ROTARY_ID2) ) {
//            encoder1.SetRotary(ROTARY_ID3);
//            dispMode = Display_Mode_Balance;
//            Serial.println("ROTARY_ID3 now Active.");
//        }
        else {
            // switch back to rotary, but depends on boost mode:
            if( BoostMode )
               encoder1.SetRotary(ROTARY_ID2);
            else
               encoder1.SetRotary(ROTARY_ID1);
            BalanceMode = false;   
            dispMode = Display_Mode_Balance;
            //dispMode = Display_Mode_Low;
            //Serial.println(F("ROTARY_ID1 now Active."));            
        }
        encoder1.SetChanged();   // force an update on active rotary   
    }

    if ( firstTime || encoder1.HasRotaryValueChanged() ) {
        Serial.print(F("Low     : ")); Serial.println(encoder1.GetRotaryValue(ROTARY_ID1));
        Serial.print(F("Boost   : ")); Serial.println(encoder1.GetRotaryValue(ROTARY_ID2)); 
        Serial.print(F("Balance : ")); Serial.println(encoder1.GetRotaryValue(ROTARY_ID3)); 
        Serial.println("---------------------------------------"); 
    
        PersistendData.fanBalance = encoder1.GetRotaryValue(ROTARY_ID3);
        PersistendData.fanGroup_1_lowPercent = encoder1.GetRotaryValue(ROTARY_ID1);
        PersistendData.fanGroup_2_lowPercent = encoder1.GetRotaryValue(ROTARY_ID1);

        int curSet = encoder1.GetRotaryValue(ROTARY_ID2);
        int left   = curSet+PersistendData.fanBalance;
        int right  = curSet-PersistendData.fanBalance;
        left = left > 100 ? 100 : left;
        left = left < 0 ? 0: left;
        right = right > 100 ? 100 : right;
        right = right < 0? 0 : right;
        PersistendData.fanGroup_1_highPercent = left;
        PersistendData.fanGroup_2_highPercent = right;

        SaveData();  //  store EEPROM

        FanPower(BoostMode);
        ShowDisplay(BoostMode,BalanceMode);

        firstTime = false;    // Now only update display if a rotary value has changed.
    }

    if( CheckAndActivateBoostSwitch() )
    {
      if( !BoostMode )
      {
        if( !BalanceMode ){  // keep Balance Mode
          encoder1.SetRotary(ROTARY_ID2);
          encoder1.SetChanged();   // force an update on active rotary   
          Serial.println(F("Boost Mode."));
        }
        BoostMode = true;
        FanPower(BoostMode);
        ShowDisplay(BoostMode,BalanceMode);
      }
    }
    else
    {
      if( BoostMode )
      {
        if( !BalanceMode ){  // keep Balance Mode
          encoder1.SetRotary(ROTARY_ID1);
          encoder1.SetChanged();   // force an update on active rotary   
          Serial.println(F("Idle Mode."));
        }
        BoostMode=false;
        FanPower(BoostMode);
        ShowDisplay(BoostMode,BalanceMode);
      }
    }


}
