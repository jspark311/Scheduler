/*
File:   Scheduler.h
Author: J. Ian Lindsay
Date:   2013.07.10

This class is meant to be a real-time task scheduler for small microcontrollers. It
should be driven by a periodic interrupt of some sort, but it may also be effectively
used with a reliable polling scheme (at the possible cost of timing accuracy).

A simple profiler is included which will allow the user of this class to determine
run-times and possibly even adjust task duty cycles accordingly.

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

#ifndef SCHEDULER_H
#define SCHEDULER_H


#include <inttypes.h>
#include "Arduino.h"

// We need to def a few types... First, let's def a function pointer to avoid
// cluttering things up with unreadable casts...

typedef void (*FunctionPointer) ();

// Data associated with profiling schedules...
typedef struct sch_item_prof_t {
  uint32_t last_time_micros;   // Last execution time, in microseconds.
  uint32_t worst_time_micros;  // Worst execution time, in microseconds.
  uint32_t best_time_micros;   // Best execution time, in microseconds.
  uint32_t execution_count;    // Number of times this schedule has executed.
  boolean  profiling_active;   // Is this data being actively refreshed?
} ScheduleProfile;

// Type for schedule items...
typedef struct sch_item_t {
  struct sch_item_t* next;             // This will be a linked-list.
  struct sch_item_prof_t* prof_data;   // If this schedule is being profiled, the ref will be here.
  uint32_t pid;                        // The process ID of this item.
  uint32_t thread_time_to_wait;        // How much longer until the schedule fires?
  uint32_t thread_period;              // How often does this schedule execute?
  int16_t  thread_recurs;              // See Note 2.
  boolean  thread_enabled;             // Is the schedule running?
  boolean  thread_fire;                // Is the schedule to be executed?
  boolean  autoclear;                  // If true, this schedule will be removed after its last execution.
  FunctionPointer schedule_callback;   // Pointers to the schedule service function.
} ScheduleItem;



/**  Note 2:
* If this value is equal to -1, then the schedule recurs for as long as it remains enabled.
*  If the value is zero, the schedule is disabled upon execution.
*  If the value is anything else, the schedule remains enabled and this value is decremented.
*/


#ifndef __cplusplus
    // Despite being public members, these values should not be written from outside the class.
    uint32_t productive_loops = 0x00000000;  // Number of calls to serviceScheduledEvents() that actually called a schedule.
    uint32_t total_loops      = 0x00000000;  // Number of calls to serviceScheduledEvents().

    uint16_t getTotalSchedules(void);   // How many total schedules are present?
    uint16_t getActiveSchedules(void);  // How many active schedules are present?
    
    boolean scheduleBeingProfiled(uint32_t g_pid);
    void beginProfiling(uint32_t g_pid);
    void stopProfiling(uint32_t g_pid);
    void clearProfilingData(uint32_t g_pid);        // Clears profiling data associated with the given schedule.
    
    
    // Alters an existing schedule (if PID is found),
    boolean alterSchedule(uint32_t schedule_index, uint32_t sch_period, short recurrence, boolean auto_clear, FunctionPointer sch_callback);
    boolean alterSchedule(uint32_t schedule_index, uint32_t sch_period);
    boolean alterSchedule(uint32_t schedule_index, boolean auto_clear);
    boolean alterSchedule(uint32_t schedule_index, int16_t recurrence);
    boolean alterSchedule(uint32_t schedule_index, FunctionPointer sch_callback);

    uint32_t createSchedule(uint32_t sch_period, short recurrence, boolean auto_clear, FunctionPointer sch_callback);
    
    boolean enableSchedule(uint32_t g_pid);   // Re-enable a previously-disabled schedule.

    boolean disableSchedule(uint32_t g_pid);  // Turn a schedule off without removing it.
    boolean removeSchedule(uint32_t g_pid);   // Clears all data relating to the given schedule.
    boolean delaySchedule(uint32_t g_pid, uint32_t by_ms);  // Set the schedule's TTW to the given value this execution only.
    boolean delaySchedule(uint32_t g_pid);                  // Reset the given schedule to its period and enable it.

    void serviceScheduledEvents(void);        // Execute any schedules that have come due.
    void advanceScheduler(void);              // Push all enabled schedules forward by one tick.
    
    char* dumpAllActiveScheduleData(void);                       // Dumps schedule data for all active schedules.
    char* dumpScheduleData(void);                                // Dumps schedule data for all defined schedules. Active or not.
    char* dumpScheduleData(uint32_t g_pid);                      // Dumps schedule data for the schedule with the given PID.
    char* dumpProfilingData(void);                               // Dumps profiling data for all schedules where the data exists.
    char* dumpProfilingData(uint32_t g_pid);                     // Dumps profiling data for the schedule with the given PID.
    char* dumpScheduleData(uint32_t g_pid, boolean active_only); // Dumps schedule data for all defined schedules. Active or not.
#endif


#ifdef __cplusplus

// This is the only version I've tested...
class Scheduler {
  uint32_t next_pid = 0x00000001;      // Next PID to assign.
  ScheduleItem* schedule_root_node = NULL; // The root of the linked lists in this scheduler.

  public:
    Scheduler();   // Constructor
    ~Scheduler();  // Destructor
    
    // Despite being public members, these values should not be written from outside the class.
    uint32_t productive_loops = 0x00000000;  // Number of calls to serviceScheduledEvents() that actually called a schedule.
    uint32_t total_loops      = 0x00000000;  // Number of calls to serviceScheduledEvents().

    uint16_t getTotalSchedules(void);   // How many total schedules are present?
    uint16_t getActiveSchedules(void);  // How many active schedules are present?
    
    boolean scheduleBeingProfiled(uint32_t g_pid);
    void beginProfiling(uint32_t g_pid);
    void stopProfiling(uint32_t g_pid);
    void clearProfilingData(uint32_t g_pid);        // Clears profiling data associated with the given schedule.
    
    // Alters an existing schedule (if PID is found),
    boolean alterSchedule(uint32_t schedule_index, uint32_t sch_period, short recurrence, boolean auto_clear, FunctionPointer sch_callback);
    boolean alterSchedule(uint32_t schedule_index, uint32_t sch_period);
    boolean alterSchedule(uint32_t schedule_index, boolean auto_clear);
    boolean alterSchedule(uint32_t schedule_index, int16_t recurrence);
    boolean alterSchedule(uint32_t schedule_index, FunctionPointer sch_callback);
    
    /* Add a new schedule. Returns the PID. If zero is returned, function failed.
     * 
     * Parameters:
     * sch_period      The period of the schedule service routine.
     * recurrence      How many times should this schedule run?
     * auto_clear      When recurrence reaches 0, should the schedule be reaped?
     * sch_callback    The service function. Must be a pointer to a (void fxn(void)).
     */    
    uint32_t createSchedule(uint32_t sch_period, short recurrence, boolean auto_clear, FunctionPointer sch_callback);
    
    boolean enableSchedule(uint32_t g_pid);   // Re-enable a previously-disabled schedule.

    boolean disableSchedule(uint32_t g_pid);  // Turn a schedule off without removing it.
    boolean removeSchedule(uint32_t g_pid);   // Clears all data relating to the given schedule.
    boolean delaySchedule(uint32_t g_pid, uint32_t by_ms);  // Set the schedule's TTW to the given value this execution only.
    boolean delaySchedule(uint32_t g_pid);                  // Reset the given schedule to its period and enable it.

    void serviceScheduledEvents(void);        // Execute any schedules that have come due.
    void advanceScheduler(void);              // Push all enabled schedules forward by one tick.
    
    /* The functions below return a malloc'd string. So be careful to free() the result
     *   once you have finished with it. No functionality depends on these functions, and 
     *   they are a bit heavy (depend on sprintf). There is no harm in removing them from 
     *   the library if you are space-constrained. 
     */
    char* dumpAllActiveScheduleData(void);                       // Dumps schedule data for all active schedules.
    char* dumpScheduleData(void);                                // Dumps schedule data for all defined schedules. Active or not.
    char* dumpScheduleData(uint32_t g_pid);                      // Dumps schedule data for the schedule with the given PID.
    char* dumpProfilingData(void);                               // Dumps profiling data for all schedules where the data exists.
    char* dumpProfilingData(uint32_t g_pid);                     // Dumps profiling data for the schedule with the given PID.
    char* dumpScheduleData(uint32_t g_pid, boolean active_only); // Dumps schedule data for all defined schedules. Active or not.

  private:
    boolean scheduleBeingProfiled(ScheduleItem *obj);
    void beginProfiling(ScheduleItem *obj);
    void stopProfiling(ScheduleItem *obj);
    void clearProfilingData(ScheduleItem *obj);        // Clears profiling data associated with the given schedule.
    
    boolean alterSchedule(ScheduleItem *obj, uint32_t sch_period, short recurrence, boolean auto_clear, FunctionPointer sch_callback);

    boolean insertScheduleItemAfterNode(ScheduleItem *nu, ScheduleItem *prev);
    boolean insertScheduleItemAtEnd(ScheduleItem *obj);
    void destroyAllScheduleItems(void);
    
    ScheduleItem* findNodeByPID(uint32_t g_pid);
    ScheduleItem* findNodeBeforeThisOne(ScheduleItem *obj);
    void destroyScheduleItem(ScheduleItem *r_node);
    
    boolean delaySchedule(ScheduleItem *obj, uint32_t by_ms);
};

#endif
#endif