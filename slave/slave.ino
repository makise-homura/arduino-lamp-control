#include <RBDdimmer.h>          // Install https://github.com/RobotDynOfficial/RBDDimmer

dimmerLamp dimmer(3);

void setup()
{
  dimmer.begin(NORMAL_MODE, OFF);
  dimmer.setPower(0);
  dimmer.setState(ON);
  Serial.begin(9600);
}

void loop()
{
  if (Serial.available())
  {
    char power = Serial.read();
    dimmer.setPower(power);
  }
}
