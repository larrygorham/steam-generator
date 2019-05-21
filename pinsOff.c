 /**********************************************************************
* Filename    : pinsOff.c
* Description : Make an led blinking.
* Author      : Robot
* E-mail      : support@sunfounder.com
* website     : www.sunfounder.com
*  gcc pinsOff.c -o pinsOff -lwiringPi
**********************************************************************/
#include <wiringPi.h>
#include <stdio.h>

#define  Pin0    0    //  0   is GPIO17
#define  Pin1    1    //  1   is GPIO18
#define  Pin2    2    //  2   is GPIO27
#define  Pin3    3    //  3   is GPIO22

int main(void)
{
	if(wiringPiSetup() == -1){ //when initialize wiring failed,print messageto screen
		printf("setup wiringPi failed !");
		return 1; 
	}
	//printf("linker Pin0 : GPIO %d(wiringPi pin)\n",Pin0); //when initialize wiring successfully,print message to screen
	
	pinMode(Pin0, OUTPUT);
	pinMode(Pin1, OUTPUT);
	pinMode(Pin2, OUTPUT);
	pinMode(Pin3, OUTPUT);


			digitalWrite(Pin0, LOW);  //water off
			printf(" Pin0 off...\n");
 			delay(1000);

			digitalWrite(Pin1, LOW);  //water off
			printf(" Pin1 off...\n");
 			delay(1000);

			digitalWrite(Pin2, LOW);  //water off
			printf(" Pin2 off...\n");
 			delay(1000);

			digitalWrite(Pin3, LOW);  //water off
			printf(" Pin3 off...\n");
 			delay(1000);
			

	return 0;
}

