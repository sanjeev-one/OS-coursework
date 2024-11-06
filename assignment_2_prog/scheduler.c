/************************************************************************************************************************

    ** File Description **
    File: scheduler.c
    Purpose: Implements a multi-level feedback queue (MLFQ) scheduler for the COMP3520 Operating Systems course.
    Author: Sanjeev Chauhan
    Institution: University of Sydney
    Date: 19 October 2024
    Assignment: COMP3520 Assignment 2 - Multi Level Feedback Queue (MLFQ)
    
    This program simulates a multi-level feedback queue scheduler, reading processes from an input file and distributing them across different priority levels.
    Each process has a time quantum for each priority level, which diminishes as processes are demoted between queues. The scheduler handles starvation by promoting
    processes from lower-priority queues if they wait beyond a specified threshold. The program calculates and outputs average turnaround time, wait time, and response time for all processes.

    ** Revision history **
    
    Current version: 1.0
    Date: 19 October 2024

    Contributors:
    1. Sanjeev Chauhan, Student, University of Sydney, NSW 2006, Australia
   

 ************************************************************************************************************************/

/* Include files */
#include "scheduler.h"



// Main function: Entry point of the program
int main (int argc, char *argv[])
{
    /*** Main function variable declarations ***/
    // Declare pointers for process control blocks (PCBs)
    PcbPtr job_dispatch_queue = NULL;
    PcbPtr round_robin_queue = NULL;

    // Array to hold queues for different priority levels
    PcbPtr queues[3] = {NULL, NULL, NULL};
    #define level0_queue queues[0]
    #define level1_queue queues[1]
    #define level2_queue queues[2]

    // File stream for input list
    FILE * input_list_stream = NULL;
    
    // Current process and timer variables
    PcbPtr current_process = NULL; // Pointer to the current running process
    PcbPtr process = NULL; // Pointer to a temporary process
    int timer = 0; // Timer variable

    int turnaround_time, response_time; // Variables to hold turnaround and response times
    double av_turnaround_time = 0.0, av_wait_time = 0.0, av_response_time = 0.0; // Variables to hold average turnaround and wait times
    int n = 0; // Variable to hold number of processes

    //ask for time_quantum 
    int t0, t1, t2, W; // Variables to hold time quantum values and starvation time
    int time_quantum[3];

    // Get time quantum values from user
    printf("Enter an integer value for t0: ");
    if (scanf("%d", &t0) != 1) {
        fprintf(stderr, "Error: Invalid input for t0\n");
        exit(EXIT_FAILURE);
    }
    printf("Enter an integer value for t1: ");
    if (scanf("%d", &t1) != 1) {
        fprintf(stderr, "Error: Invalid input for t1\n");
        exit(EXIT_FAILURE);
    }
    printf("Enter an integer value for t2: ");
    if (scanf("%d", &t2) != 1) {
        fprintf(stderr, "Error: Invalid input for t2\n");
        exit(EXIT_FAILURE);
    }

    printf("Enter an integer value for W: ");
    if (scanf("%d", &W) != 1) {
        fprintf(stderr, "Error: Invalid input for W\n");
        exit(EXIT_FAILURE);
    }

    time_quantum[0] = t0;
    time_quantum[1] = t1;
    time_quantum[2] = t2;

//  1. Populate the job dispatch queue from the input file
    if (argc <= 0)
    {
        fprintf(stderr, "FATAL: Bad arguments array\n");
        exit(EXIT_FAILURE);
    }
    else if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <TESTFILE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!(input_list_stream = fopen(argv[1], "r")))
    {
        fprintf(stderr, "ERROR: Could not open \"%s\"\n", argv[1]);
        exit(EXIT_FAILURE);
    }
//  Read in the job dispatch list from the input file
    while (!feof(input_list_stream)) {  // put processes into job_dispatch queue
        process = createnullPcb();
        if (fscanf(input_list_stream,"%d, %d, %d",
             &(process->arrival_time), 
             &(process->service_time),
             &(process->priority)) != 3) {
            free(process);
            continue;
        }
        process->remaining_cpu_time = process->service_time;
        process->status = PCB_INITIALIZED;
        job_dispatch_queue = enqPcb(job_dispatch_queue, process);
        n++;
    }

    fclose(input_list_stream);

    
    //printf(stderr, "INFO: Read %d processes from \"%s\"\n", n, argv[1]);


//  2. Whenever there is a running process or the job_dispatch queue or round robin queue is not empty: 

    while (current_process || job_dispatch_queue || level0_queue || level1_queue || level2_queue)
    {

        //Unload any arrived pending processes from the Job Dispatch queue  dequeue  process  from  Job  Dispatch  queue  and  enqueue  on  Round  Robin queue;
        while (job_dispatch_queue && job_dispatch_queue->arrival_time <= timer)
        {
            process = deqPcb(&job_dispatch_queue);
            //printf("Process arrrived: %d, service time %d \n", process->arrival_time, process->service_time);
            queues[process->priority] = enqPcb(queues[process->priority], process);
        }

        //starvation detection

        /*When the job in front of the Level-1 queue has not been scheduled to run for W  
        units of time since the last time it is pre-empted, all jobs in Level-1 and Level- 2
         queues are moved to the end (or tail) of the Level-0 queue (i.e., their priorities 
          are upgraded to 0). Level-1 jobs are placed before Level-2 jobs and jobs at the  
          same level will maintain their order.*/

        //check if the front of the level 1 queue has not been scheduled to run for W units of time since the last time it is pre-empted
        if (level1_queue && level1_queue->starvation >= W)
        {
           // printf("\nSTARVATION DETECTED - Moving processes to Level 0\n");
            //printf("Process in Level 1 waited %d units without running\n", level1_queue->starvation);
    
            //move all jobs in level 1 and level 2 to the end of the level 0 queue
            while (level1_queue)
            {
                process = deqPcb(&level1_queue);
                process->priority = 0;
                process->starvation = 0; // Reset starvation counter

                level0_queue = enqPcb(level0_queue, process);
            }
            while (level2_queue)
            {
                process = deqPcb(&level2_queue);
                process->priority = 0;
                level0_queue = enqPcb(level0_queue, process);
            }
        }

        //check if the front of the level 2 queue has not been scheduled to run for W units of time since the last time it is pre-empted
        if (level2_queue && level2_queue->starvation >= W)
        {
            //printf("\nSTARVATION DETECTED - Moving processes to Level 0\n");
            //printf("Process in Level 2 waited %d units without running\n", level2_queue->starvation);
    
            //move all jobs in level 2 to the end of the level 0 queue
        
            while (level2_queue)
            {
                process = deqPcb(&level2_queue);
                process->priority = 0;
                process->starvation = 0; // Reset starvation counter
                level0_queue = enqPcb(level0_queue, process);
            }
        }

        // Increment starvation counters for waiting processes
        if (level1_queue) {
            PcbPtr temp = level1_queue;
            while (temp) {
                temp->starvation++;
                temp = temp->next;
            }
        }
        if (level2_queue) {
            PcbPtr temp = level2_queue;
            while (temp) {
                temp->starvation++;
                temp = temp->next;
            }
        }


//    i. If there is a running process:

        if (current_process) //check if there is high priority process
        {   // adjust time in quantum and remaining cpu time
            current_process->time_in_quantum ++;
            current_process->remaining_cpu_time --; // timer changes by 1 each step //todo maybe move this to before premption swamp 
            //printf("just decremented time \n Process %d is running with %d units of time left\n", current_process->pid, current_process->remaining_cpu_time);

             //check if there is high priority process and switch to it first
             //When a new Level-0 job 
             /**When a new Level-0 job 
arrives, the currently running Level-1 job will be pre-empted immediately 
(regardless  of  whether  it  has  used  its  allocated  time-quantum  as  a  Level-1  job  or  
not). If the job has not used its time-quantum, it will be placed at the front (or head) 
of the Level-1 queue. It can then be scheduled to run again when the Level-0 queue 
becomes empty. If it cannot complete within the allocated time-quantum (i.e., if its 
accumulated run time at this level is equal to t1), the job will be moved to the end 
(or tail) of the Level-2 queue (i.e., its priority is downgraded to 2). */
           
           if (current_process->priority == 1){
            //level 1 queue so check that level 0 is empty
            if (level0_queue){
                //suspend the current process and send to back of the queue of the correct queue
                // downgrade

                // if it cannot complete within the allocated time-quantum the job will go to the tail of the level 2 queue
                // if the job has not used its time quantum, it will be placed at the front of the level 1 queue
                if (current_process->remaining_cpu_time > time_quantum[1]) //todo can be current_process->quantum
                {
                    // downgrade to level 2
                    current_process->priority = 2;
                    queues[2] = enqPcb(queues[2], suspendPcb(current_process));
                }
                else
                {
                    // downgrade to front of level 1
                    queues[1] = enqHeadPcb(queues[1], suspendPcb(current_process));
                }
                //start level 0 process
                current_process = deqPcb(&level0_queue);
                current_process = startPcb(current_process);
                current_process->time_in_quantum = 0;


                //recalculate quantum for new process
                //printf("calculating quantum for new higher process\n");
                if (current_process->remaining_cpu_time <= time_quantum[current_process->priority])
                {
                    current_process->quantum = current_process->remaining_cpu_time;
                    //printf("h:REMAING TIME LESS THAN or equal QUANTUM\n quantum set to %d\n", current_process->quantum);
                }
                else
                {
                    current_process->quantum = time_quantum[current_process->priority];
                    //printf("h:REMAING TIME MORE THAN QUANTUM\n quantum set to %d\n", current_process->quantum);

                }

                    //printf("current process priority should be 0: %d\n", current_process->priority);
                    //printf("SWAPPED TO HIGHER PRIORIY, new quantum is %d \n", current_process->quantum);
            }


            //when running a level 2 process, check level 0 and level 1 queues for a higher process
           }
            if (current_process->priority == 2){
                //level 2 queue so check that level 0 and level 1 are empty
                if (level0_queue || level1_queue){
                    //suspend the current process and send to back of the queue of the correct queue
                    // downgrade
                    if (current_process->remaining_cpu_time > time_quantum[2])
                    {
                        // downgrade to level 2
                        queues[2] = enqPcb(queues[2], suspendPcb(current_process));

                    }
                    else 
                    {
                        // downgrade to front of level 2
        
                        queues[2] = enqHeadPcb(queues[2], suspendPcb(current_process));
                    }
                
                    current_process = next(queues); //next function returns the next highest priority process
                    //start process immediately
                    current_process = startPcb(current_process);
                    current_process->time_in_quantum = 0; //reset time in quantum
                //recalculate quantum for new process
                if (current_process->remaining_cpu_time <= time_quantum[current_process->priority])
                {
                    current_process->quantum = current_process->remaining_cpu_time;
                    //printf("h2REMAING TIME LESS THAN or equal QUANTUM\n quantum set to %d\n", current_process->quantum);
                }
                else
                {
                    current_process->quantum = time_quantum[current_process->priority];
                    //printf("h2REMAING TIME MORE THAN QUANTUM\n quantum set to %d\n", current_process->quantum);

                }

                       // printf("current process priority should be 1 or 0: %d\n", current_process->priority);

                        //printf("SWAPPED TO HIGHER PRIORIY, new quantum is %d \n", current_process->quantum);

                }
            }

              

//          b. If the process's allocated time has expired:
            if (current_process->remaining_cpu_time <= 0) 
            {
//              A. Terminate the process;
                terminatePcb(current_process);
//		        calculate and acumulate turnaround time and wait time
                turnaround_time = timer - current_process->arrival_time;
                av_turnaround_time += turnaround_time;
                av_wait_time += turnaround_time - current_process->service_time;
                
//              B. Deallocate the PCB (process control block)'s memory
                free(current_process);
                current_process = NULL;
            }
            // if there are other processes in the other queues queue and process is not done.
            else if ((current_process->time_in_quantum >= current_process->quantum) && (current_process->remaining_cpu_time > 0) )//&& (level0_queue || level1_queue || level2_queue))
            {
                // suspend the current process and send to back of the queue of the correct queue
                // downgrade
                if (current_process->priority == 0)
                {
                    current_process->priority = 1;
                    queues[1] = enqPcb(queues[1], suspendPcb(current_process));
                }
                else if (current_process->priority == 1)
                {
                    current_process->priority = 2;
                    queues[2] = enqPcb(queues[2], suspendPcb(current_process));
                }
                else
                {
                    queues[2] = enqPcb(queues[2], suspendPcb(current_process));
                }
                //printf("checking if status is suspended (4): %d\n", current_process->status);
                current_process = NULL;
                }

            //the process has not run until quantum
            else if (current_process->time_in_quantum < time_quantum[current_process->priority])
            {
               //printf("PROCESS HAS NOT ran until QUANTUM YET, ran for %d and quantum is %d \n",current_process->time_in_quantum, time_quantum[current_process->priority]);
            }

        }

//      ii. If there is no running process and there is a process ready to run:
        if (!current_process && (level0_queue || level1_queue || level2_queue))
        {
//          Dequeue the process at the head of the queue, set it as currently running and start it
            // find what queue to get the process from using custom funciton next
            current_process = next(queues);
           
            //if the process is suspended, resume it
            if (current_process->status == PCB_SUSPENDED)
            {
                //printf("Process %d is running with %d units of time left\n", current_process->pid, current_process->remaining_cpu_time);

                current_process = resumePcb(current_process);
                current_process->time_in_quantum = 0;
                //printf("checking if status is resumed {SHOULD BE Running 3}: %d\n", current_process->status);
        
            }
            //if the process is not suspended, start it
            else{
            current_process = startPcb(current_process);
            current_process->time_in_quantum = 0;
            response_time = timer - current_process->arrival_time;
            av_response_time += response_time;
            }
            //calculate quantum for new process
             //printf("calculating quantum\n");
            if (current_process->remaining_cpu_time <= time_quantum[current_process->priority])
            {
                current_process->quantum = current_process->remaining_cpu_time;
                //printf("REMAING TIME LESS THAN or equal QUANTUM\n quantum set to %d\n", current_process->quantum);
            }
            else
            {
                current_process->quantum = time_quantum[current_process->priority];
                //printf("restart: REMAING TIME MORE THAN QUANTUM\n quantum set to %d\n", current_process->quantum);

            }

        }
      

//      iii. Let the dispatcher sleep for quantum;
//      iv. Increment the dispatcher's timer;

        timer ++;
        sleep(1);
        
    
    }


//  print out average turnaround time and average wait time
    av_turnaround_time = av_turnaround_time / n;
    av_wait_time = av_wait_time / n;
    av_response_time = av_response_time / n;
    printf("average turnaround time = %f\n", av_turnaround_time);
    printf("average wait time = %f\n", av_wait_time);
    printf("average response time = %f\n", av_response_time);
    
//  3. Terminate the FCFS dispatcher
    exit(EXIT_SUCCESS);

}

