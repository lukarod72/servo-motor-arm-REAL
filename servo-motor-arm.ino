
/*
@author: Luka Rodrigues

What is the fucntionality and purpose of this arduino program?

    Purpose:        Program servo motors arm that can be taught FOUR different positions. This arm will cycle through these positions 
                    automatically from boot, starting at a home position.

    Functionailty:  The "Servo Arm" will have the following functionalities:
                        1. Cycle through each postion
                        2. Allow the arm to enter "teach mode", via a push button to teach it desired positions
                            a. teach mode will work as follows:
                                - When the "teach button" is pressed, the arm will stop moving, go back to home position and start looking for secential button pushes
                                - When entering teach mode for the first time, the motors will be listening for position 1. Then when the teach button
                                  is pushed again, the servos will then listen to position 2. And so on for postion 3 and 4. Once position 4 is selected
                                  the arm will start doing those positions
                        3. Allow the arm to enter playback - stops program in current position




Later improvments to allow better user experience:
        1. add LED lights to display to user what mode they are on. 
        2. Hide all wires from user

*/


#include <Servo.h>

const int potentiometerPins[] = {A0, A1, A2, A3}; // Pins for the four potentiometers
const int numPotentiometers = 4;

const int LED_playback = 12;
const int LED_teach = 13;



int potentiometerValues[4] = {0}; // Array to store the saved positions

const int servo_rows = 20;
const int servo_columns = 4;
int current_capacity_of_positions = 0;

int servo_positions[servo_rows][servo_columns]; // Declare a 2D array to store all postions



//I ADDED THIS FOR GIT COMMITING PRACTICE/UNDERSTANDING

const int testing_for_things = 0;//made this to test repo

void new_function(){
    Serial.println(testing_for_things);
}

const int teachButtonPin = 2; // Pin for the "teach button" - Blue
const int playbackButtonPin = 3; // Pin for the "playback button" - Green

Servo servoMotors[4]; // Create an array of 4 Servo objects for the four motors
const int servo_pins[4] = {6, 9, 10, 11};//pin numbers



volatile bool teachMode = false; // Flag to indicate if we are in "teach" mode
volatile bool playbackMode = false;
volatile bool teachButtonEnabled = true;
volatile bool playbackEnable = true;


volatile unsigned long lastTEACHPressTime = 0;
volatile unsigned long lastPLAYBACKPressTime = 0;

const unsigned long debounceDelay = 250; // Adjust this value based on your needs




void setup(){
    Serial.begin(9600);
    // Initialize each element (array) in the servo_positions 2D array with zeros
    for (int i = 0; i < servo_rows; i++) {
        for (int j = 0; j < servo_columns; j++) {
            servo_positions[i][j] = 0;
        }
    }


    //Define pins
    pinMode(teachButtonPin, INPUT_PULLUP);
    pinMode(playbackButtonPin, INPUT_PULLUP);
    pinMode(LED_playback, OUTPUT);
    pinMode(LED_teach, OUTPUT);
    pinMode(7, OUTPUT);
    for(int i = 0; i < numPotentiometers/*4*/; i++){
        pinMode(potentiometerPins[i], INPUT);
        pinMode(servo_pins[i], OUTPUT);

    }



     // Attach interrupts to the buttons
    attachInterrupt(digitalPinToInterrupt(teachButtonPin), teachButtonPressed, FALLING);
    attachInterrupt(digitalPinToInterrupt(playbackButtonPin), playbackButtonPressed, FALLING);
    servoMotors[0].attach(servo_pins[0]);//attach to pin 6, (pin 12 is not PWM)
    servoMotors[1].attach(servo_pins[1]);
    servoMotors[2].attach(servo_pins[2]);
    servoMotors[3].attach(servo_pins[3]);


    //set to home position
    delay(5000);
    home();

}

void loop() {

    //check for teach mode
    if(teachMode && teachButtonEnabled){
        //do teaching methods
        //deatch Interuppts to prevent from any distrubances
        teachButtonEnabled = false;
        playbackEnable = false;
        Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        Serial.println("Teaching mode active. Press the 'teach' button again to save positions: ");
        
        teaching();

        Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        Serial.print("Teach-Mode over, all values have been written. Moving to home position. Total Capacity of positons: ");
        Serial.println(current_capacity_of_positions);
        home();
        delay(2000);//stay at home for 2 seconds, then resume
        teachButtonEnabled = true;
        playbackEnable = true;

    }




    //iterate through each positon
    for(int row = 0; row < current_capacity_of_positions+1; row++){
        if(current_capacity_of_positions <= 0){break;}
        if(teachMode){break;}
        if(playbackMode){stop();}
        for(int col = 0; col < servo_columns; col++){
            int servoAngle = servo_positions[row][col];//fetch servo angle from memory
            servoMotors[col].write(servoAngle);
        }
        Serial.print("Position: ");
        Serial.println(row+1);
        delay(2000);//allow servo motor to move (is not instant)
    }
    delay(100);

}


//Purpose: Go back to home positon. Write zero to all servos
void home(){
    for(int i = 0; i < numPotentiometers; i++){servoMotors[i].write(0);}
}



//Purpose: cycle through the current potentiometer values to see motors turn
void mimic_mode(){
    for (int i = 0; i < numPotentiometers; i++) {
            int potValue = analogRead(potentiometerPins[i]);
            potentiometerValues[i] = potValue;
            servoMotors[i].write(map(potValue, 0, 1023, 0, 180));//write mapped output of potentiometer values (to degrees)
        }
}


//stop in current positon. Infinite loop (breakable)
void stop(){
  //check for playback mode
    if(playbackMode && playbackEnable){
        digitalWrite(LED_playback, HIGH);
        teachButtonEnabled = false;
        playbackEnable = false;  
        Serial.println("Press again to resume motion");
        int trigger = 1;
        while(trigger){
            trigger = digitalRead(playbackButtonPin);
            delay(200);

        }
        teachButtonEnabled = true;
        playbackEnable = true;
        playbackMode = false;
        digitalWrite(LED_playback, LOW);

    }


}

//Purpose: teach servos postions 
void teaching(){
        digitalWrite(LED_teach, HIGH);
        for(int row = 0; row < servo_rows; row++){
            Serial.print("Please enter position number: ");
            Serial.println(row + 1);
            int trigger = 1;//digital button is allways "1"
            int stop_trigger = 1;
            while(trigger && stop_trigger){
                mimic_mode();//put into mimic mode, to see positions of each motor
                trigger = digitalRead(teachButtonPin);
                stop_trigger = digitalRead(playbackButtonPin);
                //Serial.println("In loop");
            }
            digitalWrite(7, HIGH);
            delay(500);
            digitalWrite(7, LOW);

            if(!stop_trigger){
                Serial.println("STOPPING - playback pressed");
                break;//stop teaching was requested
            }

            for(int col = 0; col < servo_columns; col++){
                int current_servo_angle = map(potentiometerValues[col], 0, 1023, 0, 180);//fetch current position, and map to postions
                servo_positions[row][col] = current_servo_angle;//write postions to memory
                Serial.print(col);
                Serial.print(": ");
                Serial.print(current_servo_angle);
                Serial.print('\t'); // Separate with tabs for formatting
            }
            Serial.println(); // Move to the next line for the next row
            current_capacity_of_positions = row;

        }


        // Wait until the Teach Button is released
        while (digitalRead(teachButtonPin) == LOW) {
            delay(50); // Debounce the button
        }
        teachMode = false;
        digitalWrite(LED_teach, LOW);

    


        //re-attach to resume functionallitys        
}





//Purpose: this function is activated when "teach button" is pressed, turns teach mode ons
void teachButtonPressed(){
    unsigned long current_millis = millis();
    if(teachButtonEnabled && !teachMode && current_millis - lastTEACHPressTime >= debounceDelay){
        Serial.println("Teach Button Pressed");
        //if button is enabled and not in teach mode, enable teach mode
        teachMode = true;
        lastTEACHPressTime = current_millis;
    }
    
}

//Purpose: this function writes the values in "potentiometerValues" to the servo motors
void playbackButtonPressed() {
    unsigned long current_millis = millis();
    if(!playbackMode && playbackEnable && current_millis - lastPLAYBACKPressTime >= debounceDelay){
        Serial.println("Playback Button Pressed");
        playbackMode = true;
        lastPLAYBACKPressTime = current_millis;

    }

}







