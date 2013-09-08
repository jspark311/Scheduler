/*
File:   Scheduler.cpp
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
#include <Scheduler/Scheduler.h>
#endif


/****************************************************************************************************
* Class-management functions...                                                                     *
****************************************************************************************************/

/**
*  Called once on setup to bring the timer system into a known state.
*/
Scheduler::Scheduler() {
}


/**
* Destructor.
*/
Scheduler::~Scheduler() {
  this->destroyAllScheduleItems();
}



/****************************************************************************************************
* Functions dealing with profiling data.                                                            *
****************************************************************************************************/

/**
* Any schedule that has a ScheduleProfile object in the appropriate slot will be profiled.
*  So to begin profiling a schedule, simply malloc() the appropriate struct into place and initialize it.
*/
void Scheduler::beginProfiling(ScheduleItem *target) {
  if (target != NULL) {
    if (target->prof_data == NULL) {
      ScheduleProfile *p_data  = (ScheduleProfile *) malloc(sizeof(ScheduleProfile));
      target->prof_data = p_data;
      p_data->profiling_active  = true;
      p_data->last_time_micros  = 0x00000000;
      p_data->execution_count   = 0x00000000;
      p_data->worst_time_micros = 0x00000000;
      p_data->best_time_micros  = 0xFFFFFFFF;
    }
  }
}


/**
*  Given the schedule PID, reset the profiling data.
*  Typically, we'd do this when the profiler is being turned on.
*/
void Scheduler::beginProfiling(uint32_t g_pid) {
  this->beginProfiling(findNodeByPID(g_pid));
}


/**
* Stops profiling without destroying the collected data.
* Note: If profiling is ever re-started on this schedule, the profiling data
*  that this function preserves will be wiped.
*/
void Scheduler::stopProfiling(ScheduleItem *obj) {
  if (obj != NULL) {
    ScheduleProfile *p_data  = obj->prof_data;
    if (p_data != NULL) {
      p_data->profiling_active  = false;
    }
  }
}


/**
*  Given the schedule PID, reset the profiling data.
*  Typically, we'd do this when the profiler is being turned on.
*/
void Scheduler::stopProfiling(uint32_t g_pid) {
  this->stopProfiling(findNodeByPID(g_pid));
}


/**
* Destroys whatever profiling data might be stored in the given schedule.
*/
void Scheduler::clearProfilingData(ScheduleItem *obj) {
  if (obj != NULL) {
    ScheduleProfile *p_data  = obj->prof_data;
    if (p_data != NULL) {
      free(p_data);
      p_data = NULL;
    }
  }
}


/**
*  Given the schedule PID, reset the profiling data.
*  Typically, we'd do this when the profiler is being turned on.
*/
void Scheduler::clearProfilingData(uint32_t g_pid) {
  this->clearProfilingData(findNodeByPID(g_pid));
}


/**
* Asks if this schedule is being profiled...
*  Returns true if so, and false if not.
*  Also returns false in the event that the ScheduleItem is NULL.
*/
boolean Scheduler::scheduleBeingProfiled(ScheduleItem *obj) {
  if (obj != NULL) {
    if (obj->prof_data != NULL) {
      return obj->prof_data->profiling_active;
    }
  }
  return false;
}


/**
* Asks if this schedule is being profiled...
*  Returns true if so, and false if not.
*  Also returns false in the event that the ScheduleItem is NULL.
*/
boolean Scheduler::scheduleBeingProfiled(uint32_t g_pid) {
  return this->scheduleBeingProfiled(findNodeByPID(g_pid));
}



/****************************************************************************************************
* Linked-list helper functions...                                                                   *
****************************************************************************************************/

/**
* Returns the number of schedules presently defined.
*/
uint16_t Scheduler::getTotalSchedules() {
  uint16_t return_value = 0;
  ScheduleItem *current  = this->schedule_root_node;
  while (current != NULL) {
    return_value++;
    current = current->next;
  }
  return return_value;
}


/**
* Returns the number of schedules presently active.
*/
uint16_t Scheduler::getActiveSchedules() {
  uint16_t return_value = 0;
  ScheduleItem *current  = this->schedule_root_node;
  while (current != NULL) {
    if (current->thread_enabled) return_value++;
    current = current->next;
  }
  return return_value;
}



/**
* Destroy everything in the list. Should only be called by the destructor, but no harm
*  in calling it for other reasons. Will stop and wipe all schedules. 
*/
void Scheduler::destroyAllScheduleItems() {
  ScheduleItem *temp0  = this->schedule_root_node;
  ScheduleItem *temp1;
  while (temp0 != NULL) {
    temp1  = temp0->next;
    this->clearProfilingData(temp0);
    free(temp0);
    temp0 = temp1;
  }
}


/*
* Inserts nu after prev. Maintains link integrity.
*/
boolean Scheduler::insertScheduleItemAfterNode(ScheduleItem *nu, ScheduleItem *prev) {
  if (prev != NULL) {
    nu->next    = prev->next;
    prev->next  = nu;
    return true;
  }
  return false;
}


/**
* Inserts nu at the end of the linked list.
*  Returns true on success, and false on failure. Should never return false if good parameters are given.
*/
boolean Scheduler::insertScheduleItemAtEnd(ScheduleItem *nu) {
  if (this->schedule_root_node != NULL) {
    ScheduleItem *temp  = this->schedule_root_node;
    while (temp->next != NULL) temp  = temp->next;
    temp->next  = nu;
    return true;
  }
  else {
    this->schedule_root_node = nu;
  }
  return false;
}


/**
* Traverses the linked list and returns a pointer to the node that has the given PID.
* Returns NULL if a node is not found that meets this criteria.
*/
ScheduleItem* Scheduler::findNodeByPID(uint32_t g_pid) {
  ScheduleItem *current  = this->schedule_root_node;
  while (current != NULL) {
    if (current->pid == g_pid) {
      return current;
    }
    current  = current->next;
  }
  return NULL;
}


/**
* Traverses the linked list and returns a pointer to the node that has target as its "->next" member.
* Returns NULL if a node is not found that meets this criteria.
* If target IS the schedule_root_node, returns NULL, because there is no node before the root node.
*/
ScheduleItem* Scheduler::findNodeBeforeThisOne(ScheduleItem *target) {
  ScheduleItem *current  = this->schedule_root_node;
  while (current != NULL) {
    if (current->next == target) {  // Not a mistake. Trying to compare pointer addresses.
      return current;
    }
    current  = current->next;
  }
  return NULL;
}


/** 
* Given a pointer to the node we wish destroyed, destroy it, and maintain link integrity.
* If the provided node is not part of the chain beginning at schedule_root_node, destroy it
* anyway to avoid a leak, and since that is why we were called.
* 
* This is a private function.
*/
void Scheduler::destroyScheduleItem(ScheduleItem *r_node) {
  if (r_node != NULL) {
    ScheduleItem *current  = this->findNodeBeforeThisOne(r_node);
    if (current != NULL) {          // Did we find a place to put our "->next" ref?
      current->next = r_node->next;
    }
    else if (r_node == this->schedule_root_node) {    // Special-case, the root node is being destroyed.
      this->schedule_root_node = r_node->next;
    }
    // We are now free to free()...
    this->clearProfilingData(r_node);
    free(r_node);
  }
}


/**
*  When we assign a new PID, call this function to get one. Since we don't want
*    to collide with one that already exists, or get the zero value. 
*/
uint32_t Scheduler::get_valid_new_pid() {
    uint32_t return_value = this->next_pid++;
    if (return_value == 0) {
        return_value = this->get_valid_new_pid();  // Recurse...
    }
    // Takes too long, but represents a potential bug.
    //else if (this->findNodeByPID(return_value) == NULL) {
    //	return_value = this->get_valid_new_pid();  // Recurse...
    //}

	return return_value;
}

uint32_t Scheduler::peekNextPID() {
  return this->next_pid;
}


/**
*  Call this function to create a new schedule with the given period, a given number of repititions, and with a given function call.
*
*  Will automatically set the schedule active, provided the input conditions are met.
*  Returns the newly-created PID on success, or 0 on failure.
*/
uint32_t Scheduler::createSchedule(uint32_t sch_period, short recurrence, boolean ac, FunctionPointer sch_callback) {
  uint32_t return_value  = 0;
  if (sch_period > 1) {
    if (sch_callback != NULL) {
      ScheduleItem *nu_sched = (ScheduleItem *) malloc(sizeof(ScheduleItem));
      if (nu_sched != NULL) {  // Did we actually malloc() successfully?
        bzero(nu_sched, sizeof(ScheduleItem));
        nu_sched->pid  = this->get_valid_new_pid();
        nu_sched->thread_enabled      = true;    // Note: Enables immediately.
        nu_sched->thread_fire         = false;
        nu_sched->thread_recurs       = recurrence;
        nu_sched->thread_period       = sch_period;
        nu_sched->next                = NULL;
        nu_sched->thread_time_to_wait = sch_period;
        nu_sched->autoclear           = ac;
        nu_sched->schedule_callback   = sch_callback;
        return_value  = nu_sched->pid;
        this->insertScheduleItemAtEnd(nu_sched);
      }
    }
  }
  return return_value;
}


/**
* Call this function to alter a given schedule. Set with the given period, a given number of times, with a given function call.
*  Returns true on success or false if the given PID is not found, or there is a problem with the parameters.
*
* Will not set the schedule active, but will clear any pending executions for this schedule, as well as reset the timer for it.
*/
boolean Scheduler::alterSchedule(ScheduleItem *obj, uint32_t sch_period, int16_t recurrence, boolean ac, FunctionPointer sch_callback) {
  boolean return_value  = false;
  if (sch_period > 1) {
    if (sch_callback != NULL) {
      if (obj != NULL) {
        obj->thread_fire         = false;
        obj->thread_recurs       = recurrence;
        obj->thread_period       = sch_period;
        obj->thread_time_to_wait = sch_period;
        obj->autoclear           = ac;
        obj->schedule_callback   = sch_callback;
        return_value  = true;
      }
    }
  }
  return return_value;
}

boolean Scheduler::alterSchedule(uint32_t g_pid, uint32_t sch_period, int16_t recurrence, boolean ac, FunctionPointer sch_callback) {
  return this->alterSchedule(findNodeByPID(g_pid), sch_period, recurrence, ac, sch_callback);
}

boolean Scheduler::alterSchedule(uint32_t schedule_index, boolean ac) {
  boolean return_value  = false;
  ScheduleItem *nu_sched  = findNodeByPID(schedule_index);
  if (nu_sched != NULL) {
    nu_sched->autoclear = ac;
    return_value  = true;
  }
  return return_value;
}

boolean Scheduler::alterSchedule(uint32_t schedule_index, FunctionPointer sch_callback) {
  boolean return_value  = false;
  if (sch_callback != NULL) {
    ScheduleItem *nu_sched  = findNodeByPID(schedule_index);
    if (nu_sched != NULL) {
      nu_sched->schedule_callback   = sch_callback;
      return_value  = true;
    }
  }
  return return_value;
}

boolean Scheduler::alterSchedulePeriod(uint32_t schedule_index, uint32_t sch_period) {
  boolean return_value  = false;
  if (sch_period > 1) {
    ScheduleItem *nu_sched  = findNodeByPID(schedule_index);
    if (nu_sched != NULL) {
      nu_sched->thread_fire         = false;
      nu_sched->thread_period       = sch_period;
      nu_sched->thread_time_to_wait = sch_period;
      return_value  = true;
    }
  }
  return return_value;
}

boolean Scheduler::alterScheduleRecurrence(uint32_t schedule_index, int16_t recurrence) {
  boolean return_value  = false;
  ScheduleItem *nu_sched  = findNodeByPID(schedule_index);
  if (nu_sched != NULL) {
    nu_sched->thread_fire         = false;
    nu_sched->thread_recurs       = recurrence;
    return_value  = true;
  }
  return return_value;
}


/**
* Returns true if...
* A) The schedule exists
*    AND
* B) The schedule is enabled, and has at least one more runtime before it *might* be auto-reaped.
*/
boolean Scheduler::willRunAgain(uint32_t g_pid) {
  ScheduleItem *nu_sched  = findNodeByPID(g_pid);
  if (nu_sched != NULL) {
    if (nu_sched->thread_enabled) {
      if ((nu_sched->thread_recurs == -1) || (nu_sched->thread_recurs > 0)) {
	return true;
      }
    }
  }
  return false;
}


boolean Scheduler::scheduleEnabled(uint32_t g_pid) {
  ScheduleItem *nu_sched  = findNodeByPID(g_pid);
  if (nu_sched != NULL) {
    return nu_sched->thread_enabled;
  }
  return false;
}


/**
* Enable a previously disabled schedule.
*  Returns true on success and false on failure.
*/
boolean Scheduler::enableSchedule(uint32_t g_pid) {
  ScheduleItem *nu_sched  = findNodeByPID(g_pid);
  if (nu_sched != NULL) {
    nu_sched->thread_enabled = true;
    return true;
  }
  return false;
}


boolean Scheduler::delaySchedule(ScheduleItem *obj, uint32_t by_ms) {
  if (obj != NULL) {
    obj->thread_time_to_wait = by_ms;
    obj->thread_enabled = true;
    return true;
  }
  return false;
}

/**
* Causes a given schedule's TTW (time-to-wait) to be set to the value we provide (this time only).
* If the schedule wasn't enabled before, it will be when we return.
*/
boolean Scheduler::delaySchedule(uint32_t g_pid, uint32_t by_ms) {
  ScheduleItem *nu_sched  = findNodeByPID(g_pid);
  if (nu_sched != NULL) {
    this->delaySchedule(nu_sched, by_ms);
    return true;
  }
  return false;
}

/**
* Causes a given schedule's TTW (time-to-wait) to be reset to its period.
* If the schedule wasn't enabled before, it will be when we return.
*/
boolean Scheduler::delaySchedule(uint32_t g_pid) {
  ScheduleItem *nu_sched  = findNodeByPID(g_pid);
  if (nu_sched != NULL) {
    this->delaySchedule(nu_sched, nu_sched->thread_period);
    return true;
  }
  return false;
}



/**
* Call this function to push the schedules forward.
*/
void Scheduler::advanceScheduler() {
  ScheduleItem *current  = this->schedule_root_node;
  while (current != NULL) {
    if (current->thread_enabled) {
      if (current->thread_time_to_wait > 0) current->thread_time_to_wait--;
      else {
        current->thread_fire = true;
        current->thread_time_to_wait = current->thread_period;
      }
    }
    current = current->next;
  }
}


/**
* Call to disable a given schedule.
*  Will reset the time_to_wait so that if the schedule is re-enabled, it doesn't fire sooner than expected.
*  Returns true on success and false on failure.
*/
boolean Scheduler::disableSchedule(uint32_t g_pid) {
  ScheduleItem *nu_sched  = findNodeByPID(g_pid);
  if (nu_sched != NULL) {
      nu_sched->thread_enabled = false;
      nu_sched->thread_fire    = false;
      nu_sched->thread_time_to_wait = nu_sched->thread_period;
      return true;
  }
  return false;
}


/**
* Will remove the indicated schedule and wipe its profiling data.
* In case this gets called from the schedule's service function (IE,
*   if the schedule tries to delete itself), let it expire this run
*   rather than ripping the rug out from under ourselves.
* Returns true on success and false on failure.
*/
boolean Scheduler::removeSchedule(uint32_t g_pid) {
  ScheduleItem *obj  = findNodeByPID(g_pid);
  if (obj != NULL) {
    if (obj->pid != this->currently_executing) {
      this->destroyScheduleItem(obj);
    }
    else { 
      obj->autoclear = true;
      obj->thread_recurs = 0;
    }
  }
  return true;
}


/**
* This is the function that is called from the main loop to offload big
*  tasks into idle CPU time. If many scheduled items have fired, function
*  will only execute the first one it finds.
*  Therefore: Lower-numbered schedules are de facto higher-priority.
*/
void Scheduler::serviceScheduledEvents() {
  uint32_t profile_start_time, profile_last_time;
  uint32_t origin_time = micros();
  ScheduleItem *current  = this->schedule_root_node;
  ScheduleItem *temp;
  while (current != NULL) {
    temp = NULL;
    if (current->thread_fire) {
      if (current->schedule_callback != NULL) {
        if (this->scheduleBeingProfiled(current)) profile_start_time = micros();
        
        this->currently_executing = current->pid;
        ((void (*)(void)) current->schedule_callback)();    // Call the schedule's service function.
        this->currently_executing = 0;

        if (this->scheduleBeingProfiled(current)) {
          profile_last_time     = micros();
          current->prof_data->last_time_micros   = max(profile_start_time, profile_last_time) - min(profile_start_time, profile_last_time);  // Rollover invarient.
          current->prof_data->worst_time_micros  = max(current->prof_data->worst_time_micros, current->prof_data->last_time_micros);
          current->prof_data->best_time_micros   = min(current->prof_data->best_time_micros, current->prof_data->last_time_micros);
          current->prof_data->execution_count++;
        }            
      }
      current->thread_fire = false;
         
      switch (current->thread_recurs) {
        case -1:           // Do nothing. Schedule runs indefinitely.
          break;
        case 0:            // Disable (and remove?) the schedule.
          if (current->autoclear) {
            temp = current->next;  // Careful.....
            this->destroyScheduleItem(current);
            current = temp;
          }
          else {
            current->thread_enabled = false;  // Disable the schedule...
            current->thread_fire    = false;  // ...mark it as serviced.
            current->thread_time_to_wait = current->thread_period;  // ...and reset the timer.
          }
          break;
        default:           // Decrement the run count.
          current->thread_recurs--;
          break;
      }
      this->productive_loops++;
      break;
    }
    current = (current == NULL) ? temp : current->next;
  }
  this->overhead = micros() - origin_time;
  this->total_loops++;
}



/****************************************************************************************************
* These functions deal with writing output for the user to read...                                  *
* Please note that all of these functions malloc space for their output. So be sure to free the     *
*  memory after you've finished writing the string to a serial port, or whatever you do with it.    *
****************************************************************************************************/

/**
* Dumps profiling data for the schedule with the given PID.
*/
char* Scheduler::dumpProfilingData(uint32_t g_pid) {
  const char* PROFILER_HEADER = "[PID, PROFILING, EXECUTED, LAST, BEST, WORST]\n";
  char* return_value  = NULL;
  const uint16_t EXPECTED_SIZE_OF_LINE = 140;
  uint16_t num_strs  = this->getTotalSchedules();
  if (num_strs > 0) {
    ScheduleItem *current  = this->schedule_root_node;
    char* temp_str_out  = (char*) alloca(EXPECTED_SIZE_OF_LINE * num_strs);  // Arbitrary. Slightly too big. Should not overflow.
    if (temp_str_out != NULL) {
      bzero(temp_str_out, EXPECTED_SIZE_OF_LINE * num_strs);
      char* temp_str  = (char*) alloca(EXPECTED_SIZE_OF_LINE);  // Arbitrary. Slightly too big. Should not overflow.
      bzero(temp_str, EXPECTED_SIZE_OF_LINE);
      strcat(temp_str_out, PROFILER_HEADER);    // Write the header.
  
      while (current != NULL) {
        if (current->prof_data != NULL) {
	  if ((g_pid == 0 | g_pid == current->pid) | (g_pid == -1)) {
            sprintf(temp_str, "[%d, %s, %d, %d, %d, %d]\n", current->pid, ((current->prof_data->profiling_active) ? "YES":"NO"), current->prof_data->execution_count, current->prof_data->last_time_micros, current->prof_data->best_time_micros, current->prof_data->worst_time_micros);
            strcat(temp_str_out, temp_str);
            bzero(temp_str, EXPECTED_SIZE_OF_LINE);
	  }
        }
        current = current->next;
      }
      return_value = strdup(temp_str_out);
    }
    else {
      sprintf(temp_str_out, "COULD NOT ALLOCATE %d BYTES ON STACK\n", (EXPECTED_SIZE_OF_LINE * num_strs));
      return_value = strdup(temp_str_out);
    }
  }
  else {
    return_value = strdup("NO SCHEDULES");
  }
  return return_value;
}

/**
* Dumps profiling data for all schedules where the data exists.
*/
char* Scheduler::dumpProfilingData() {
  return this->dumpProfilingData(-1);
}


/**
* Dumps schedule data. Pass 0 as the first parameter to get all processes.
*/
char* Scheduler::dumpScheduleData(uint32_t g_pid, boolean actives_only) {
  const char* SCHEDULE_HEADER = "[PID, ENABLED, TTF, PERIOD, RECURS, PENDING, AUTOCLEAR, PROFILED]\n";
  char* return_value  = NULL;
  const uint16_t EXPECTED_SIZE_OF_LINE = 146;
  uint16_t num_strs  = this->getTotalSchedules();
  if (num_strs > 0) {
    ScheduleItem *current  = this->schedule_root_node;
    char* temp_str_out  = (char*) alloca(EXPECTED_SIZE_OF_LINE * num_strs);  // Arbitrary. Slightly too big. Should not overflow.
    if (temp_str_out != NULL) {
      bzero(temp_str_out, EXPECTED_SIZE_OF_LINE * num_strs);
      char* temp_str  = (char*) alloca(EXPECTED_SIZE_OF_LINE);  // Arbitrary. Slightly too big. Should not overflow.
      bzero(temp_str, EXPECTED_SIZE_OF_LINE);
      strcat(temp_str_out, SCHEDULE_HEADER);    // Write the header.
  
      while (current != NULL) {
	if ((g_pid == 0 | g_pid == current->pid) | !actives_only){
          sprintf(temp_str, "[%d, %s, %d, %d, %d, %s, %s, %s]\n", current->pid, ((current->thread_enabled) ? "YES":"NO"), current->thread_time_to_wait, current->thread_period, current->thread_recurs, ((current->thread_fire) ? "YES":"NO"), ((current->autoclear) ? "YES":"NO"), ((current->prof_data != NULL && current->prof_data->profiling_active) ? "YES":"NO"));
          strcat(temp_str_out, temp_str);
          bzero(temp_str, EXPECTED_SIZE_OF_LINE);
	}
        current = current->next;
      }
      return_value = strdup(temp_str_out);
    }
    else {
      sprintf(temp_str_out, "COULD NOT ALLOCATE %d BYTES ON STACK\n", (EXPECTED_SIZE_OF_LINE * num_strs));
      return_value = strdup(temp_str_out);
    }
  }
  else {
    return_value = strdup("NO SCHEDULES");
  }
  return return_value;
}

/**
* Dumps schedule data for all defined schedules. Active or not.
*/
char* Scheduler::dumpScheduleData() {
  return this->dumpScheduleData(0, false);
}

/**
* Dumps schedule data for the schedule with the given PID.
*/
char* Scheduler::dumpScheduleData(uint32_t g_pid) {
  return this->dumpScheduleData(g_pid, false);
}

/**
* Dumps schedule data for all active schedules.
*/
char* Scheduler::dumpAllActiveScheduleData() {
  return this->dumpScheduleData(0, true);
}
