#!/bin/sh

cat /sys/kernel/mm/transparent_hugepage/enabled	# check transparent hugepage status
cat /proc/sys/vm/overcommit_memory	# check overcommit status
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor	# check cpu governor setting (core 1)
cat /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor	# check cpu governor setting (core 2)
cat /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor	# check cpu governor setting (core 3)
cat /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor	# check cpu governor setting (core 4)
dmesg | grep BIOS
make 
./prog
