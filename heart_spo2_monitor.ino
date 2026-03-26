/* This code works with MAX30102 + 128x32 OLED i2c + Buzzer and Arduino  UNO
 * It's displays the Average BPM on the screen, with an animation and a buzzer  sound
 * everytime a heart pulse is detected
 * It's a modified version of  the HeartRate library example
 * Refer to www.surtrtech.com for more details  or SurtrTech YouTube channel
 */


#include <Wire.h>
#include <DFRobot_RGBLCD1602.h>
#include "MAX30105.h"           //MAX3010x library
#include "heartRate.h"          //Heart rate  calculating algorithm
#include "spo2_algorithm.h"

MAX30105 particleSensor;


//Arduino Uno doesn't have enough SRAM to store 80 samples of IR led data and red led data in 32-bit format
//To solve this problem, 16-bit MSB of the sampled data will be truncated. Samples become 16-bit data.
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data

int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

// LCD configuration
DFRobot_RGBLCD1602 lcd(/*RGBAddr*/0x6B ,/*lcdCols*/16,/*lcdRows*/2);  //16 characters and 2 lines of show


void setup() {  
  Serial.begin(115200);
  Serial.println("Initialising...");

  // Initialize LCD
  lcd.init();               // Initialize the LCD
  lcd.setPWM(WHITE, 255);         // Turn on the backlight
  
  // Display "Monaghan Best County" on two lines of the LCD
  lcd.clear();              // Clear any previous content
  
  lcd.setCursor(0, 0);
  lcd.print("Please place");
  lcd.setCursor(0,1);
  lcd.print("your finger");


  //  Initialize sensor
  particleSensor.begin(Wire, I2C_SPEED_FAST); //Use default  I2C port, 400kHz speed
  particleSensor.setup(); //Configure sensor with default  settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to  indicate sensor is running

}

void loop()
{

  bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps

  //read the first 100 samples, and determine the signal range
  for (byte i = 0 ; i < bufferLength ; i++)
  {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample

    Serial.print(F("red="));
    Serial.print(redBuffer[i], DEC);
    Serial.print(F(", ir="));
    Serial.println(irBuffer[i], DEC);
  }

  //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
  while (1)
  {
    //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
    for (byte i = 25; i < 100; i++)
    {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    //take 25 sets of samples before calculating the heart rate.
    for (byte i = 75; i < 100; i++)
    {
      while (particleSensor.available() == false) //do we have new data?
        particleSensor.check(); //Check the sensor for new data

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); //We're finished with this sample so move to next sample

      //send samples and calculation result to terminal program through UART
      Serial.print(F("red="));
      Serial.print(redBuffer[i], DEC);
      Serial.print(F(", ir="));
      Serial.print(irBuffer[i], DEC);

      Serial.print(F(", HR="));
      Serial.print(heartRate, DEC);

      Serial.print(F(", HRvalid="));
      Serial.print(validHeartRate, DEC);

      Serial.print(F(", SPO2="));
      Serial.print(spo2, DEC);

      Serial.print(F(", SPO2Valid="));
      Serial.println(validSPO2, DEC);
    }


    lcd.clear();                                              //Clears the LCD
    lcd.setCursor(0,0);
    lcd.print("HR: ");
    if (validHeartRate == 1){
      lcd.print(heartRate);
      }
    else{
      lcd.print("N/A");
    }                                 //Prints the average beats per minute
    lcd.print(" BPM");


    lcd.setCursor(0,1);
    lcd.print("SP02:");
    if (validSPO2 == 1){
      lcd.print(spo2);
      }
    else{
      lcd.print("N/A");
    }                                                 //Prints the average beats per minute


    //After gathering 25 new samples recalculate HR and SP02
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
  }
}
