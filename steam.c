/*
 *  steam.c:
 *  Program to Operate the steam generator
 *    and humidify the house
 *
 * Copyright (c) 2019  https://github.com/larrygorham/LICENSE
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms  the License.
 *
sudo gcc steam.c -o steam -lwiringPi -lm
 ***********************************************************************
 * Requires: wiringPi (http://wiringpi.com)
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

#define	TRUE	            (1==1)
#define	FALSE	            (!TRUE)
#define CHAN_CONFIG_SINGLE  8
#define CHAN_CONFIG_DIFF    0
#define powerPin    0    //  0   is GPIO17
#define waterPin    1    //  1   is GPIO18

static int myFd ;

char *usage = "Usage: mcp3008 all|analogChannel[1-8] [-l] [-ce1] [-d]";
// -l   = load SPI driver,  default: do not load
// -ce1  = spi analogChannel 1, default:  0
// -d   = differential analogChannel input, default: single ended
//
//---------------------------------------------------------------------------
void loadSpiDriver()
{
    if (system("gpio load spi") == -1)
    {
        fprintf (stderr, "Can't load the SPI driver: %s\n", strerror (errno)) ;
        exit (EXIT_FAILURE) ;
    }
}
//-----------------------------------------------------------------------------------------
void spiSetup (int spiChannel)
// Max speed 16 mHz now set to 1 mHz  gives 25kHz sampling
//    500 kHz gives 12.5 kHz sampling
{
    if ((myFd = wiringPiSPISetup (spiChannel, 62500)) < 0)
    {
        fprintf (stderr, "Can't open the SPI bus: %s\n", strerror (errno)) ;
        exit (EXIT_FAILURE) ;
    }
}
//-------------------------------------------------------------------------------------
int myAnalogRead(int spiChannel,int channelConfig,int analogChannel)
	/*
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 */

{
    if(analogChannel<0 || analogChannel>7)
        return -1;
    unsigned char buffer[3] = {1}; // start bit
    buffer[1] = (channelConfig+analogChannel) << 4;
    wiringPiSPIDataRW(spiChannel, buffer, 3);
    return ( (buffer[1] & 3 ) << 8 ) + buffer[2]; // get last 10 bits
}
//---------------------------------------------------------------------------------------------
void getData(float *sampAvgC, float *rmsC, float *sampAvgV, float *rmsV, int *avgNum,int spiChannel,int channelConfig,int analogChannel)
{
float readC[*avgNum];
float readV[*avgNum];
float squarAvgC = 0.0;
float squarAvgV = 0.0;
int i;
*sampAvgC = 0.0;
*sampAvgV = 0.0;
for(i=0;i<(*avgNum);i++)
  {
  readC[i] = myAnalogRead(spiChannel,channelConfig,analogChannel-1);  
  *sampAvgC += readC[i];
  squarAvgC += readC[i]*readC[i];
  analogChannel += 1;
  readV[i] = myAnalogRead(spiChannel,channelConfig,analogChannel-1);  
  *sampAvgV += readV[i];
  squarAvgV += readV[i]*readV[i];
  analogChannel -= 1;
  }
*sampAvgC /= (*avgNum);
squarAvgC /= (*avgNum);
*rmsC = sqrt((squarAvgC)-(*sampAvgC)*(*sampAvgC));
*sampAvgV /= (*avgNum);
//squarAvgV /= (*avgNum);      not doing rms here
//*rmsV = sqrt((squarAvgV)-(*sampAvgV)*(*sampAvgV));  not doing rms here
//printf(" %f %f %f %f %d %d %d %d\n",readC[i],*sampAvg,*squarAvg,*rms,*avgNum,spiChannel,channelConfig,analogChannel);
}

//---------------------------------------------------------------------------------------------
void addWater(int *floodFlag, int *endTime, int *endTimeM, int addTime, int pin1, int spiChannel,int channelConfig)
{
// one sec  water shot
digitalWrite(waterPin, LOW);  //led on
delay(1);
digitalWrite(waterPin, HIGH);  //led off
delay(1000);
digitalWrite(waterPin, LOW);  //led on
delay(1);
*floodFlag = 0;
*endTime = 0;
*endTimeM = 0;

}

//***********************************************************************

void getHumFileName(void *tempFileName)
{
FILE *eventTimes_file;
int numbers[] = {'0','1', '2', '3', '4', '5','6','7','8','9'};
char preFileName[] = "/home/pi/boot/hum"; //prefix file name

int j,temp; //Used in conversion
int temp2,temp3,runNum;

eventTimes_file = fopen("/home/pi/boot/eventTimes.txt", "r");
fscanf(eventTimes_file, "%d %d %d", &temp2,&temp3,&runNum);
fclose(eventTimes_file); 	
if(runNum == 9)
	runNum = -1;
runNum++;

eventTimes_file = fopen("/home/pi/boot/eventTimes.txt", "w");
fprintf(eventTimes_file, "%d %d %d", temp2,temp3,runNum);
fclose(eventTimes_file); 

preFileName[17] = numbers[runNum];

sprintf(tempFileName, "%.18s.txt", preFileName); //size of preFileName + 1, Attach the suffix	
return ;
}

//**************************************************


void getNewFileName(char tempFileName[], int tempOld)
{
	//**  This  converts current int time to char time into seconds
        int numbers[] = {'0','1', '2', '3', '4', '5','6','7','8','9'};
	//char seconds[10]; //Time in seconds
	char preFileName[24] = "/home/pi/boot/"; //prefix file name

	int j,temp; //Used in conversion

	for(j=0; j <= 9; j++)
		{
		temp = (int)(tempOld/10.);
		preFileName[23-j] = numbers[tempOld - temp*10];
		tempOld = temp;
		}
	sprintf(tempFileName, "%.24s.txt", preFileName); // Attach the suffix	
	return ;
}


//************************************************** MAIN **************************
void main (){
FILE *temp_file;
FILE *log_file;
FILE *settings_file;
FILE *eventTimes_file;
FILE *averageH_file;
int loadSpi=FALSE;
int spiChannel=0;
int analogChannel=1;
int channelConfig=CHAN_CONFIG_SINGLE;
int floodFlag, endTime, endTimeM, addTime,runNum;
int powerOnTime = 0;
int powerOffTime = 0;
int powerOnTotal = 0;
int powerOffTotal = 0;
int purgeTime = 0;
int totalShots = 0;
int i;
float onOffRatio;
float hiLimit;//50. 20. 9.
time_t cur_time;
time_t electrodePowerOffTime;
char* cur_t_string;

//
if(loadSpi==TRUE)
    loadSpiDriver();
wiringPiSetup () ;
int avgNum=320; //was 1282 320 is  ~2 sec avg  number raw samples 
float avgNumf=(float)avgNum;  //  ~2.0 sec  averageC 
int blockSize = 100;  //5  peak capture block in sample averageCs 5 10 20 
int samplesPreShot = 50; //150 3  number sample averageCs befor shot 1  4  4
int sampleMaxNum; // Sample number for the max current
int waterFlag = 1;   //  when high water is detected = 1
int powerLostCtr= 0; //  when power is not detected on the electrodes
int powerOn = 0;    //  when power is on the electrodes
int cur_timeLast = 0; // previous loop time
int powerOnOld = 0; // previous loop power
int hiCurCtr = 0; // counts loops above hiCurCtr
int preShotCurAvgNum = 5; //Number samples in preShotCurAvg


float lowLimit = 3.0;  // was 3.0
float cHys = 5.; //  current hysteresis  10.
float hThreshold;
float hHys = .5;      // humidity hysteresis 1.5
float dac;       // dac read in 
float averageC[blockSize];  // sample averageCs
float averageV[blockSize];  // sample averages squared
float maxCurLast = 11.; //
float maxPreDiff = 0.;
float maxPreDiffAvg = 0.;
float cRms; //current rms in block
float sampAvg, squarAvg;
float preShotCur = 0.; // Current average before shot, took last value due to noise 
float preShotCurLast = 0.; //
float preShotDiff = 0.;
float preShotDiffAvg = 0;
float preShotCurAvg = 0.; // current average just before water added
float hiCurPurgeTest; //90. High current test point
float hiLow; //Percentage current increase after shot
float maxCurAvg = 0.0;
float alpha = .3333;
float sampAvgC, rmsC, rmsV;
float sampAvgV = 0.0;
float blockAvgV;
    
pinMode(powerPin, OUTPUT);
pinMode(waterPin, OUTPUT);
int purgePlus27, temp1, temp2, temp3, temp4,temp5;
settings_file = fopen("/home/pi/boot/settings.txt", "r");
int loopNum = 0;
int logStartTime = 0;
int j, k;
int purgeFlag = 0;
float avgH;
float avgHOld = 0.;
int stuckCtrH = 0;
int firstShot = 0; // first water shot, it's 1
int floodCtr = 0; // Count successive water adds 

//static char fileName[28];// for getNewFileName epoch.txt files
static char fileName[22];// for getHumFileName 10 daily hum*.txt files

temp_file =fopen("/home/pi/boot/temp", "w");
fclose(temp_file);  
digitalWrite(powerPin, LOW);  //Power off, unintentionally on
spiSetup(spiChannel);
cur_time = time(NULL);	
logStartTime = 0;
for(;;){
	eventTimes_file = fopen("/home/pi/boot/eventTimes.txt", "r");
	fscanf(eventTimes_file, "%d %d %d", &powerOffTime,&purgeTime,&runNum);
	fclose(eventTimes_file); 	
	settings_file = fopen("/home/pi/boot/settings.txt", "r");
	fscanf(settings_file, "%f %f %d %f %f,",&hiLimit,&hiCurPurgeTest,&purgePlus27,&hHys,&hThreshold);
	fclose(settings_file);
	averageH_file = fopen("/home/pi/boot/averageH.txt", "r");
	fscanf(averageH_file, " %f,", &avgH);
	fprintf(averageH_file,  "%.2f  ", avgH);
	fclose(averageH_file);
	spiSetup(spiChannel);
	cur_time = time(NULL);	
	if(fabs(avgH - avgHOld) < .000001) //   Humidistat testing    
		stuckCtrH++;
	else
		stuckCtrH = 0;
	avgHOld = avgH;
	if(avgH + hHys < hThreshold && powerOn == 0){
		powerOn = 1;
		digitalWrite(powerPin, HIGH);  //Power on
		powerOnTime = cur_time;
		eventTimes_file = fopen("/home/pi/boot/eventTimes.txt", "r");
		fscanf(eventTimes_file, "%d %d %d", &temp2,&temp3,&runNum);
		fclose(eventTimes_file); 
		//printf("******* ELECTRODE POWER ON *********\n");
		}
	if(avgH - hHys > hThreshold && powerOn == 1){
		powerOn = 0;
		digitalWrite(powerPin, LOW);  //Power off
		powerOffTime = cur_time;
		eventTimes_file = fopen("/home/pi/boot/eventTimes.txt", "r");
		fscanf(eventTimes_file, "%d %d %d", &temp2,&temp3,&runNum);
		fclose(eventTimes_file); 
		eventTimes_file = fopen("/home/pi/boot/eventTimes.txt", "w");
		fprintf(eventTimes_file, "%d %d %d\n", powerOffTime,temp3,runNum);
		fclose(eventTimes_file); 
		//printf("******* ELECTRODE POWER OFF *********\n");
		}
	if(cur_time >= logStartTime + 86400){ //  86400 ******************************************
		powerOnTotal = 0;
		powerOffTotal = 0;
		cur_t_string = ctime(&cur_time); //convert to local time format
		//printf("%d  %s \n",cur_time, cur_t_string);
		//getNewFileName(fileName,cur_time);
		getHumFileName(fileName);
		//printf("File name %s \n", fileName);
		logStartTime = cur_time;
		log_file =fopen(fileName, "w");
		temp_file =fopen("/home/pi/boot/temp", "a");
		//printf("%d  %s \n",cur_time, cur_t_string);
		eventTimes_file = fopen("/home/pi/boot/eventTimes.txt", "r");
		fscanf(eventTimes_file, "%d %d %d", &temp2,&temp3,&runNum);
		fclose(eventTimes_file); 	
		powerOffTime = temp2;
		//printf("   ELECTRODE POWER OFF since %d\n",powerOffTime);
		fprintf(log_file,"%d  %s \n",cur_time, cur_t_string);
		fprintf(log_file,"hiLimit %.1f purgePlus27 %d hHys %.1f hThreshold %.1f\n",
			hiLimit,purgePlus27,hHys,hThreshold);
		fprintf(temp_file,"File name %s \n", fileName);
		fprintf(temp_file,"%d  %s \n",cur_time, cur_t_string);
		fprintf(temp_file,"hiLimit %.1f hiCurPurgeTest %.1f purgePlus27 %d hHys %.1f hThreshold %.1f\n",
			hiLimit,hiCurPurgeTest,purgePlus27,hHys,hThreshold);
		fclose(log_file);
		fclose(temp_file);
		loopNum = 0;
		totalShots = 0;
	}
	temp_file =fopen("/home/pi/boot/temp", "a");
	log_file =fopen(fileName, "a");
	loopNum += 1;
	if (loopNum == 1)
		cur_timeLast = cur_time;		
//	preShotCur = 0.0;
	preShotCurAvg = 0;
	blockAvgV = 0.;
	if(powerOn == 1){ // Collect data prior to the time water shot may be added
		for(i = 0; i < samplesPreShot; i++){
			//dac = myAnalogRead(spiChannel,channelConfig,analogChannel-1);
        		//printf("  dac = %f\n", dac);
        		//printf(" %d %d %d\n",spiChannel,channelConfig,analogChannel); 
        		getData(&sampAvgC,&rmsC,&sampAvgV,&rmsV,&avgNum,spiChannel,channelConfig,analogChannel);   
			averageC[i] = rmsC;
        		averageV[i] = sampAvgV; // Hi point voltage
			blockAvgV += sampAvgV;
			preShotCur = averageC[i];
//printf("i %d averageC[i] %f preShotCur %f \n",i, averageC[i], preShotCur); //*************
        	}
		blockAvgV /= samplesPreShot;
		for (j=1; j <= preShotCurAvgNum; j++){ // 5 Current average at end of preBlock
			preShotCurAvg += averageC[samplesPreShot - j];
//fprintf(temp_file,"j %d preShotCurAvg %f\n",j, preShotCurAvg);
		}
		preShotCurAvg /= preShotCurAvgNum;

	}
	if(firstShot == 0){   
//		maxCurAvg = preShotCurAvg;
	}

	if(powerOn == 1){  //  Test conditions
		if(preShotCurAvg< lowLimit){  // Loss of power 
			++powerLostCtr;
		}
		if(preShotCurAvg>= lowLimit && powerLostCtr> 0){  // Power is detected on 
			powerLostCtr= 0;
		}
		if(blockAvgV > hiLimit){   //  Water has reached the Hi sensor
			waterFlag = 0;
		}else{
			waterFlag = 1;
		}
	}

	if(powerOn == 1){ //  OK for power	
		if(waterFlag == 1){ // Add shot water this section
			addTime = time(NULL);
			system("/home/pi/boot/add2water &");
			++totalShots;
			++firstShot;
			++floodCtr;
			maxCurAvg = 12.;//initialize
			for(i = samplesPreShot; i < blockSize; i++){ 
				getData(&sampAvgC,&rmsC,&sampAvgV,&rmsV,&avgNum,spiChannel,channelConfig,analogChannel);   
				averageC[i] = rmsC;
				averageV[i] = sampAvgV ; //  rmsV
				rmsV = sampAvgV;
				if(maxCurAvg < averageC[i]){
					maxCurAvg = averageC[i];
					sampleMaxNum = i;
				}
//printf("i %d %d avgC[i] %f avgeV[i] %f preShotC %f \n",i, addTime, averageC[i],averageV[i], preShotCur); //*************
 			}
			if(sampleMaxNum < blockSize-1 && sampleMaxNum > 2 )
				maxCurAvg = (averageC[sampleMaxNum - 1] + averageC[sampleMaxNum] + 
				averageC[sampleMaxNum + 1])/3.;
		}//*************
		
	}// end this power on section
	if(powerOn == 0 || waterFlag == 0){
		floodCtr = 0;
	}
	//printf(" %d  C =  %f   V = %f\n", i, averageC[i],averageV[i]);  
	//****************   Purge ***************
	if(purgeFlag == 1 && cur_time > purgeTime + 3600){
		//printf("   ***** Purge in progress wait 5 minutes was 10  *****\n");
		digitalWrite(powerPin, LOW);  //
                powerOffTime = cur_time;
                eventTimes_file = fopen("/home/pi/boot/eventTimes.txt", "r");
		fscanf(eventTimes_file, "%d %d %d", &temp2,&temp3,&runNum);
                fclose(eventTimes_file);
                eventTimes_file = fopen("/home/pi/boot/eventTimes.txt", "w");
                fprintf(eventTimes_file, "%d %d %d\n", powerOffTime,temp3,runNum);
                fclose(eventTimes_file);
		//printf("   ELECTRODE POWER OFF...\n");
		delay(10);   // 
		digitalWrite(waterPin, HIGH);  //
		//printf("   water on  \n");
 		delay(purgePlus27); //7000
        		digitalWrite(waterPin, LOW);  //
		delay(1000);
		digitalWrite(waterPin, HIGH);  //
		delay(10000);
		digitalWrite(waterPin, LOW);  //
		delay(1000);
		digitalWrite(waterPin, HIGH);  //
		delay(17000);
		digitalWrite(waterPin, LOW);  //        	
		//printf("   purging...\n");//*******************************************************
		//printf("   Drain delay  ...\n");
		digitalWrite(powerPin, HIGH);  //
                powerOnTime = cur_time;
		delay(300000);  // 5 min Allow siphon to fully drain ****************************
		settings_file = fopen("/home/pi/boot/settings.txt", "r");
		fscanf(settings_file, "%f %f %d %f %f,",&hiLimit,&hiCurPurgeTest,&purgePlus27,&hHys,&hThreshold);
		fclose(settings_file);
		purgeFlag = 0; 
		cur_time = time(NULL);	
		purgeTime = cur_time;
		eventTimes_file = fopen("/home/pi/boot/eventTimes.txt", "r");
		fscanf(eventTimes_file, "%d %d %d", &temp2,&temp3,&runNum);
		fclose(eventTimes_file); 
		eventTimes_file = fopen("/home/pi/boot/eventTimes.txt", "w");
		fprintf(eventTimes_file, "%d %d %d\n", temp2,purgeTime,runNum);
		fclose(eventTimes_file); 
		//printf("   *****  Purge Complete ELECTRODE POWER ON  *****\n");
		fprintf(log_file,"Purge complete %s \n", cur_t_string);
		fprintf(temp_file,"Purge complete %s \n", cur_t_string);
		hiCurCtr = 0;
		digitalWrite(powerPin, LOW);  
	}
	//printf("  %f \n", sampAvgC); //  DAC mean, not current ~ 492 
	//printf("Power On      %d, Off       %d \n",powerOnTime,powerOffTime); 
	close (myFd) ; 
	cur_t_string = ctime(&cur_time); //convert to local time format
	char sTime[8]; //short format current time
	for (j=1; j <= 8; j++)
		sTime[j-1] = cur_t_string[j+10];

	if(powerOn == 1 && powerOnOld == 0){
		powerOffTotal += cur_time - cur_timeLast;
		powerOnOld = 1;
		cur_timeLast = cur_time;
	}

	if(powerOn == 0 && powerOnOld == 1){
		powerOnTotal += cur_time - cur_timeLast;
		powerOnOld = 0;
		cur_timeLast = cur_time;
	}
	if ( powerOffTotal > 0)
		onOffRatio = (float)powerOnTotal/(float)powerOffTotal;
	else
		onOffRatio = 0;

	preShotDiffAvg = preShotCurAvg - preShotCurLast;

	preShotCurLast = preShotCurAvg;


	if(powerOn == 1 || loopNum%4 == 0){
		fprintf(temp_file,"%4d %d %d %5.2f %7.2f %d %6.1f %6.1f %d %6.3f %d %.8s\n",
		loopNum,cur_time,powerOffTime,onOffRatio,preShotCurAvg,powerOn,
		maxCurAvg,blockAvgV,waterFlag,avgH,totalShots,sTime);

		fprintf(log_file,"%4d %d %d %5.2f %7.2f %d %6.1f %6.1f %d %6.3f %d %.8s\n",
		loopNum,cur_time,powerOffTime,onOffRatio,preShotCurAvg,powerOn,
		maxCurAvg,blockAvgV,waterFlag,avgH,totalShots,sTime);
	}

	if(stuckCtrH >= 100){ // Humidity Sensor Failure 
		digitalWrite(powerPin, LOW);  //Power off
		fprintf(temp_file," Shutdown -- Humidity Sensor Failure \n ");
		fprintf(log_file," Shutdown -- Humidity Sensor Failure \n ");
		fclose(temp_file);  
		fclose(log_file); 
		return;
	}

	if(floodCtr >= 40){ // 80 for 1 sec shots Too much water added  
		digitalWrite(powerPin, LOW);  //Power off
		fprintf(temp_file," Shutdown -- Flood failure \n ");
		fprintf(log_file," Shutdown -- Flood failure \n ");
		fclose(temp_file);  
		fclose(log_file); 
		return;
	}
	if(powerLostCtr >= 4){ // Power Lost   
		digitalWrite(powerPin, LOW);  //Power off
		fprintf(temp_file," Shutdown -- Power Lost failure \n ");
		fprintf(log_file," Shutdown -- Power Lost failure \n ");
		fclose(temp_file);  
		fclose(log_file); 
		return;
	}

	if(powerOn == 0){
		delay(15000);
	}
	fclose(temp_file);  
	fclose(log_file); 
	if(powerOn == 1){
		if(preShotCurAvg > hiCurPurgeTest){
			++hiCurCtr;
			if(hiCurCtr >= 3 && cur_time > purgeTime + 3600)
				purgeFlag = 1;
		}else
			hiCurCtr = 0;
	}
} // infinite loop end
} // main end

