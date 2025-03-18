#ifndef stepperMotor_H_   /* Include guard */
#define stepperMotor_H_
#define HALL_SENSOR_PIN PIND7 //connected to D38

int homeMotor(void);  //exactly what is i main

void moveStepper(int moveNum,int* stepNumInput);

#endif // stepperMotor_H_