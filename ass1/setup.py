#!/usr/bin/env python
import os

with open('/etc/rc.local') as fin:
	with open('/etc/rc.local.tmp', 'w+') as fout:
		for line in fin:
			if line == 'exit 0\n':
				fout.write('# enable transparent hugepages\n')
				fout.write('echo never > /sys/kernel/mm/transparent_hugepage/enabled\n\n')
				fout.write('# overcommit always on:\n')
				fout.write('echo 1 > /proc/sys/vm/overcommit_memory\n\n')
				fout.write('# set cpu governor to performance so that speedstep is disabled:\n')
				fout.write('echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor\n')
				fout.write('echo performance > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor\n')
				fout.write('echo performance > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor\n')
				fout.write('echo performance > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor\n\n')
			fout.write(line)
		fout.close()
	fin.close()
				
os.rename('/etc/rc.local', '/etc/rc.local.bkp')
os.rename('/etc/rc.local.tmp', '/etc/rc.local')

