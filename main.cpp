/*
Use this website for reference with asynchronous timers 
https://os.mbed.com/teams/TVZ-Mechatronics-Team/wiki/Timers-interrupts-and-tasks
and
https://os.mbed.com/docs/mbed-os/v6.15/apis/timeout.html
*/

#include "mbed.h"
#include "DS1820.h"
#include "hcsr04.h"
/*
Global modes of operation for the automated smart home.
Mode 0: Normal Mode
Mode 1: Eco Mode
Mode 2: Security Mode
Mode 3: Fire Emergency Mode
Mode 4: Flood Emergency Mode
*/
int mode = 0;

//For the automated garage door
DigitalOut garage_opening_led(p25);
DigitalOut garage_closing_led(p26);
DigitalOut garage_door_led(p27);
HCSR04 usensor(p7,p8);
unsigned int ultrasonic_distance;
int garage_mode;
Timer garage_timer;
int garage_inc;
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
bool alarm_trigger = false;
int alarm_iterator = 0;

//For water sensor
AnalogIn w_sensor(p20);
float water_value;

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
Motion sensing function: 
 If motion is detected it turns on the lights. If the house is in security mode
 it will also sound an alarm if motion is detected.
*/
void pir_sensor(){
    /*
    If the PIR detects motion it turns on the lights for 10 seconds every time 
    it detects motion. 10 seconds without motion turns the lights off
    */
     if (pir){
        house_lights = 1;
        pir_timer.reset();
        if(mode == 2)
            alarm_trigger = true;
     }
     else if(pir_timer.read() > 10)
        house_lights = 0;     
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
    water_value = w_sensor;
    
    if(water_value > 0.05)
        alarm_trigger = true;
}

void garage_door_opener(){
    ultrasonic_distance = usensor.get_dist_cm();
    pc.printf("Distance = %ld cm\r\n", ultrasonic_distance);
    
    if((ultrasonic_distance < 50) && (garage_mode == 1))
        garage_mode = 0; //switches mode to opening
    
    /*
    Opening: 
    Checks if mode is in opening mode.
    Checks timer so it can increment every 0.1 seconds
    Checks garage inc to simulate the servo motor going in increments of 1
    */
    if((garage_mode == 0) && (garage_timer > 0.1) && (garage_inc < 100)){
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
    else if((garage_mode == 1) && (garage_timer > 0.1) && (garage_inc > 0)){
        garage_inc--;
        garage_opening_led = 0;
        garage_closing_led = !garage_closing_led;
        garage_timer.reset();
    } 
    
    //This led just shows if the door is open or closed
    if(garage_inc == 100)
        garage_door_led = 1;
    else if(garage_inc == 0);
        garage_door_led = 0;
        garage_mode = 0;
    else
        garage_door_led = 0;
        
}

int main() {
    pir_timer.start();
    temp_timer.start();
    alarm_timer.start();
    heating_timer.start();
    garage_timer.start();
    usensor.start();
    
    pc.printf("\r\n--Starting--\r\n");
    /*
    This might be the only wait statement in the code.
    I am thinking that we might do a 60 second wait for bootup
    for the final implementation
    */
    wait(20); // Wait for sensor to take snap shot of still room 
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

    
    
    if (ds1820.begin()){
        while(1) {
            pir_sensor();
            smart_heating();
            alarm();
            flood_detector();
            garage_door_opener();
        }
    }
    else
        pc.printf("No DS1820 sensor found!\r\n");
}



