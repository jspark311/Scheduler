<b>Scheduler<br />
=========</b><br />
Author: J. Ian Lindsay<br />
Date:   2013.07.10<br />
<br />
This class is meant to be a real-time task scheduler for small microcontrollers. It<br />
should be driven by a periodic interrupt of some sort, but it may also be effectively<br />
used with a reliable polling scheme (at the possible cost of timing accuracy).<br />
<br />
A simple profiler is included which will allow the user of this class to determine<br />
run-times and possibly even adjust task duty cycles accordingly.<br />
<br />
This library was written for the Teensy 3.0, but it ought to work on other arduino-esque<br />
platforms as well. I tried to be general enough to allow for use on other boards such as<br />
the Maple, Ardunino, and similar. YMMV<br />
<br />
The scheduler is based on a linked-list of ScheduleItems (see .h file). The order of the<br />
nodes in the list is of minimal importance. Nodes are automatically free()'d as schedules<br />
expire, with the oldest schedules gravitating toward the head of the list. An integer PID<br />
is assigned to each new schedule added to the library. This PID is the handle by which the<br />
schedule is later manipulated, so the responsibility of keeping track of those PIDs rests<br />
with the user's program.<br />
<br />
Profiling is not enabled by default. If you don't intend on using the profiling feature of<br />
the library, you may excise all the related code from the class to save space.<br />
<br />
The output functions of the class depend on sprintf, which increases the binary's size by<br />
roughtly 2k. Again, if space is a serious constraint, you will not break anything by removing<br />
these functions.<br />
<br />
Actual overhead of running the scheduler is very small. That overhead grows logrithmicly with<br />
the number of schedules being serviced. Each defined schedule takes about 30 bytes of RAM. If<br />
profiling is enabled, that number rises to 44 bytes.<br />
<br />
<br />
<b>Basic Usage<br />
===========</b><br />
<br />
#include <Scheduler/Scheduler.h><br />
Scheduler scheduler;	// Does not need to be declared volatile.<br />
<br />
<br />
// This is the callback function that services the schedule. For now, it must take no<br />
// arguments and have a void return type.<br />
void analog_read_fxn(void); <br />
<br />
<br />
// Now we need to add a schedule...<br />
uint32_t a = scheduler.createSchedule(1500, -1, false, analog_read_fxn);<br />
<br />
// If we want to profile this schedule...<br />
scheduler.beginProfiling(a);<br />
<br />
<br />
// Then in a periodic interrupt...<br />
scheduler.advanceScheduler();<br />
<br />
// Finally, in your main loop...<br />
scheduler.serviceScheduledEvents();<br />
<br />
<br />
I am using this library to schedule I/O-intensive tasks that can't be called from an ISR<br />
directly. Calling dumpAllScheduleData() in my program gives this output...<br />
<pre>[PID, ENABLED, TTF, PERIOD, RECURS, PENDING, PROFILED]
[YES, 1, 1796, 5000, -1, NO, YES]
[YES, 2, 47, 50, -1, NO, YES]
[YES, 3, 338, 500, -1, NO, YES]
[YES, 4, 2793, 13000, -1, NO, YES]
[YES, 5, 46, 50, -1, NO, YES]
[YES, 6, 46, 71, -1, NO, YES]
[NO, 7, 333, 333, 0, NO, YES]
[YES, 8, 6793, 10000, -1, NO, YES]</pre>
<br />
I call my ISR once per millisecond, and so the TTF and PERIOD values are represented in milliseconds.<br />
PID 7 is presently stopped. It is treated as a "one-shot" routine to blink an LED so many times. It can<br />
be fired with a command similar to....<br />
   enableSchedule(led_schedule_pid);<br />
...where led_schedule_pid is the 32-bit unsigned int returned by the call to createSchedule(). In this case,<br />
that 32-bit unsigned int is equal to 0x00000007.<br />
<br />
And calling dumpProfilingData() gives...<br />
<pre>[PID, PROFILING, EXECUTED, LAST, BEST, WORST]
[1, YES, 27, 177, 176, 187]
[2, YES, 2745, 269, 261, 279]
[3, YES, 279, 62, 62, 65]
[4, YES, 10, 10573, 10572, 10575]
[5, YES, 2745, 638, 3, 656]
[6, YES, 1944, 72, 71, 77]
[7, YES, 9, 1, 1, 2]
[8, YES, 13, 2306, 1679, 2306]</pre>
<br />
These measurements are independent of the timing base of the program, and are all in microseconds.<br />
From this I can tell...<br />
* PID 4 is the most costly operation, but is very consistant.<br />
* PID 5 has the highest varience in runtime.<br />
* PID 8's last run was its worst.<br />
<br />
PID 8 is the process that dumps the profiling data to the serial port once every 10 seconds.<br />
<br />
<br />
<br />
<br />
<br />
<b>License<br />
=======</b><br />
Copyright (C) 2013 J. Ian Lindsay<br />
All rights reserved.<br />
<br />
This library is free software; you can redistribute it and/or<br />
modify it under the terms of the GNU Lesser General Public<br />
License as published by the Free Software Foundation; either<br />
version 2.1 of the License, or (at your option) any later version.<br />
<br />
This library is distributed in the hope that it will be useful,<br />
but WITHOUT ANY WARRANTY; without even the implied warranty of<br />
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU<br />
Lesser General Public License for more details.<br />
<br />
You should have received a copy of the GNU Lesser General Public<br />
License along with this library; if not, write to the Free Software<br />
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
