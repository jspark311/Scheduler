Scheduler
=========
Author: J. Ian Lindsay
Date:   2013.07.10

This class is meant to be a real-time task scheduler for small microcontrollers. It
should be driven by a periodic interrupt of some sort, but it may also be effectively
used with a reliable polling scheme (at the possible cost of timing accuracy).

A simple profiler is included which will allow the user of this class to determine
run-times and possibly even adjust task duty cycles accordingly.

This library was written for the Teensy 3.0, but it ought to work on other arduino-esque
platforms as well. I tried to be general enough to allow for use on other boards such as
the Maple, Ardunino, and similar. YMMV

The scheduler is based on a linked-list of ScheduleItems (see .h file). The order of the
nodes in the list is of minimal importance. Nodes are automatically free()'d as schedules
expire, with the oldest schedules gravitating toward the head of the list. An integer PID
is assigned to each new schedule added to the library. This PID is the handle by which the
schedule is later manipulated, so the responsibility of keeping track of those PIDs rests
with the user's program.

Profiling is not enabled by default. If you don't intend on using the profiling feature of
the library, you may excise all the related code from the class to save space.

The output functions of the class depend on sprintf, which increases the binary's size by
roughtly 2k. Again, if space is a serious constraint, you will not break anything by removing
these functions.

Actual overhead of running the scheduler is very small. That overhead grows logrithmicly with
the number of schedules being serviced. Each defined schedule takes about 30 bytes of RAM. If
profiling is enabled, that number rises to 44 bytes.


Basic Usage
===========

#include <Scheduler/Scheduler.h>
Scheduler scheduler;	// Does not need to be declared volatile.


// This is the callback function that services the schedule. For now, it must take no
// arguments and have a void return type.
void analog_read_fxn(void); 


// Now we need to add a schedule...
uint32_t a = scheduler.createSchedule(1500, -1, false, analog_read_fxn);

// If we want to profile this schedule...
scheduler.beginProfiling(a);


// Then in a periodic interrupt...
scheduler.advanceScheduler();

// Finally, in your main loop...
scheduler.serviceScheduledEvents();


I am using this library to schedule I/O-intensive tasks that can't be called from an ISR
directly. Calling dumpAllScheduleData() in my program gives this output...
[PID, ENABLED, TTF, PERIOD, RECURS, PENDING, PROFILED]
[YES, 1, 1796, 5000, -1, NO, YES]
[YES, 2, 47, 50, -1, NO, YES]
[YES, 3, 338, 500, -1, NO, YES]
[YES, 4, 2793, 13000, -1, NO, YES]
[YES, 5, 46, 50, -1, NO, YES]
[YES, 6, 46, 71, -1, NO, YES]
[NO, 7, 333, 333, 0, NO, YES]
[YES, 8, 6793, 10000, -1, NO, YES]

I call my ISR once per millisecond, and so the TTF and PERIOD values are represented in milliseconds.
PID 7 is presently stopped. It is treated as a "one-shot" routine to blink an LED so many times. It can
be fired with a command similar to....
   enableSchedule(led_schedule_pid);
...where led_schedule_pid is the 32-bit unsigned int returned by the call to createSchedule(). In this case,
that 32-bit unsigned int is equal to 0x00000007.

And calling dumpProfilingData() gives...
[PID, PROFILING, EXECUTED, LAST, BEST, WORST]
[1, YES, 27, 177, 176, 187]
[2, YES, 2745, 269, 261, 279]
[3, YES, 279, 62, 62, 65]
[4, YES, 10, 10573, 10572, 10575]
[5, YES, 2745, 638, 3, 656]
[6, YES, 1944, 72, 71, 77]
[7, YES, 9, 1, 1, 2]
[8, YES, 13, 2306, 1679, 2306]

These measurements are independent of the timing base of the program, and are all in microseconds.
From this I can tell...
* PID 4 is the most costly operation, but is very consistant.
* PID 5 has the highest varience in runtime.
* PID 8's last run was its worst.

PID 8 is the process that dumps the profiling data to the serial port once every 10 seconds.





License
=======
Copyright (C) 2013 J. Ian Lindsay
All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
