#!/bin/bash
 
# This only prints new date from the last line of temp
i="0"
MY_OLD_VAR='test'
while [ $i -lt 1000000 ]
do
	tail -n -1 temp > testVar.txt
	MY_NEW_VAR=$(cat  testVar.txt | sed -e 's/\r//g')
	if [ "$MY_OLD_VAR" != "$MY_NEW_VAR" ]
	then
		echo "${MY_NEW_VAR}"
		MY_OLD_VAR=${MY_NEW_VAR}
	fi
	sleep 12
	i=$[$i+1]
done




