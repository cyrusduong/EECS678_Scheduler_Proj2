/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"


/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements.
*/
typedef struct _job_t
{
  int jobNumber;
  int arrivalTime;
  int runTime;
  int remainingTime;
  int priority;
  int initTime;
  int lastTime;
} job_t;

/**
 * This defines cores
 */
typedef struct core {
  int nCores;
  job_t** jobs;
} core_t;

scheme_t curScheme;
priqueue_t* mQueue;
core_t cpu;
int currentTime = 0;
double totalWaitTime = 0.0;
int numWaiting = 0;
double totalRespTime = 0.0;
int numResponse = 0;
double totalTurnAroundTime = 0.0;
int numTurnAround = 0;

int compare(const void* jobA, const void* jobB) {
  int arrivalTimeA = ((job_t*)jobA)->arrivalTime;
  int arrivalTimeB = ((job_t*)jobB)->arrivalTime;
  int runningTimeA = ((job_t*)jobA)->runTime;
  int runningTimeB = ((job_t*)jobB)->runTime;
  int remainingTimeA = ((job_t*)jobA)->remainingTime;
  int remainingTimeB = ((job_t*)jobB)->remainingTime;
  int priorityA = ((job_t*)jobA)->priority;
  int priorityB = ((job_t*)jobB)->priority;

  int arrivalTimeDiff = arrivalTimeA - arrivalTimeB;
  int runTimeDiff = runningTimeA - runningTimeB;
  int remainingTimeDiff = remainingTimeA - remainingTimeB;
  int priorityDiff = priorityA - priorityB;

  switch (curScheme) {
    case FCFS: {
      return arrivalTimeDiff;
      break;
    }
    case SJF: {
      if (runTimeDiff == 0) {
        return arrivalTimeDiff;
      } else {
        return runTimeDiff;
      }
      break;
    }
    case PSJF: {
      if (remainingTimeDiff == 0) {
        return arrivalTimeDiff;
      } else {
        return remainingTimeDiff;
      }
      break;
    }
    case PRI: {
      if (priorityDiff == 0) {
        return arrivalTimeDiff;
      } else {
        return priorityDiff;
      }
      break;
    }
    case PPRI: {
      if (priorityDiff == 0) {
        return arrivalTimeDiff;
      } else {
        return priorityDiff;
      }
      break;
    }
    case RR: {
      return 0;
      break;
    }
    default: {
      break;
    }
  }
  return 0;
}

/**
 * CPU emulation functions
 */

/**
 * Initalizes the CPU emulator
 * @param cpu   the pointer to the current cpu object
 * @param cores number of cores the cpu should emulate
 */
void cpuInit(core_t* cpu, int cores) {
  cpu->nCores = cores;
  cpu->jobs = (job_t**)malloc(sizeof(job_t)*cores);

  for (int i = 0; i < cores; i++) {
    cpu->jobs[i] = NULL;
  }
}

/**
 * Updates time of all jobs in cpu
 * @param time the current time being updated to
 */
void cpuUpdateTime(int time) {
  job_t* job = NULL;
  currentTime = time;

  for (int i = 0; i < cpu.nCores; i++) {
    job = cpu.jobs[i];
    if (job != NULL) {
      if (job->initTime == -1 && job->lastTime != time) {
        job->initTime = job->lastTime;

        totalRespTime += job->initTime - job->arrivalTime;
        numResponse++;
      }

      job->remainingTime = job->remainingTime - (time - job->lastTime);
      job->lastTime = time;
    }
  }
}

/**
 * Returns the lowest number cpu core
 * Returns -1 if all cores are busy
 */
int cpuCoresAvailable() {
  for (int i = 0; i < cpu.nCores; i++) {
    if (cpu.jobs[i] == NULL) {
      return i;
    }
  }
  return -1;
}

/**
 * Attempts to assign a job to a cpu core
 * @param index  the cpu core we are assigning the job to
 * @param job    the job object a cpu core is being handed
 * @return       return job pointer that was added
 */
job_t* cpuCoreAssignJob(int index, job_t* job) {
  if (cpu.jobs[index] != NULL) {
    exit(1); //This shouldn't happen but it is saying the CPU is busy/is used. Made a check before this.
  }

  cpu.jobs[index] = job;
  job->lastTime = currentTime;
  return job;
}

job_t* cpuCoreRemoveJob(int core_id, int job_number) {
  //Not in range of the number of cores we have
  if (core_id < 0 || core_id >= cpu.nCores ) {
    exit(1);
  }

  //Removing a job a cpu that doesn't have it currently
  if (cpu.jobs[core_id]->jobNumber != job_number) {
    exit(1);
  }
  //----- The above are to catch program

  job_t* job = cpu.jobs[core_id];
  job->lastTime = -1;
  cpu.jobs[core_id] = NULL;
  priqueue_offer(mQueue, job);
  return job;
}


int cpuCorePreempt(job_t* job) {
  //This shouldn't happen, because there should have been a spot to queue the job up on CPU
  if (cpuCoresAvailable() != -1) {
    exit(1);
  }

  //Searches for a valid CPU to preempt, if none found return -1;
  int prevCompare = 0;
  int currCompare = 0;
  int cpuIndex = -1;
  for (int i = 0; i < cpu.nCores; i++) {
    prevCompare = compare(job, cpu.jobs[i]);
    if (prevCompare < currCompare) {
      currCompare = prevCompare;
      cpuIndex = i;
    } else if (prevCompare == currCompare) {
      if (cpuIndex != -1) {
        if (cpu.jobs[cpuIndex]->arrivalTime < cpu.jobs[i]->arrivalTime) {
          cpuIndex = i;
        }
      }
    }
  }

  //Found a valid CPU to preempt, swap jobs on that core.
  if (cpuIndex >= 0) {
    cpuCoreRemoveJob(cpuIndex, cpu.jobs[cpuIndex]->jobNumber);
    cpuCoreAssignJob(cpuIndex, job);
  }

  return cpuIndex;
}

/**
  Initalizes the scheduler.

  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/
void scheduler_start_up(int cores, scheme_t scheme)
{
  curScheme = scheme;

  mQueue = (priqueue_t*)malloc(sizeof(priqueue_t));
  priqueue_init(mQueue, compare);
  //Emulate initalization of CPU.
  cpuInit(&cpu, cores);
  cpuUpdateTime(0);
}


/**
  Called when a new job arrives.

  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
  Assumptions:
    - You may assume that every job wil have a unique arrival time.

  @param job_number a globally unique identification number of the job arriving.
  @param time the current time of the simulator.
  @param running_time the total number of time units this job will run before it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made.

 */
int scheduler_new_job(int job_number, int time, int running_time, int priority)
{
  cpuUpdateTime(time);
  job_t* job = (job_t*)malloc(sizeof(job_t));
  job->jobNumber = job_number;
  job->arrivalTime = time;
  job->runTime = running_time;
  job->remainingTime = running_time;
  job->priority = priority;
  job->initTime = -1;
  job->lastTime = -1;

  int workingCore = cpuCoresAvailable(); //Returns the lowest available core
  printf("workingCore=%d\n", workingCore);
  if (cpu.jobs[0] != NULL) {
    printf("cpu.jobs[0]=%d\n", cpu.jobs[0]->jobNumber);
  }

  if (workingCore != -1) {
    cpuCoreAssignJob(workingCore, job);
    return workingCore;
  } else if (curScheme == PSJF || curScheme == PPRI) {
    workingCore = cpuCorePreempt(job); //Foreces core to stop to look at current job if applicable
    printf("workingCore=%d\n", workingCore);
    if (workingCore == -1) { //If all current jobs on cpu have higher 'priority' at the moment
      priqueue_offer(mQueue, job);
    }
    return workingCore;
  } else {
    priqueue_offer(mQueue, job);
    printf("returning -1");
    return -1;
  }
}


/**
  Called when a job has completed execution.

  The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.

  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
 */
int scheduler_job_finished(int core_id, int job_number, int time)
{
  cpuUpdateTime(time);
  job_t* job = cpuCoreRemoveJob(core_id, job_number);
  priqueue_remove(mQueue, job);

  totalWaitTime += (currentTime - job->arrivalTime - job->runTime);
  numWaiting++;

  totalTurnAroundTime += (currentTime - job->arrivalTime);
  numTurnAround++;

  free(job);

  job = priqueue_poll(mQueue);
  if (job) {
    cpuCoreAssignJob(core_id, job);
    return job->jobNumber;
  }

	return -1;
}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.

  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time)
{
  cpuUpdateTime(time);
  cpuCoreRemoveJob(core_id, cpu.jobs[core_id]->jobNumber);

  job_t* job = priqueue_poll(mQueue);
  if (job) {
    cpuCoreAssignJob(core_id, job);
    return job->jobNumber;
  }
	return -1;
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
  if (numWaiting != 0) {
    return (double)totalWaitTime/numWaiting;
  }
	return 0.0;
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
  if (numTurnAround != 0) {
    return (double)totalTurnAroundTime/numTurnAround;
  }
	return 0.0;
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
  if (numResponse != 0) {
    return (double)totalRespTime/numResponse;
  }
	return 0.0;
}


/**
  Free any memory associated with your scheduler.

  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
  void* job = NULL;
  do {
    job = priqueue_poll(mQueue);
    free((job_t*) job);
  } while(job != NULL);
  free(cpu.jobs);
  priqueue_destroy(mQueue);
  free(mQueue);
}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled. Furthermore, we have also listed the current state of the job (either running on a given core or idle). For example, if we have a non-preemptive algorithm and job(id=4) has began running, job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:

    2(-1) 4(0) 1(-1)

  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.
 */
void scheduler_show_queue()
{
  //Woof
}
