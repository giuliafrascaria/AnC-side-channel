#!/bin/sh

# enable transparent hugepages
echo never > /sys/kernel/mm/transparent_hugepage/enabled

# overcommit always on:
echo 1 > /proc/sys/vm/overcommit_memory

# set cpu governor to performance so that speedstep is disabled:
echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo performance > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
echo performance > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
echo performance > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
