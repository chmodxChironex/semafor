#!/bin/bash

L_start="^[1-9][0-9]*: L [1-9][0-9]*: started$"
L_start_n=0

L_z="^[1-9][0-9]*: L [1-9][0-9]*: arrived to [1-9][0-9]*$"
L_z_n=0

L_boarding="^[1-9][0-9]*: L [1-9][0-9]*: boarding$"
L_boarding_n=0

L_ski="^[1-9][0-9]*: L [1-9][0-9]*: going to ski$"
L_ski_n=0

BUS_start="^[1-9][0-9]*: BUS: started$"
BUS_start_n=0

BUS_finish="^[1-9][0-9]*: BUS: finish$"
BUS_finish_n=0

BUS_ar_fin="^[1-9][0-9]*: BUS: arrived to final$"
BUS_ar_fin_n=0

BUS_lv_fin="^[1-9][0-9]*: BUS: leaving final$"
BUS_lv_fin_n=0

BUS_ar_z="^[1-9][0-9]*: BUS: arrived to [1-9][0-9]*$"
BUS_ar_z_n=0

BUS_lv_z="^[1-9][0-9]*: BUS: leaving [1-9][0-9]*$"
BUS_lv_z_n=0
while read line
do
  if echo $line | grep "$L_start" >/dev/null; then
  	L_start_n=$(($L_start_n + 1))
  elif echo $line | grep "$L_z" >/dev/null; then
  	L_z_n=$(($L_z_n + 1))
  elif echo $line | grep "$L_boarding" >/dev/null; then
  	L_boarding_n=$(($L_boarding_n + 1))
  elif echo $line | grep "$L_ski" >/dev/null; then
  	L_ski_n=$(($L_ski_n + 1))
   elif echo $line | grep "$BUS_start" >/dev/null; then
   	BUS_start_n=$(($BUS_start_n + 1))
   elif echo $line | grep "$BUS_finish" >/dev/null; then
   	BUS_finish_n=$(($BUS_finish_n + 1))
   elif echo $line | grep "$BUS_ar_fin" >/dev/null; then
   	BUS_ar_fin_n=$(($BUS_ar_fin_n + 1))
   elif echo $line | grep "$BUS_lv_fin" >/dev/null; then
   	BUS_lv_fin_n=$(($BUS_lv_fin_n + 1))
   elif echo $line | grep "$BUS_ar_z" >/dev/null; then
   	BUS_ar_z_n=$(($BUS_ar_z_n + 1))
   elif echo $line | grep "$BUS_lv_z" >/dev/null; then
   	BUS_lv_z_n=$(($BUS_lv_z_n + 1))
  else
  	echo "Line format error:" $line
  fi
done

if [ X$L_start_n = "X0" ]; then 
	echo "WARNING: no L started"
fi
if [ X$L_z_n = "X0" ]; then 
	echo "WARNING: no L arrived to idZ"
fi
if [ X$L_boarding_n = "X0" ]; then 
	echo "WARNING: no L borading"
fi
if [ X$L_ski_n = "X0" ]; then 
	echo "WARNING: no L going to ski"
fi
if [ ! X$L_start_n = X$L_z_n ]; then
	echo "ERROR: L started != L arrived to idZ"
fi
if [ ! X$L_start_n = X$L_boarding_n ]; then
	echo "ERROR: L started != L boarding"
fi
if [ ! X$L_start_n = X$L_ski_n ]; then
	echo "ERROR: L started != L going to ski"
fi
if [ ! X$BUS_start_n = "X1" ]; then 
	echo "ERROR: number of BUS stared different from 1"
fi
if [ ! X$BUS_finish_n = "X1" ]; then 
	echo "ERROR: number of BUS finish different from 1"
fi
if [ X$BUS_ar_fin_n = "X0" ]; then 
	echo "WARNING: no BUS arrived to final"
fi
if [ ! X$BUS_ar_fin_n = X$BUS_lv_fin_n ]; then
	echo "ERROR: BUS arrived to final != BUS leaving final"
fi
if [ X$BUS_ar_z_n = "X0" ]; then 
	echo "WARNING: no BUS arrived to idZ"
fi
if [ ! X$BUS_ar_z_n = X$BUS_lv_z_n ]; then
	echo "ERROR: BUS arrived to idZ != BUS leaving idZ"
fi



