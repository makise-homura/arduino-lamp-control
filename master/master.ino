#include <Ds1302.h>             // Install DS1302
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>   // Install Adafruit_SSD1306
#include <EEPROM.h>

int oldpower = -1;
Ds1302 rtc(51, 47, 49);
Ds1302::DateTime now;
enum settings_t {D_MH = 0, D_MM = 1, D_EH = 2, D_EM = 3, D_DUR = 4, D_END};
int settings[D_END];
const int defaults[D_END] = {7, 0, 23, 0, 60};
enum state_t {S_RUN = 0, S_SET_H, S_SET_M, S_SET_S, S_SET_MH, S_SET_MM, S_SET_EH, S_SET_EM, S_SET_DUR} state = S_RUN;
int oldpressed = 0;
int power;

void dprintf(Adafruit_SSD1306 &display, bool invert, const char *format, int value)
{
  char str[40];
  sprintf(str, format, value);
  display.setTextColor(invert ? BLACK: WHITE, WHITE);
  display.print(str);
}

#define SSD1306_128x64

#ifdef SSD1306_128x64
#define SSD1306_HEIGHT 64
#define SSD1306_ADDRESS 0x3C
void ssd1306_show_data(Adafruit_SSD1306 &display)
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(16, 10);
  dprintf(display, state == S_SET_H, "%2u", now.hour);
  dprintf(display, false, ":", 0);
  dprintf(display, state == S_SET_M, "%02u", now.minute);
  dprintf(display, false, ":", 0);
  dprintf(display, state == S_SET_S, "%02u", now.second);
  display.setCursor(6, 36);
  display.setTextSize(1);
  dprintf(display, false, "Day:  ", 0);
  dprintf(display, state == S_SET_MH, "%2u", settings[D_MH]);
  dprintf(display, false, ":", 0);
  dprintf(display, state == S_SET_MM, "%02u", settings[D_MM]);
  dprintf(display, false, " - ", 0);
  dprintf(display, state == S_SET_EH, "%2u", settings[D_EH]);
  dprintf(display, false, ":", 0);
  dprintf(display, state == S_SET_EM, "%02u", settings[D_EM]);
  display.setCursor(6, 46);
  dprintf(display, false, "Duration (min):  ", 0);
  dprintf(display, state == S_SET_DUR, "%2u", settings[D_DUR]);

  if (state == S_RUN)
  {
    display.setCursor(6, 56);
    dprintf(display, false, "Current power: %3u%%", power);
  }
  else
  {
    display.setCursor(9, 56);
    dprintf(display, false, "  - SETUP MODE -  ", power);
  }
  display.display();
}
#endif 

#ifdef SSD1306_128x32
#define SSD1306_HEIGHT 32
#define SSD1306_ADDRESS 0x3C
void ssd1306_show_data(Adafruit_SSD1306 &display)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 16);
  if (state == S_RUN) dprintf(display, false, "V=%3u%%  ", power); else dprintf(display, false, " -SET-  ", power);
  dprintf(display, state == S_SET_H, "%2u", now.hour);
  dprintf(display, false, ":", 0);
  dprintf(display, state == S_SET_M, "%02u", now.minute);
  dprintf(display, false, ":", 0);
  dprintf(display, state == S_SET_S, "%02u", now.second);
  display.setCursor(0, 25);
  dprintf(display, state == S_SET_MH, "%2u", settings[D_MH]);
  dprintf(display, false, ":", 0);
  dprintf(display, state == S_SET_MM, "%02u", settings[D_MM]);
  dprintf(display, false, "-", 0);
  dprintf(display, state == S_SET_EH, "%2u", settings[D_EH]);
  dprintf(display, false, ":", 0);
  dprintf(display, state == S_SET_EM, "%02u", settings[D_EM]);
  dprintf(display, false, " / ", 0);
  dprintf(display, state == S_SET_DUR, "%2u", settings[D_DUR]);
  display.display();
}
#endif 

Adafruit_SSD1306 display(128, SSD1306_HEIGHT, &Wire);

void setup()
{
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, SSD1306_ADDRESS);
  rtc.init();
  Serial.begin(115200);
  Serial2.begin(9600);
  for(int i = 0; i < D_END; ++i)
  {
    if((settings[i] = EEPROM.read(i)) > 127) settings[i] = defaults[i];
  }
  pinMode(30, INPUT_PULLUP);
}

int rotate(int what, int min, int max, int delta)
{
  if (delta < 0)
  {
    if (what == min) return max;
    return --what;
  }
  else if (delta > 0)
  {
    if (what == max) return min;
    return ++what;
  }
  return what;
}

void loop()
{
  /* If state is RUNNING, get time from RTC and calculate power */
  if (state == S_RUN)
  {
    rtc.getDateTime(&now);
    long cur_ts = (long)now.hour * 3600 + (long)now.minute * 60 + (long)now.second;
    long morning_start = (long)settings[D_MH] * 3600 + (long)settings[D_MM] * 60;
    long morning_end = morning_start + (long)settings[D_DUR] * 60;
    long evening_start = (long)settings[D_EH] * 3600 + (long)settings[D_EM] * 60;
    long evening_end = evening_start + (long)settings[D_DUR] * 60;
    if (morning_start < evening_start)
    {
      if (cur_ts < morning_start) power = 0;
      else if (cur_ts < morning_end) power = (cur_ts - morning_start) * 100 / (morning_end - morning_start);
      else if (cur_ts < evening_start) power = 100;
      else if (cur_ts < evening_end) power = 100 - (cur_ts - evening_start) * 100 / (evening_end - evening_start);
      else power = 0;
    }
    else
    {
      if (cur_ts < evening_start) power = 100;
      else if (cur_ts < evening_end) power = 100 - (cur_ts - evening_start) * 100 / (evening_end - evening_start);
      else if (cur_ts < morning_start) power = 0;
      else if (cur_ts < morning_end) power = (cur_ts - morning_start) * 100 / (morning_end - morning_start);
      else power = 100;
    }

    /* Write new power value to slave */
    if (power != oldpower)
    {
      Serial2.write(power);
      oldpower = power;
    }
  }

  ssd1306_show_data(display);

  /* Determine if joystick is actuated */
  int xdif = analogRead(A15), ydif = analogRead(A14), pressed = !digitalRead(30);
  if ((pressed && !oldpressed) || (!pressed && oldpressed))
  {
    delay(50);
    pressed = !digitalRead(30);
  }
  if (xdif < 200 || xdif > 824)
  {
    delay(50);
    xdif = analogRead(A15);
  }
  if (ydif < 200 || ydif > 824)
  {
    delay(50);
    ydif = analogRead(A14);
  }
  
  /* State machine */
  if(oldpressed && !pressed)
  {
    if (state == S_RUN) state = S_SET_H;
    else
    {
      /* Save data to EEPROM and RTC */
      for(int i = 0; i < D_END; ++i)
      {
        EEPROM.update(i,settings[i]);
      }
      rtc.setDateTime(&now);
      state = S_RUN;
    }
  }
  else
  {
    if (xdif < 200 || xdif > 824)
    {
      state = (enum state_t)rotate(state, S_SET_H, S_SET_DUR, (xdif < 512) ? -1 : 1);
    }
    else if (ydif < 200 || ydif > 824)
    {
      if (state == S_SET_H) now.hour = rotate(now.hour, 0, 23, (ydif < 512) ? -1 : 1);
      else if (state == S_SET_M) now.minute = rotate(now.minute, 0, 59, (ydif < 512) ? -1 : 1);
      else if (state == S_SET_S) now.second = rotate(now.second, 0, 59, (ydif < 512) ? -1 : 1);
      else if (state == S_SET_MH) settings[D_MH] = rotate(settings[D_MH], 0, 23, (ydif < 512) ? -1 : 1);
      else if (state == S_SET_MM) settings[D_MM] = rotate(settings[D_MM], 0, 59, (ydif < 512) ? -1 : 1);
      else if (state == S_SET_EH) settings[D_EH] = rotate(settings[D_EH], 0, 23, (ydif < 512) ? -1 : 1);
      else if (state == S_SET_EM) settings[D_EM] = rotate(settings[D_EM], 0, 59, (ydif < 512) ? -1 : 1);
      else settings[D_DUR] = rotate(settings[D_DUR], 1, 99, (ydif < 512) ? -1 : 1);
    }
  }
  oldpressed = pressed;

  delay(200);
}
