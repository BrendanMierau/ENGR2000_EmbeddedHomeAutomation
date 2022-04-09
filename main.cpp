/*
Use this website for reference with asynchronous timers 
https://os.mbed.com/teams/TVZ-Mechatronics-Team/wiki/Timers-interrupts-and-tasks
and
https://os.mbed.com/docs/mbed-os/v6.15/apis/timeout.html
*/

#include "mbed.h"
#include "DS1820.h"
#include "hcsr04.h"
#include "Servo.h"
#include <string> 

/*
Global modes of operation for the automated smart home.
Mode 0: Normal Mode
Mode 1: Eco Mode
Mode 2: Security Mode
Mode 3: Fire Emergency Mode
Mode 4: Flood Emergency Mode
*/

//For the automated garage door
DigitalOut garage_opening_led(p25);
DigitalOut garage_closing_led(p26);
DigitalOut garage_door_led(p27);
HCSR04 usensor(p7,p8);
unsigned int ultrasonic_distance;
int garage_mode;
Timer garage_timer;
int garage_inc;
Servo garage_motor(p24);
/*
Mode 0: open
Mode 1: closed
Mode 2: opening
Mode 3: closing
*/

//For the PIR Motion sensor
DigitalOut house_lights(p15);
DigitalIn pir(p5);
Timer pir_timer;


//For the Smart Temperature control
DigitalOut led3(LED3);
DigitalOut heater_led(p7);
DigitalOut aircon_led(p8);
DS1820 ds1820(p6); // mbed pin name connected to module
Serial pc(USBTX, USBRX);

//Temp timer is for triggering the smart heating logic
Timer temp_timer;

//heating_timer is for the heat or aircon to stay on for 300 seconds
Timer heating_timer;

float temp = 0;
int result = 0;
bool temp_conversion = false;

//For the buzzer alarm
PwmOut buzzer(p22);
float freq[]= {659,440,659,440,659,440,659,440,659,440,659,440};
Timer alarm_timer;
bool alarm_trigger;
int alarm_iterator;

//For water sensor
AnalogIn w_sensor(p20);
float water_value;
Timer flood_timer;

// For Phone App
Serial device(p9, p10);
char app_out;
char app_in;
char alarm_type;
/* 
Alarm types:
F for fire
X for flood
*/

// Different modes our system operates in such as "security", "eco" and "fire"
string system_mode;

// Controls the servo motor to open the window
Servo window_motor(p21);
int window_position;
string mode;
 
void window_open(){
    window_motor = 0.0;
}

void window_close(){
    window_motor = 100.0;
}


/*
Alarm function: 
 Triggers an alarm. Any system that has an alarm uses this alarm
*/
void alarm(){
    //The alarm starts at 50% duty cycle so this silences it
    if(!alarm_trigger){
        buzzer.period(0);
    }
    
    /*
    This flags if the alarm is triggered and then makes a noise every .5 seconds
    and iterates throught the freq[] array for different sounds
    */
    if(alarm_trigger && (alarm_timer.read() > 0.5) && (alarm_iterator < 12)){
        buzzer.period(1/(2*freq[alarm_iterator]));
        alarm_iterator++;
        alarm_timer.reset(); 
    }
    else
        alarm_iterator = 0;
}

/*
House lights
*/
void house_lighting_on(){
    house_lights = 1;
}

void house_lighting_off(){
    house_lights = 0;
}

/*
Motion sensing function: 
 If motion is detected it turns on the lights. If the house is in security mode
 it will also sound an alarm if motion is detected.
*/
void pir_sensor(){
    /*
    If the PIR detects motion it turns on the lights for 10 seconds every time 
    it detects motion. 10 seconds without motion turns the lights off
    */
     if (pir && (system_mode != "eco_mode")){
        house_lighting_on();
        pir_timer.reset();
        if(system_mode == "security_mode")
            alarm_trigger = true;
     }
     else if(pir_timer.read() > 10)
        house_lighting_off();    
}

/*
Smart Heating function: 
 If motion is detected it turns on the lights. If the house is in security mode
 it will also sound an alarm if motion is detected.
*/
void smart_heating(){ 
    /* 
    This detects a fire. Right now it is set to detect a fire at 33 celsius 
    for testing purposes 
    */    
    if(temp > 33){
        alarm_trigger = true;
        alarm_type = 'F';   
    }
    
    //The next two else ifs are to start gathering and reading the heat data from the ds1820
    else if(temp_timer > 7 && !temp_conversion){
        ds1820.startConversion();
        temp_conversion = true;
    }
    else if(temp_timer > 10 && temp_conversion){
        result = ds1820.read(temp); // read temperature
        switch (result) {
            case 0: // no errors
                pc.printf("Temperature= %3.1f C\r\n", temp);  
                break;
            case 1: // no sensor present
                pc.printf("No sensor present\n\r");
            break;
            case 2: // CRC error -> 'temp' is not updated
                pc.printf("CRC error\r\n");
        }
        temp_conversion = false;
        temp_timer.reset();
        led3 = !led3; // indicate update by blinking led
        
        /*
        This checks the heater every (supposed to be 300 seconds) 20 seconds
        and turns on the heater if its too cold or the aircon if it is too hot
        to reach the desired temperature range
        */
        
        if(heating_timer.read() > 20){
            if(temp > 29)
                aircon_led = 1;
            else if(temp < 29)
                aircon_led = 0;
            if(temp < 27)
                heater_led = 1;
            else if(temp > 27)
                heater_led = 0;   
        }  
    }
}

/*
Flood Detecting function: 
 If water is detected it will sound an alarm
*/
void flood_detector(){
    if(flood_timer > 20){
        water_value = w_sensor;
        flood_timer.reset(); 
    }

    
    if(water_value > 0.1){
        pc.printf("FLOOD ALARM \r\n FLOOD ALARM \r\n FLOOD ALARM");  
        alarm_trigger = true;
        alarm_type = 'X';
    }
        
        
}

void garage_door_opener(){
    ultrasonic_distance = usensor.get_dist_cm();
    
    
    //if((ultrasonic_distance < 10) && (garage_mode == 3))
        //garage_mode = 2; //switches mode to opening
    
    /*
    Opening: 
    Checks if mode is in opening mode.
    Checks timer so it can increment every 0.1 seconds
    Checks garage inc to simulate the servo motor going in increments of 1
    */
    if((garage_mode == 2) && (garage_timer > 0.1) && (garage_inc < 100)){
        garage_inc++;
        garage_opening_led = !garage_opening_led;
        garage_closing_led = 0;
        garage_timer.reset();
    }   
    /*
    Closing: 
    Checks if mode is in opening mode.
    Checks timer so it can increment every 0.1 seconds
    Checks garage inc to simulate the servo motor going in increments of 1
    */ 
    else if((garage_mode == 3) && (garage_timer > 0.1) && (garage_inc > 0)){
        garage_inc--;
        garage_opening_led = 0;
        garage_closing_led = !garage_closing_led;
        garage_timer.reset();
    } 
    
    //This led just shows if the door is open or closed
    if(garage_inc >= 100){
        garage_door_led = 1;
        garage_mode = 1;
        }
    else if(garage_inc <= 0){
        garage_door_led = 0;
        garage_mode = 0;
        }
    else
        garage_door_led = 0;
    
    garage_motor = (float) garage_inc/100; 
    //pc.printf("garage motor = %i, Distance = %ld cm \n\r" ,garage_inc, ultrasonic_distance);
}

/*
    This method is the interface between the MIT android application and our mbed
*/
Timer phone_timer;

void phone_app() {
    if (device.readable()) {
        app_out = device.getc();
        device.putc(app_in);
        pc.printf("app signal = %c, app input signal = %c water value = %f\n\r" ,app_out, app_in, water_value);
    }
    switch (app_out){
    case '0': // Open Garage
        garage_inc++;
        garage_mode = 2;
        break; // Close Garage
    case '1':
        garage_inc--;
        garage_mode = 3;
        break;
    case '2': // Eco Mode On
        window_open();
        house_lighting_off();
        system_mode = "eco_mode";
        pc.printf("eco mode activated \r\n");
        break; 
    case '4': // Eco Mode Off
        window_close();
        system_mode = "resting";
        pc.printf("eco mode deactivated \r\n");
        break;
    case '6': // Security Mode On
        system_mode = "security";
        house_lighting_off();
        window_close();
        pc.printf("Security Activated \r\n");
        break;
    case '7': // Security Mode Off
        system_mode = "resting";
        pc.printf("Security Deactivated \r\n");
        break;
    }
    switch (alarm_type)
    {
    case 'F': //Fire
        if(phone_timer > 60){
            phone_timer.reset();
            app_in = 'F';
            pc.printf("Fire detected \r\n");
            }
        else if(phone_timer > 1)
            app_in = 'O';
        break;
    case 'X': // flood
        app_in = 'X';
        pc.printf("Flood detected \r\n");
        break;
    default: //default
        app_in = 'O';
    }
    
}

int main() {
    pir_timer.start();
    temp_timer.start();
    alarm_timer.start();
    heating_timer.start();
    garage_timer.start();
    flood_timer.start();
    phone_timer.start();
    usensor.start();
    window_position = 0;
    
    system_mode = "resting";
    alarm_trigger = false;
    alarm_iterator = 0;
    pc.printf("\r\n--Starting--\r\n");
    /*
    This might be the only wait statement in the code.
    I am thinking that we might do a 60 second wait for bootup
    for the final implementation
    */
    wait(2); // Wait for sensor to take snap shot of still room 
    house_lights = 0;
    heater_led = 0;
    aircon_led = 0;
    ultrasonic_distance = 0;
    garage_mode = 1; //Garage door starts closed
    buzzer = 0.5;
    garage_opening_led = 0;
    garage_closing_led = 0;
    garage_door_led = 0;
    garage_inc = 0;
    alarm_type = '9';
    
    
    if (ds1820.begin()){
        while(1) {
            pir_sensor();
            smart_heating();
            alarm();
            flood_detector();
            garage_door_opener();
            phone_app();
        }
    }
    else
        pc.printf("No DS1820 sensor found!\r\n");
}
