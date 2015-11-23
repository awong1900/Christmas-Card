/*
 Controlling a servo position using a potentiometer (variable resistor)
 by Michal Rinott <http://people.interaction-ivrea.it/m.rinott>

 modified on 8 Nov 2013
 by Scott Fitzgerald
 http://arduino.cc/en/Tutorial/Knob
*/

#include <Servo.h>

Servo myservo;  // create servo object to control a servo

int potpin = 0;  // analog pin used to connect the potentiometer
int val;    // variable to read the value from the analog pin

void setup()
{
  myservo.attach(21);  // attaches the servo on pin 9 to the servo object
}

void loop()
{
  myservo.write(20);                  // sets the servo position according to the scaled value
  delay(4000);
  myservo.write(90);
  delay(4000);                           // waits for the servo to get there
}
