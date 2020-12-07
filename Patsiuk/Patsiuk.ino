/*
* Sketch for SAM "Patsiuk" project
* Fully written by Vitalii Kuzhdin (@vitaliy172), 2020
* For more information, look at shematic
*/

/*=============SETTINGS=============*/
#define timeForRiding 240            //ms, must be 10 cm
#define timeForTurning 1200          //ms, must be 90 degrees
#define minDuty 140                  //motors should start at this speed (0-255)
#define smoothSpeed 50               //ms, time for motors to reach the speed
#define MAX_SPEED 255                //max motor speed (0-255)
#define RIGHT_FRONT_DIRECTION NORMAL //motor direcion, NORMAL or REVERSE
#define RIGHT_BACK_DIRECTION REVERSE //motor direcion, NORMAL or REVERSE
#define LEFT_FRONT_DIRECTION NORMAL  //motor direcion, NORMAL or REVERSE
#define LEFT_BACK_DIRECTION REVERSE  //motor direcion, NORMAL or REVERSE
#define RIGHT_FRONT_MODE HIGH        //change if motor is "on brake" (HIGH or LOW)
#define RIGHT_BACK_MODE HIGH         //change if motor is "on brake" (HIGH or LOW)
#define LEFT_FRONT_MODE HIGH         //change if motor is "on brake" (HIGH or LOW)
#define LEFT_BACK_MODE HIGH          //change if motor is "on brake" (HIGH or LOW)
#define MAX_SONAR_DISTANCE 34463     //max value to get from sonar sensors

/*==========PINS==========*/
#define RIGHT_FRONT_PWM 3
#define RIGHT_FRONT_D 2

#define RIGHT_BACK_PWM 10
#define RIGHT_BACK_D A4

#define LEFT_FRONT_PWM 9
#define LEFT_FRONT_D 4

#define LEFT_BACK_PWM 11
#define LEFT_BACK_D A3

#define RIGHT_TRIG 6
#define RIGHT_ECHO 7
#define RIGHT_SONAR_VCC 5

#define LEFT_TRIG 12
#define LEFT_ECHO 13
#define LEFT_SONAR_VCC 8

#define RX A0
#define TX A1

#define METAL_PIN A5

/*========MESSAGES========*/
#define FOUND_MSG 'Y'
#define NOT_FOUND_MSG 'n'
#define DONE_RIDING_MSG 'e'

/*==================================LIBRARIES==================================*/
#include <NewPing.h>//documentation: https://bitbucket.org/teckel12/arduino-new-ping/wiki/Home
NewPing RIGHT_SONAR(RIGHT_TRIG, RIGHT_ECHO, MAX_SONAR_DISTANCE);
NewPing LEFT_SONAR(LEFT_TRIG, LEFT_ECHO, MAX_SONAR_DISTANCE);

#include <GyverMotor.h>//documentation: https://alexgyver.ru/gyvermotor/
GMotor RIGHT_FRONT(DRIVER2WIRE, RIGHT_FRONT_D, RIGHT_FRONT_PWM, RIGHT_FRONT_MODE);
GMotor RIGHT_BACK(DRIVER2WIRE, RIGHT_BACK_D, RIGHT_BACK_PWM, RIGHT_BACK_MODE);
GMotor LEFT_FRONT(DRIVER2WIRE, LEFT_FRONT_D, LEFT_FRONT_PWM, LEFT_FRONT_MODE);
GMotor LEFT_BACK(DRIVER2WIRE, LEFT_BACK_D, LEFT_BACK_PWM, LEFT_BACK_MODE);

#include <SoftwareSerial.h>//documentation: https://www.arduino.cc/en/Reference/softwareSerial
SoftwareSerial BTserial(RX, TX);

/*==========================GLOBAL VARIABLES==========================*/

//PARSING
boolean doneParsing, startParsing, readMode;
String stringConvert;

//RIDING
boolean joystickMode, stopCarBool;
int X, xDuplicate, Y;

//AVOIDING
byte timesAvoidedX, timesAvoidedY;
boolean avoidedObstacles;

//RETURNING HOME
boolean doneReturning;
int angle, xTravel, yTravel;

void setup() {
    BTserial.begin(9600);

    //D9 and D10 62.5 kHz PWM
    TCCR1A = 0b00000001;
    TCCR1B = 0b00001001;

    //D3 and D11 62.5 kHz PWM
    TCCR2B = 0b00000001;
    TCCR2A = 0b00000011;

    pinMode(METAL_PIN, INPUT);

    pinMode(RIGHT_FRONT_D, OUTPUT);
    pinMode(RIGHT_FRONT_PWM, OUTPUT);

    pinMode(RIGHT_BACK_D, OUTPUT);
    pinMode(RIGHT_BACK_PWM, OUTPUT);

    pinMode(LEFT_FRONT_D, OUTPUT);
    pinMode(LEFT_FRONT_PWM, OUTPUT);

    pinMode(LEFT_BACK_D, OUTPUT);
    pinMode(LEFT_BACK_PWM, OUTPUT);

    pinMode(RIGHT_SONAR_VCC, OUTPUT);
    pinMode(LEFT_SONAR_VCC, OUTPUT);

    digitalWrite(RIGHT_SONAR_VCC, HIGH);//power for right sonar
    digitalWrite(LEFT_SONAR_VCC, HIGH); //power for left sonar

    RIGHT_FRONT.setResolution(8);
    RIGHT_BACK.setResolution(8);
    LEFT_FRONT.setResolution(8);
    LEFT_BACK.setResolution(8);

    RIGHT_FRONT.setDirection(RIGHT_FRONT_DIRECTION);
    RIGHT_BACK.setDirection(RIGHT_BACK_DIRECTION);
    LEFT_FRONT.setDirection(LEFT_FRONT_DIRECTION);
    LEFT_BACK.setDirection(LEFT_BACK_DIRECTION);

    RIGHT_FRONT.setMinDuty(minDuty);
    RIGHT_BACK.setMinDuty(minDuty);
    LEFT_FRONT.setMinDuty(minDuty);
    LEFT_BACK.setMinDuty(minDuty);

    RIGHT_FRONT.setMode(AUTO);
    RIGHT_BACK.setMode(AUTO);
    LEFT_FRONT.setMode(AUTO);
    LEFT_BACK.setMode(AUTO);

    RIGHT_FRONT.setSmoothSpeed(smoothSpeed);
    RIGHT_BACK.setSmoothSpeed(smoothSpeed);
    LEFT_FRONT.setSmoothSpeed(smoothSpeed);
    LEFT_BACK.setSmoothSpeed(smoothSpeed);
}

void loop() {
    parsing();
    if (doneParsing) {
            BTserial.println(NOT_FOUND_MSG);
            xTravel = 0;
            yTravel = 0;
            angle = 0;
            stopCarBool = false;
            avoidedObstacles = false;

        if (joystickMode) {
            int dutyR = Y + X;
            int dutyL = Y - X;

            dutyR = constrain(dutyR, -MAX_SPEED, MAX_SPEED);
            dutyL = constrain(dutyL, -MAX_SPEED, MAX_SPEED);

            RIGHT_FRONT.smoothTick(dutyR);
            RIGHT_BACK.smoothTick(dutyR);
            LEFT_FRONT.smoothTick(dutyL);
            LEFT_BACK.smoothTick(dutyL);

            if (analogRead(METAL_PIN) >= 120) { 
                BTserial.flush();
                BTserial.println(FOUND_MSG);
            } else { //(analogRead(METAL_PIN) < 120)
                BTserial.flush();
                BTserial.println(NOT_FOUND_MSG);
            }

        } else { //(joystickMode == false)
            if (stopCarBool == false) {
                if (noObstacles()) {
                    if (avoidedObstacles == false) {
                        if (Y > 0) {
                            if (X > 0) {
                                if (timesAvoidedX == 0) {
                                    forward();
                                } else {
                                    timesAvoidedX--;  
                                }
                                X--;
                            } else { //(X == 0) change to the next lane
                                timesAvoidedX = 0;
                                if (Y % 2 == 0) { //Y is even
                                    right();
                                    forward();
                                    right();
                                } else { //(Y % 2 != 0) Y is odd
                                    left();
                                    forward();
                                    left();
                                }
                                X = xDuplicate;
                            }
                        
                        } else { //(Y == 0) done riding, return home
                            if (doneReturning == false) {
                                returnHome();
                            }
                        }
                    } else { //(avoidedObstacles)
                        if (timesAvoidedY > 0) {
                            if (angle != 270) {
                                left();
                            } else {
                                forward();
                                timesAvoidedY--;
                            }
                        } else { //(timesAvoidedY == 0)
                            if (angle != 0) {
                                right();
                            } else {
                                avoidedObstacles = false;
                            }
                        }
                    }
                } else { //noObstacles() == false
                    avoidObstacles();
                }   
            } else { //(stopCarBool)
                stopCar();
                Serial.println(FOUND_MSG);
                doneParsing = false;  
            }
            if (stopCarBool == false) {
                Serial.println(DONE_RIDING_MSG);
            }
        }
        doneParsing = false;
    }
}

void avoidObstacles(){
    right();
    forward();
    left();
    timesAvoidedY++;
}

void returnHome() {
    //return home Y
    if (yTravel > 0) {
        if (angle != 180) {
            right();
        } else {
            forward();  
        }
    }
    else if (yTravel < 0) {
        if (angle != 0) {
            right();
        } else {
            forward();
        }
    }
    //return home X
    else if (yTravel < 0) {
        if (angle != 270) {
            right();
        } else {
            forward();  
        }
    }
    else if (xTravel > 0) {
        if (angle != 270) {
            right();  
        } else {
            forward();
        }
    } else {
        doneReturning = true;
    }
}

boolean noObstacles() {
    stopCar();

    unsigned int rightSonarSumm = 0;
    for (byte i = 0; i < 5; i++) {
        rightSonarSumm += RIGHT_SONAR.ping_cm();
        delay(29);
    }
    rightSonarSumm /= 5;

    unsigned int leftSonarSumm = 0;
    for (byte i = 0; i < 5; i++) {
        leftSonarSumm += LEFT_SONAR.ping_cm();
        delay(29);
    }
    leftSonarSumm /= 5;

    if (rightSonarSumm <= 11 or leftSonarSumm <= 11) {
        return false;
    } else {
        return true;
    }
}

void right() {
    RIGHT_FRONT.setSpeed(-MAX_SPEED);
    RIGHT_BACK.setSpeed(-MAX_SPEED);
    LEFT_FRONT.setSpeed(MAX_SPEED);
    LEFT_BACK.setSpeed(MAX_SPEED);

    angle += 90;
    if (angle == 360) {
        angle = 0;
    }

    delay(timeForTurning);
}

void left() {
    RIGHT_FRONT.setSpeed(MAX_SPEED);
    RIGHT_BACK.setSpeed(MAX_SPEED);
    LEFT_FRONT.setSpeed(-MAX_SPEED);
    LEFT_BACK.setSpeed(-MAX_SPEED);

    angle -= 90;
    if (angle == 360) {
        angle = 0;
    }

    delay(timeForTurning);
}

void forward() {
    RIGHT_FRONT.setSpeed(MAX_SPEED);
    RIGHT_BACK.setSpeed(MAX_SPEED);
    LEFT_FRONT.setSpeed(MAX_SPEED);
    LEFT_BACK.setSpeed(MAX_SPEED);

    if (angle == 0 or angle == 90) {
        yTravel++;
    } else { //(angle == 180 or angle == 270)
        xTravel--;
    }

    for (unsigned int i = 0; i < timeForRiding; i++) {
        int metalRead = analogRead(METAL_PIN);
        
        if (metalRead > 120) {
            BTserial.println(FOUND_MSG);//Found!
            stopCarBool = true;
        }
        
        delay(1);
    }
}

void stopCar() {
    RIGHT_FRONT.setSpeed(0);
    RIGHT_BACK.setSpeed(0);
    LEFT_FRONT.setSpeed(0);
    LEFT_BACK.setSpeed(0);
}

/*
* This function reads packets like this '$125,-28@1;',
* where '125,-28' is X and Y
* '1' is mode (1 - joystick, 0 - auto)
*/
void parsing() {
    if (BTserial.available() > 0) {
        doneParsing = false;
        char incomingChar = BTserial.read();

        if (startParsing) {
            if (incomingChar == ',') {
                X = stringConvert.toInt();
                stringConvert = "";
            }
            else if (incomingChar == '@') {
                Y = stringConvert.toInt();
                stringConvert = "";
                startParsing = false;
                readMode = true;
            } else {
                stringConvert += incomingChar;
            }
        }

        else if (readMode) {
            readMode = false;
            joystickMode = incomingChar - '0';
            if (joystickMode == false) {
                X = X * 10 + 1;
                Y = Y * 10 + 1;
                xDuplicate = X;
            }
        }

        else if (incomingChar == '$') {
            startParsing = true;
        }
        else if (incomingChar == ';') {
            doneParsing = true;
        }
    }
}
