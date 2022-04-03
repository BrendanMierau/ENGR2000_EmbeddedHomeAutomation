/*
Use this website for reference with asynchronous timers 
https://os.mbed.com/teams/TVZ-Mechatronics-Team/wiki/Timers-interrupts-and-tasks
and
https://os.mbed.com/docs/mbed-os/v6.15/apis/timeout.html
*/

#include "mbed.h"
#include "DS1820.h"
DigitalOut led1(LED1);
DigitalOut led2(LED2);
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


void alarm(){
    if(alarm_trigger && (alarm_timer.read() > 0.5) && (alarm_iterator < 12)){
        pc.printf("alarm trigger true \n\r");
        buzzer.period(1/(2*freq[alarm_iterator]));
        alarm_iterator++;
        alarm_timer.reset();
    }
    else
        alarm_iterator = 0;
}

void pir_sensor(){
     if (pir){
        led1 = 1;
        led2 = 0;
        pir_timer.reset();
     }
     else if(pir_timer.read() > 10){
        led1 = 0;
        led2 = 1;
     }
}

void smart_heating(){
    if(!alarm_trigger){
        buzzer.period(0);
    }
    
    if(temp > 33){
        alarm_trigger = true;    
    }
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

void flood_detector(){
    water_value = w_sensor;
    
    if(water_value > 0.05){
        alarm_trigger = true;
    }
}


int main() {
    pir_timer.start();
    temp_timer.start();
    alarm_timer.start();
    heating_timer.start();
    /*
    This might be the only wait statement in the code.
    I am thinking that we might do a 60 second wait for bootup
    for the final implementation
    */
    pc.printf("\r\n--Starting--\r\n");
    wait(20); //Wait for sensor to take snap shot of still room 
    led1 = 0;
    heater_led = 0;
    aircon_led = 0;
    buzzer = 0.5;
    
    //For the water sensor

    
    
    if (ds1820.begin()){
        while(1) {
            pir_sensor();
            smart_heating();
            alarm();
            flood_detector();
            water_value = w_sensor;
            pc.printf("Value of potentiometer is %f \n\r", water_value);
        }
    }
    else
        pc.printf("No DS1820 sensor found!\r\n");
}





