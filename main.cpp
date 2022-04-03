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


Serial pc(USBTX, USBRX);
DigitalOut led3(LED3);
DS1820 ds1820(p6); // mbed pin name connected to module
float temp = 0;
int result = 0;

int main() {
     pir_timer.start();
     temp_timer.start();
     
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
            ds1820.startConversion();
            wait(1.0); // let DS1820 complete the conversion
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
            led3 = !led3; // indicate update by blinking led
            pir_sensor();
        }  
     }
     else
        pc.printf("No DS1820 sensor found!\r\n");
}

void smart_heating(){
    pc.printf("\r\n--Starting--\r\n");
     if (ds1820.begin()){
        ds1820.startConversion();
        wait(1.0); // let DS1820 complete the conversion
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
        led3 = !led3; // indicate update by blinking led
     }  
     else
        pc.printf("No DS1820 sensor found!\r\n");
}


