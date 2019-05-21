/*
 *  recordH.c:
 *      program to record average humidistat
 *      dht22 sensor
sudo gcc recordH.c -o recordH -lwiringPi -lm
 */
//**********************************************************************

#include <time.h>
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#define MAXTIMINGS      85
#define DHTPIN         2// was 7
int dht22_dat[5] = { 0, 0, 0, 0, 0 };

float avg_dht22_dat(float avgH,float alphaH)
{
        uint8_t laststate       = HIGH;
        uint8_t counter         = 0;
        uint8_t j               = 0, i;
        float   f; /* fahrenheit */
	static int badCounter;
        dht22_dat[0] = dht22_dat[1] = dht22_dat[2] = dht22_dat[3] = dht22_dat[4] = 0;

        /* pull pin down for 18 milliseconds */
        pinMode( DHTPIN, OUTPUT );
        digitalWrite( DHTPIN, LOW );
        delay( 18 );
        /* then pull it up for 40 microseconds */
        digitalWrite( DHTPIN, HIGH );
        delayMicroseconds( 40 );
        /* prepare to read the pin */
        pinMode( DHTPIN, INPUT );

        /* detect change and read data */
        for ( i = 0; i < MAXTIMINGS; i++ )
        {
                counter = 0;
                while ( digitalRead( DHTPIN ) == laststate )
                {
                        counter++;
                        delayMicroseconds( 1 );
                        if ( counter == 255 )
                        {
                                break;
                        }
                }
                laststate = digitalRead( DHTPIN );

                if ( counter == 255 )
                        break;

                /* ignore first 3 transitions */
                if ( (i >= 4) && (i % 2 == 0) )
                {
                        /* shove each bit into the storage bytes */
                        dht22_dat[j / 8] <<= 1;
                        if ( counter > 16 )
                                dht22_dat[j / 8] |= 1;
                        j++;
                }
        }

        /*
         * check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
         * print it out if data is good
         */
        if ( (j >= 40) &&
             (dht22_dat[4] == ( (dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3]) & 0xFF) ) )
        {
         float t, h;
        h = (float)dht22_dat[0] * 256 + (float)dht22_dat[1];
        h /= 10;
        t = (float)(dht22_dat[2] & 0x7F)* 256 + (float)dht22_dat[3];
        t = t*9./50.+ 32.;
        if ((dht22_dat[2] & 0x80) != 0)  t *= -1;

		avgH = (h - avgH) * alphaH + avgH;
		badCounter = 0;
		//printf("Humidity = %.2f %% Temperature = %.2f *C \n", h, t );
        }else  {
                //printf( "Data not good, skip\n" );
        	if(++badCounter >= 30)
			avgH = 0.;
	}
	//printf(" badCounter = %d\n",badCounter);
	return avgH;  
}


int main( void )
{
        FILE *averageH_file;
        FILE *recordH_file;

	printf( "Average humidity \n" );
	
	time_t cur_time;
	char *cur_t_string;
	int initCtr = 0;
	int hungCtr = 0;
	float avgH = 30.0;
	float avgOldH = avgH;
	float alphaH = .2;
	float delH;
        if ( wiringPiSetup() == -1 )
                exit( 1 );

        while ( 1 ){
		cur_time = time(NULL); // defined as int
          	cur_t_string = ctime(&cur_time);
		avgH = avg_dht22_dat(avgH,alphaH);
		delH = fabs(avgH - avgOldH);
		//printf("delH = %.7f  initCtr = %d ",delH,initCtr);
		++initCtr;
		if(initCtr > 10 && delH > 8.0){
			avgH = avgOldH;
			delH = 0.;
		}
                if(initCtr > 20 && delH > 4.5){
                        avgH = avgOldH;
			delH = 0.;
		}

                if(initCtr > 30 && delH > 1.0){
                        avgH = avgOldH;
			delH = 0.;
		}

		averageH_file = fopen("/home/pi/boot/averageH.txt", "w");
		fprintf(averageH_file,  "%.2f  ", avgH);
		fclose(averageH_file);
		if(delH < .0000001){
			++hungCtr;
		}else{
			hungCtr = 0;
		}
		avgOldH = avgH;

		if(avgH <= 0.001 || hungCtr >= 30){
			printf(" Shutdown -- Humidistat failure \n ");
			return(1);
		}
                delay( 2000 ); /* wait 2 sec to refresh */
        }

        return(0);
}


