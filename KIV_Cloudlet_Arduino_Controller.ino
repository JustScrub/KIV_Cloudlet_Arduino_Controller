// Link with https://github.com/CuriosityGym/motordriver
// and https://bitbucket.org/jezhill/keyhole/src/main/

#include <MotorDriver.h>
#include "Keyhole.h"

#define CMD_PING "ping!"
#define CMD_LED  "led"
#define CMD_FAN1 "fan1"
#define CMD_FAN2 "fan2"
#define CMD_FAN3 "fan3"

#define M_FAN1 1
#define M_FAN2 2
#define M_FAN3 3
#define M_LED  4

MotorDriver m;
int power = 0;
int up_down = 1;
int led_count = 0;

String ping = "pong!";
short led_pwm = 0;
short fan1_pwm = 0;
short fan2_pwm = 0;
short fan3_pwm = 0;

KEYHOLE keyhole(Serial);

void setup()
{
  Serial.begin(9600);
  while (!Serial) continue;

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{  
  if(keyhole.begin()) // on most loops, this will return false, so
  {                   // very little processing will need to be done
    
    if (keyhole.variable(CMD_PING, ping, VARIABLE_READ_ONLY)) {
    }    
    if (keyhole.variable(CMD_FAN1, fan1_pwm)) {
      m.motor(M_FAN1, FORWARD, fan1_pwm);
    }
    if (keyhole.variable(CMD_FAN2, fan2_pwm)) {
      m.motor(M_FAN2, FORWARD, fan2_pwm);
    }
    if (keyhole.variable(CMD_FAN3, fan3_pwm)) {
      m.motor(M_FAN3, FORWARD, fan3_pwm);
    }
    if (keyhole.variable(CMD_LED, led_pwm)) {
      m.motor(M_LED, FORWARD, led_pwm);
    }

    keyhole.end(); // must call this if `.begin()` returned `true`
  }

  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(500);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(500);                      // wait for a second
}
