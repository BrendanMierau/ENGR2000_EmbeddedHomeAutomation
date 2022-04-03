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
Timer temp_timer;

//For the Heat sensor
DigitalOut led3(LED3);
DS1820 ds1820(p6); // mbed pin name connected to module
Serial pc(USBTX, USBRX);
float temp = 0;
int result = 0;
bool temp_conversion = false;

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
    if(temp_timer > 7 && !temp_conversion){
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
    }
}

int main() {
    pir_timer.start();
    temp_timer.start();
    pc.printf("\r\n--Starting--\r\n");
    /*
    This might be the only wait statement in the code.
    I am thinking that we might do a 60 second wait for bootup
    for the final implementation
    */
     
    wait(20); //Wait for sensor to take snap shot of still room 
    led1 = 0;
     
    pc.printf("\r\n--Starting--\r\n");
    if (ds1820.begin()){
        while(1) {
            pir_sensor();
            smart_heating();
        }
    }
    else
        pc.printf("No DS1820 sensor found!\r\n");
}




