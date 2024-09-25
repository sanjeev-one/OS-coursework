/*
    COMP3520 Exercise 7 - FCFS Dispatcher

    usage:

        ./fcfs <TESTFILE>
        where <TESTFILE> is the name of a job list
*/

/* Include files */
#include "fcfs.h"

int main (int argc, char *argv[])
{
    /*** Main function variable declarations ***/

    FILE * input_list_stream = NULL;
    PcbPtr fcfs_queue = NULL;
    PcbPtr current_process = NULL;
    PcbPtr process = NULL;
    int timer = 0;

    int turnaround_time, response_time;
    double av_turnaround_time = 0.0, av_wait_time = 0.0, av_response_time = 0.0;
    int n = 0;

//  1. Populate the FCFS queue

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

    while (!feof(input_list_stream)) {  // put processes into fcfs_queue
        process = createnullPcb();
        if (fscanf(input_list_stream,"%d, %d",
             &(process->arrival_time), 
             &(process->service_time)) != 2) {
            free(process);
            continue;
        }
        process->remaining_cpu_time = process->service_time;
        process->status = PCB_INITIALIZED;
        fcfs_queue = enqPcb(fcfs_queue, process);
        n++;
    }


//  2. Whenever there is a running process or the FCFS queue is not empty:

    while (current_process || fcfs_queue)
    {
//      i. If there is a currently running process;
        if (current_process)
        {
//          a. Decrement the process's remaining_cpu_time variable;
            current_process->remaining_cpu_time--;
            
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
        }

//      ii. If there is no running process and there is a process ready to run:
        if (!current_process && fcfs_queue && fcfs_queue->arrival_time <= timer)
        {
//          Dequeue the process at the head of the queue, set it as currently running and start it
            current_process = deqPcb(&fcfs_queue);
            startPcb(current_process);
            response_time = timer - current_process->arrival_time;
            av_response_time += response_time;
        }
        
//      iii. Let the dispatcher sleep for one second;
        sleep(1);
        
//      iv. Increment the dispatcher's timer;
        timer++;
        
//      v. Go back to 2.
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
