Steam Generator Humidifier
By: larry_gorham@hotmail.com

This project contains all the software to deploy the humidifier. Hardware construction 
is located on constructable.com.
*************************************************************************************************
Get recent Raspberry Pi OS updates with:
sudo apt-get update
sudo apt-get upgrade

Enable the SPI interface with:
sudo raspi-config

**************************************************************************
Create a directory:
/home/pi/boot/

From https://github.com/larrygorham/steam-generator
 copy all *.c and *.txt into the boot directory:

Compile all *.c in the boot directory using the following format:
sudo gcc steam.c -o steam -lwiringPi -lm

chmod all *.sh files to executabled

***********************************************************************************************
The top level is the boot service:
Create a systemd service file /etc/systemd/system/boot.service 
Here it is called boot:

sudo touch /etc/systemd/system/boot.service
sudo chmod 664 /etc/systemd/system/boot.service

Open the boot.service file and add the minimal service configuration options that allow 
this service to be controlled via systemctl:

[Unit]
Description=test boot

[Service]
ExecStart=/home/pi/boot/steam.sh

[Install]
WantedBy=multi-user.target


Once the service file is changed, it needs to reload systemd configuration:

sudo systemctl daemon-reload

Now start, stop, restart and check the service status:

sudo systemctl start boot
sudo systemctl stop boot
sudo systemctl restart boot
systemctl status boot

To configure a service to start automatically on boot, you need to enable it:

sudo systemctl enable boot

To check the service logs, run:

journalctl -u boot
***********************************************************************************************************
so:
sudo systemctl start boot

calls the script file steam.sh

The script starts recordH.exe in a separate process
waits two seconds and
starts steam.exe the executive program which controls the humidifier
***********************************************************************************************************
When using the boot service all references must be direct, e.g., calling the exec with
/home/pi/boot/steam

The pinsOff.c program is not normally needed. If the cpu crashes or some malfunction occurs during testing while the electrodes are energised, they may stay energised. No harm will come of this since the steam generator will simply boil away the electrolyte and
stop. During testing it's a good idea to follow the command
sudo systemctl stop boot
with
sudo ./pinsOff
which de-energises all digital outputs and kills the electrodes.
********************************************************************************************
Operating the Humidifier
Startup is considered the first time the Humidifier is energized or subsequent to annual 
cleaning of the SG. Look at the settings.txt file in the boot directory to verify the starting
 settings.

The settings.txt file contains 4 columns with the following information:
1. Scaled voltage when electrolyte reaches the High Level sensor (~100.)
2. Scaled current for Purge initiation (~80.)
3. Additional Purge time in milliseconds beyond 27 seconds (~5000)
4. Humidity hysteresis in percent (~0.5)
5. Humidity in percent (~48.)


Fill the SG with tap water about ¾ full and add a pinch of ordinary salt if the tap water is
 relativity soft. Install all parts in position and energise the Humidifier. After about 
5 minutes go to the boot directory and type “sudo ./run.sh”. This will display the latest 
log file entry and continue to do so once per minute. 


The Log File contains 12 columns with the following information:
1. Index number
2. Epoch Time of Update
3. Epoch Time when electrodes were last de-energised.
4. Ratio of electrode ‘On Time’ to ‘Off Time’
5. Electrode current before water injection
6. Power on flag
7. Electrode current during after water injection
8. Hi level sensor voltage
9. Water this update flag
10. Humidity in return duct
11. Number of water shots added (0.2 cups/shot)
12. Local Time

Both electrode currents should increase as the SG fills and heats. Current should stop 
when the humidity reaches the set point. A purge should occur when the current reaches 
that condition for three cycles and the SG is full. Verify that no foaming is present by
 observing the lid. If that happens the lid will be doused with liquid from the cracks around 
the electrodes  Foaming can allow liquid droplets to leave the steam pipe. If foaming does 
occur reduce the Purge Initiation current which will reduce the electrolyte concentration and
 stop the foaming. When the humidifier runs successfully with no foaming it can be left without
 further attention for a year. Monitoring the Log file periodically is a good idea for the firs
 month. When shutting down, It’s not enough to just stop the boot process since the electrodes
 may continue to be energised. Executing pinsOff.exe will ensure all power is off. Remember
 that the Humidifier will boot up and run whenever power is cycled back on unless the boot
 command is deactivated. Happy humidifying!
************************************************************************************************************







