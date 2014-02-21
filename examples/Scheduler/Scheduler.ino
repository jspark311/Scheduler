/*
File:   Scheduler.ino
Author: J. Ian Lindsay 
Date:   2013.06.28

This is an example sketch that will demonstrate basic usage of the Scheduler library.
There are numerous ways that the package can be used. This is only a very basic example.


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
*/

#include "Arduino.h"

#include <avr/io.h>
#include <avr/interrupt.h>


#ifndef SCHEDULER_H
#include <Scheduler/Scheduler.h>
#endif

// Pin definitions...
#define LED_PIN 13
#define PIEZO_PIN 14
#define ANALOG_PIN 15


// Global vars...
// Some periodic timer. Doesn't matter what. This is for the Teensy3...
#if defined(__MK20DX128__) || defined(__MK20DX256__)
#include "IntervalTimer.h"
IntervalTimer timer0;
#else
//...Or for anything else...
#include <TimerOne.h>
#endif


Scheduler scheduler;     // The actual scheduler object.

// These variables catch PIDs for us...
int led_flash_pid;
int profiler_dump_pid;
int analog_read_pid;

int analog_data;

/**************************************************************************
* Callbcak functions                                                      *
**************************************************************************/
/**
* Writes the scheduler's profiler data to the serial port.
*/
void printProfilingData() {
	char *temp_str = scheduler.dumpProfilingData();
	Serial.print(temp_str);
	free(temp_str);			// If you use a dump function, be sure to free() the variable you use...
}


/**
* Maybe we have an IR remote, or a piezo buzzer on this pin...
*/
void software_pwm() {
	digitalWrite(PIEZO_PIN, !(digitalRead(PIEZO_PIN) == HIGH));
}


/**
* This function will insert a new schedule entry that will cause a tone on PIEZO_PIN.
*/
void beep_via_software_pwm() {
	scheduler.createSchedule(1, 400, false, software_pwm);  // Output a short-duration 500Hz tone at PIEZO_PIN.
}


/**
* This function will read an analog pin and write the result to the serial port.
*/
void analog_read_fxn() {
	analog_data = analogRead(ANALOG_PIN);
	Serial.println(String(analog_data, 16));
}


/**
* Toggle the state of a given pin...
*/
void led_schedule_service() {
	digitalWrite(LED_PIN, !(digitalRead(LED_PIN) == HIGH));
}


/**
* Services the scheduler...
* Called once every millisecond for fine-grained events.
* Do no IO in this ISR. Takes too long.
*/
void timerCallbackScheduler() {
  // Calling this function will push the Scheduler forward by one tick.
  scheduler.advanceScheduler();
}


/**************************************************************************
* Init                                                                    *
**************************************************************************/
void setup() {                
  Serial.begin(9600);

  scheduler.createSchedule(250, 8, true, led_schedule_service);                        // Flash the LED four times at 2Hz. Auto-clears.
  analog_read_pid   = scheduler.createSchedule(1500, -1, false, analog_read_fxn);      // Read analog data every 1.5 seconds.
  profiler_dump_pid = scheduler.createSchedule(10000, -1, false, printProfilingData);  // Print profiling data once every 10 seconds.

  scheduler.createSchedule(800, 3, true, beep_via_software_pwm);   // Initiate another schedule. Beep three times and auto-clear.
  
  scheduler.beginProfiling(analog_read_pid);		// Profile the analog read function.
  scheduler.beginProfiling(profiler_dump_pid);		// Profile the function that dumps profiler data.
  
  // Write all the scheduler data to the serial port before we start running.
  char *temp_str = scheduler.dumpScheduleData();

  Serial.print(temp_str);
  free(temp_str);		// If you use a dump function, be sure to free() the variable you use...
  
  // Setup the periodic interrupt to call timerCallbackScheduler() once every millisecond.
  // This does not *need* to be done from a periodic interrupt, but it helps to keep accurate time.
  // Profiler data uses micros(), so it will be accurate regardless of the method chosen to advance
  //  the scheduler.
#if defined(__MK20DX128__) || defined(__MK20DX256__)
  timer0.begin(timerCallbackScheduler, 1000);     // <--- Teensy3
#else
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerCallbackScheduler);   // <--- Arduino
#endif

  sei();  // Let the madness begin.
}


/**************************************************************************
* Main Loop                                                               *
**************************************************************************/

void loop() {
  // Calling this function will result in the execution of all pending schedules.
  scheduler.serviceScheduledEvents();
}
