#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <core_pins.h>
#include <touch.c>
#include <SimpleKalmanFilter.h>

// GUItool: begin automatically generated code
AudioPlaySdWav           playSdWav1;     //xy=268,214
AudioPlaySdWav           playSdWav2;     //xy=287,270
AudioMixer4              mixer1;         //xy=448.00000381469727,228.00000190734863
AudioOutputI2S           i2s2;           //xy=614.0000114440918,224.00000190734863
AudioConnection          patchCord1(playSdWav1, 1, mixer1, 0);
AudioConnection          patchCord2(playSdWav2, 1, mixer1, 1);
AudioConnection          patchCord3(mixer1, 0, i2s2, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=539,135
// GUItool: end automatically generated code-------------------


// Use these if SD card is in the Audio Shield// --
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

//for kalman filter smoothing----------------------------------------
SimpleKalmanFilter simpleKalmanFilter(50, 50, 0.009);
// 1st 2 values are the same, estimated amt of variation,
// last v is btwn 0.001 - 1. the smaller the smoother.

void setup()   {
  Serial.begin(9600);
  AudioMemory(10);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  mixer1.gain(0, 0.5);
  mixer1.gain(1, 0.5);

  pinMode(13, OUTPUT); // LED on pin 13
  delay(1000);
}

elapsedMillis blinkTime;

void setTouchReadSensitivity(uint8_t t_current, uint8_t num_scans, uint8_t t_prescale) {
  //check the new values are in range
  if (t_current > 15) t_current = 15;
  if (num_scans > 31) num_scans = 31;
  if (t_prescale > 7) t_prescale = 7;

  //update variables
  CURRENT = t_current; //0-15, default is 2,
  NSCAN = num_scans; //number of times to scan, 0-31, default is 9;
  PRESCALE = t_prescale; //0-7,default is 2
}

void loop()
{
  if (playSdWav1.isPlaying() == false) {
    playSdWav1.play("STORY.wav");
    Serial.println("Start playing 1");
    delay(100); // wait for library to parse WAV info
  }
  if (playSdWav2.isPlaying() == false) {
    playSdWav2.play("NOISE1.wav");
    Serial.println("Start playing 2");
    delay(10); // wait for library to parse WAV info
  }

  // touchread input
  int touch = touchRead(29);

  // smooth value
  float touchSmoothed = simpleKalmanFilter.updateEstimate(touch);

  int touchMin = 2150; // calculating gain from range----------------=--
  int touchMax = 2225;
  int increment = (touchMax - touchMin) / 24;
  //laptop: min 2830, max 2900
  //wall plug: min 2100, max 2175
  //min should be whatever the rouchread value is when you're at the farthest point.
  // max is the value when you're nearest
  // lower is clearer, higher is more garbled. you want it right in the middle where it's
  // clear from close, but garbled from far

  // if sound is garbled no matter what, decrease both numbers
  // if sound is clear no matter what, increase numbers
  // (because if touchsmoothed - too small a #, then it's too high and defaults to maxgain)

  float gain1 = touchSmoothed - touchMin;
  gain1 = gain1 / increment;

  float maxGain = 24;
  if (gain1 > maxGain) {
    gain1 = maxGain;
  }
  if (gain1 < 0) {
    gain1 = 0;
  }
  float noiseMod = 0.1; // noise modifier

  float gain2 = (maxGain - gain1) * noiseMod;

  mixer1.gain(0, gain1);
  mixer1.gain(1, gain2);

  //Serial.print("audio mem max = ");
  //Serial.print(AudioMemoryUsageMax());
  //Serial.print("    ");

  Serial.print("touch = ");
  Serial.print(touch - touchMin);
  Serial.print("    ");

  Serial.print("smoothed = ");
  Serial.print(touchSmoothed - touchMin);
  Serial.print("    ");

  Serial.print("gain1 = ");
  Serial.print(gain1);
  Serial.print("    ");
  Serial.print("gain2 = ");
  Serial.println(gain2);


  // blink the LED without delays
  if (blinkTime < 100) {
    digitalWrite(13, LOW);
  } else if (blinkTime < 20) {
    digitalWrite(13, HIGH);
  } else {
    blinkTime = 0; // start blink cycle over again
  }

  setTouchReadSensitivity(15, 31, 1); // set sensitivity -------------------------------
  // (current ,numscans, prescaler)
  // C=2, n=9, p=2 gives approx 0.02 pF sensitivity and 1200 pF range
  // c max = 15, n max = 31, p max =  7
  // Lower current, higher number of scans, and higher prescaler
  // increase sensitivity, but the trade-off is longer measurement time and decreased range.


  delay(100);
}
