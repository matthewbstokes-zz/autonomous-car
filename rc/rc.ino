#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_PWMServoDriver.h"
#include <NewPing.h>
#include <Servo.h>

#define MOTOR                     1
#define STEERING_PIN              10
#define ULTRASOUND_PIN            7
#define MAX_DISTANCE              400 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
#define SPEED_GAIN                100
#define LOWER_DISTANCE_THRESHOLD  40
#define TURN_LEFT_POS             115
#define TURN_RIGHT_POS            70
#define TURN_MID_POS              90
#define MOTOR_PWM_LOWER           42
#define MOTOR_PWM_UPPER           255

enum steering_direction {LEFT, MIDDLE, RIGHT};

class RoboCar {
  public:
    Adafruit_MotorShield AFMS;
    int motor_state;
    int steering;
    int acceleration;
    Servo steering_servo;
    unsigned int motor_speed;
    Adafruit_DCMotor *motor;
    NewPing *sonar;

  RoboCar() {
    AFMS = Adafruit_MotorShield();
    motor = AFMS.getMotor(MOTOR); 
    sonar = new NewPing(ULTRASOUND_PIN, ULTRASOUND_PIN , MAX_DISTANCE); // NewPing setup of pins and maximum distance.
    motor_state = BRAKE;
    acceleration = 1;
    motor_speed = 0;
    steering = TURN_MID_POS;
  }
  
  ~RoboCar() {
    delete sonar;
    delete motor; 
  }
  
  int Initialize() {
      AFMS.begin();
  
      steering_servo.attach(STEERING_PIN);
      
      
      steering_servo.write(TURN_MID_POS);
      delay(3000);    
   
     
      
      motor->setSpeed(100);
      motor_speed = 100;
      motor->run(RELEASE);
  }
  
  void BackThatAssUp() {
    steering_servo.write(TURN_MID_POS);
    delay(50);
    motor->setSpeed(60);
    MoveBackwards();
    delay(2000);
    motor->setSpeed(RELEASE);
    steering_servo.write(steering); 
  }
  
  int MoveForwards() {
    switch(motor_state) {
    case FORWARD:
      break;  // do nothing
    case BACKWARD:
      motor->run(RELEASE);
      delay(20);
    case RELEASE:
      motor->run(BRAKE);
      delay(20);
    case BRAKE:
      // acceleration
      motor->setSpeed(MOTOR_PWM_LOWER);
      motor->run(FORWARD);
      motor_state = FORWARD;
      for (int i = MOTOR_PWM_LOWER; i < motor_speed; i+=acceleration){
        motor->setSpeed(i);
        delay(3);
      }
      break;
    }
    motor->setSpeed(motor_speed);  // eliminates over/under error
  }    
  
    int MoveBackwards() {
    switch(motor_state) {
    case BACKWARD:
      break;  // do nothing
    case FORWARD:
      motor->run(RELEASE);
      delay(20);
    case RELEASE:
      motor->run(BRAKE);
      delay(20);
    case BRAKE:
      // acceleration
      motor->setSpeed(MOTOR_PWM_LOWER);
      motor->run(BACKWARD);
      motor_state = BACKWARD;
      int i;
      for (i = MOTOR_PWM_LOWER; i < motor_speed; i+=acceleration){
        motor->setSpeed(i);
        delay(3);
      }
      motor->setSpeed(motor_speed);  // eliminates over/under error
      break;
    }
  }  
  
  void SetSteering(steering_direction steering_dir, int percent = 100) {
    int next_direction;
    switch (steering_dir) {
      case RIGHT:
        next_direction = map(percent, 0, 100, TURN_MID_POS, TURN_RIGHT_POS);
        break; 
      case LEFT:
        next_direction = map(percent, 0, 100, TURN_MID_POS, TURN_LEFT_POS);
        break;
      case MIDDLE:
        next_direction = TURN_MID_POS;
      break;
    }
    steering = next_direction; 
    steering_servo.write(steering);

  }
};


// execute variable declariation
RoboCar car = RoboCar();
  
void setup()  {
  Serial.begin(9600);
  car.Initialize();
  delay(3000);  // put that shit down
} 

void loop()  {
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'l') {
     car.SetSteering(LEFT, 100); 
    }
    else if (c == 'r') {
      car.SetSteering(RIGHT, 100);
    }
    else if (c == 'm') {
      car.SetSteering(MIDDLE);
    }
  }
  delay(50);                      // Wait 50ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.
  unsigned int uS = car.sonar->ping(); // Send ping, get ping time in microseconds (uS).
  unsigned int ultrasound_distance = uS / US_ROUNDTRIP_CM;
  //Serial.println(ultrasound_distance); // Convert ping time to distance in cm and print result (0 = outside set distance range)
   
  if (ultrasound_distance < LOWER_DISTANCE_THRESHOLD) {
    //car.motor_speed = 0;
    //car.motor->setSpeed(BRAKE);
    car.BackThatAssUp();
  }
  else {
    int pid_speed = MOTOR_PWM_UPPER*(min((100*(ultrasound_distance-LOWER_DISTANCE_THRESHOLD))/SPEED_GAIN,100))/100;
    if (pid_speed != 0) {  // scale for indoor use
      car.motor_speed = map(pid_speed, 0, MOTOR_PWM_UPPER, 45, 55);
    }
    else {
      car.motor_speed = 0;
    }
  }
  
  car.MoveForwards();
  //Serial.println(car.motor_speed);

}


