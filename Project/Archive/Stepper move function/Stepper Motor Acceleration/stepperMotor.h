#ifndef stepperMotor_H_   /* Include guard */
#define stepperMotor_H_
#define HALL_SENSOR_PIN PIND7 //connected to D38

const char motorSteps[4];

int homeMotor(void);  //exactly what is i main

int moveStepper(int moveNum, int stepNum;);

#endif // stepperMotor_H_