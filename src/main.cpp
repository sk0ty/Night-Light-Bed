#include <Arduino.h>

bool walking;
bool state; // state = false = led aus
bool night_modus;
bool schnelles_an_aus; // default = false. Falls Led-Bett an ist, 8s liegt => licht wieder aus, dann instant wieder aufsteht, wird der counter von 8s uebersprungen und die led geht auch sofort wieder an.

const int sensor = 2;               // alufolie druckplatte
const int Anmach_zeit = 500;        // zuletzt 400
const int Ausmach_zeit = 12000;     //zuletzt 8000, und war an sich immer zu kurz
const int minimum_brightness = 128; // irgendwas zwischen 120-130
const float e = 2.718;
const long auto_shutdown = 900000; // 900s auto_shutdown, 900*1000ms
const int polling_rate = 100;
const int led_OUT = 5; // D5 = PWM-Pin

int n;
int i;
long m; // siehe loop_mit_lichtsensoren
int var;
int counter;
int led_rise_fall_time = 1500; // getestet mit 1200 ist eig zu schnell

int light_sensor_pin_0 = A0;

int light_sensor_value_0;

void led_rise()
{

  for (int n = 0; n <= 255; n++)
  {
    analogWrite(led_OUT, n);
    delay(led_rise_fall_time / 256);
  }
  state = false;
  // Serial.println("<< AN >>");
}

void led_fall()
{

  for (int n = 255; n >= 0; n--)
  {
    analogWrite(led_OUT, n);
    delay(led_rise_fall_time / 256);
  }
  state = true;
  // Serial.println("<< AUS >>");
}

void led_rise_e()
{

  for (int n = 0; n <= 255; n++)
  {
    analogWrite(led_OUT, 255 * (1 - pow(e, (-n / 70))));
    delay(led_rise_fall_time / 256);
  }
  analogWrite(led_OUT, 255);

  state = false;
  // Serial.println("<< AN >>");
}

void led_fall_e()
{
  for (int n = 255; n >= 0; n--)
  {
    analogWrite(led_OUT, 255 * pow(e, (-n / 70))); // 255*e^(-n/70)
    delay(led_rise_fall_time / 256);
  }
  analogWrite(led_OUT, 0);

  state = true;
  // Serial.println("<< AUS >>");
}

void check_brightness()
{
  light_sensor_value_0 = analogRead(light_sensor_pin_0);
  //light_sensor_value_1 = analogRead(light_sensor_pin_1);
  //light_sensor_value_2 = analogRead(light_sensor_pin_2);

  if (light_sensor_value_0 < minimum_brightness) //&& light_sensor_value_1 < minimum_brightness && light_sensor_value_2 < minimum_brightness)
  {
    night_modus = true;
  }
  else
  {
    night_modus = false;
  }
}

void setup()
{
  //Serial.begin(9600);
  pinMode(sensor, INPUT);

  pinMode(led_OUT, OUTPUT);

  walking = false;
  state = true;
  schnelles_an_aus = false;
}

void loop() // alpha finished !
{
  check_brightness();
  walking = digitalRead(sensor);
  delay(polling_rate);

  if (night_modus == false) //reset fuer schnelles_an_aus
  {
    schnelles_an_aus = false;
  }

  if (schnelles_an_aus == true)
  { // Dieser wird den fall fixen, falls hinlegen und in led_fall wieder aufstehen nix passiert. Weil er dann nicht in die                              //untere if-Bedingung gehen würde, weil walking=true ist.
    walking = false;
  }

  if (walking == false && state == true && night_modus == true)
  {
    counter = 0;
    while (walking == false && night_modus == true && counter <= 80 && schnelles_an_aus == false) //checken ob mind. 8s im bett. counter wird uebersprungen wenn schnelles_an_aus= true.
    {
      check_brightness();
      walking = digitalRead(sensor);
      delay(polling_rate);
      counter++;
    }
    if (counter >= 80 || schnelles_an_aus == true) //80 counter heisst 8s mindest liege zeit. Wenn counter >80 ist, dann wartet das System auf aufstehen oder night_modus = false. Der counter wird uebersprungen wenn der schnelles_an_aus=true ist. schnelles_an_aus wird erst wieder false wenn night_modus =false ist oder die anzeit von 15min überschritten ist.
    {
      Serial.println("Ueberpruefe ob anmachen");
      i = 0;
      while (i <= Anmach_zeit / polling_rate && night_modus == true) // Erst wenn i>19 ist oder night_modus = false ist, breaken
      {
        walking = digitalRead(sensor);
        check_brightness();
        i = 0;
        delay(polling_rate);
        while (walking == true && night_modus == true && i <= Anmach_zeit / polling_rate)
        {
          walking = digitalRead(sensor);
          check_brightness();
          i++;
          delay(polling_rate);
        }
      }

      if (i >= Anmach_zeit / polling_rate && night_modus == true) //mindestzeit von 500ms überschritten => Licht an
      {
        led_rise(); // dann gehts an
        schnelles_an_aus = true;
        m = 0;
      }

      while (state == false)
      {
        walking = digitalRead(sensor);
        check_brightness();

        i = 0;
        while (walking == false && i <= (Ausmach_zeit / polling_rate) && (night_modus == true || m >= auto_shutdown / polling_rate))
        {
          m = m + 1;

          walking = digitalRead(sensor);
          check_brightness();
          delay(polling_rate);
          i++;
        }

        m = m + 1;
        delay(polling_rate);

        if (night_modus == false || m >= auto_shutdown / polling_rate)
        {
          led_fall();
          schnelles_an_aus = false;
        }
        if (i >= Ausmach_zeit / polling_rate)
        {
          led_fall();
          schnelles_an_aus = true;
        }
      }
    }
  }
}
