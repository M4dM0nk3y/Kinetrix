#include <Wire.h>

#include <SPI.h>

#include <avr/wdt.h>

#include <Servo.h>

#include <DHT.h>

#include <Adafruit_NeoPixel.h>

#include <LiquidCrystal_I2C.h>

#include <Stepper.h>

#include <Encoder.h>

#include <PID_v1.h>



Servo _kx_servo;

DHT *_kx_dht = NULL;

Adafruit_NeoPixel *_kx_strip = NULL;

LiquidCrystal_I2C *_kx_lcd = NULL;

Stepper *_kx_stepper = NULL;

int _kx_motor_en = -1, _kx_motor_fwd = -1, _kx_motor_rev = -1;

Encoder *_kx_encoder = NULL;

Servo _kx_esc;

double _kx_pid_setpoint = 0, _kx_pid_input = 0, _kx_pid_output = 0;

PID *_kx_pid = NULL;




void setup() {

  Serial.begin(9600);

  Serial.setTimeout(100);

}


void loop() {

  _kx_stepper = new Stepper(200, 5, 6);
  _kx_motor_en = 9;
  _kx_motor_fwd = 10;
  _kx_motor_rev = 11;
  pinMode(_kx_motor_en, OUTPUT);

  pinMode(_kx_motor_fwd, OUTPUT);

  pinMode(_kx_motor_rev, OUTPUT);

  _kx_encoder = new Encoder(2, 3);
  _kx_esc.attach(4, 1000, 2000);
  if (_kx_pid) delete _kx_pid;
  _kx_pid = new PID(&_kx_pid_input, &_kx_pid_output, &_kx_pid_setpoint, 2, 0.5, 1.2, DIRECT);
  if (_kx_pid) _kx_pid->SetMode(AUTOMATIC);

  if (_kx_stepper) _kx_stepper->step(200);
  if (_kx_stepper) _kx_stepper->setSpeed(120);
  if (_kx_motor_en != -1) {

    digitalWrite(_kx_motor_fwd, HIGH);

    digitalWrite(_kx_motor_rev, LOW);

    analogWrite(_kx_motor_en, 255);
  }

  delay(1000);
  if (_kx_motor_en != -1) {

    digitalWrite(_kx_motor_fwd, LOW);

    digitalWrite(_kx_motor_rev, HIGH);

    analogWrite(_kx_motor_en, 128);
  }

  delay(1000);
  if (_kx_motor_en != -1) {

    digitalWrite(_kx_motor_fwd, LOW);

    digitalWrite(_kx_motor_rev, LOW);

    analogWrite(_kx_motor_en, 0);

  }

  _kx_esc.write(90);
  _kx_pid_setpoint = 100;
  for (int _i0 = 0; _i0 < (int)(
10); _i0++) {
    pos = (_kx_encoder ? _kx_encoder->read() : 0);
    out = (_kx_pid_input = (double)(pos), (_kx_pid ? _kx_pid->Compute() : false), _kx_pid_output);
    if (
(out > 0)) {
      if (_kx_motor_en != -1) {

        digitalWrite(_kx_motor_fwd, HIGH);

        digitalWrite(_kx_motor_rev, LOW);

        analogWrite(_kx_motor_en, out);
      }

    }
 else {
      if (_kx_motor_en != -1) {

        digitalWrite(_kx_motor_fwd, LOW);

        digitalWrite(_kx_motor_rev, HIGH);

        analogWrite(_kx_motor_en, (out * (0 - 1)));
      }

    }

    delay(100);
  }

  if (_kx_encoder) _kx_encoder->write(0);

}

