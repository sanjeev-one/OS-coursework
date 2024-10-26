/*
    COMP3520 Exercise 7 - FCFS Dispatcher

    usage:

        ./fcfs <TESTFILE>
        where <TESTFILE> is the name of a job list
*/

/* Include files */
#include "fcfs.h"



PcbPtr job_dispatch_queue = NULL;
PcbPtr round_robin_queue = NULL;

PcbPtr level0_queue = NULL;
PcbPtr level1_queue = NULL;
PcbPtr level2_queue = NULL;



PcbPtr findspot(PcbPtr process)
{
    /* takes in a process from job dispatch and returns the queue it should join */
    switch (process->priority)
    {
    case 0:
        return &level0_queue;
    case 1:
        return &level1_queue;
    case 2:
        return &level2_queue;
    default:
        fprintf(stderr, "ERROR: Invalid priority level %d\n", process->priority);
        return NULL;
    }
}


int main (int argc, char *argv[])
{
    /*** Main function variable declarations ***/

    FILE * input_list_stream = NULL;
    
    PcbPtr current_process = NULL;
    PcbPtr process = NULL;
    int timer = 0;

    int turnaround_time, response_time;
    double av_turnaround_time = 0.0, av_wait_time = 0.0, av_response_time = 0.0;
    int n = 0;

    //ask for time_quantum
    int t0;
    int t1;
    int t2;

    int quantum;

    
    printf("Enter an integer value for t0: ");
    if (scanf("%d", &t0) != 1) {
        fprintf(stderr, "Error: Invalid input for t0\n");
        exit(EXIT_FAILURE);
    }
    printf("Enter an integer value for t1: ");
    if (scanf("%d", &t1) != 1) {
        fprintf(stderr, "Error: Invalid input for t0\n");
        exit(EXIT_FAILURE);
    }
    printf("Enter an integer value for t1: ");
    if (scanf("%d", &t1) != 1) {
        fprintf(stderr, "Error: Invalid input for t1\n");
        exit(EXIT_FAILURE);
    }


//  1. Populate the job_dispatch queue

    if (argc <= 0)
    {
        fprintf(stderr, "FATAL: Bad arguments array\n");
        exit(EXIT_FAILURE);
    }
    else if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <TESTFILE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!(input_list_stream = fopen(argv[1], "r")))
    {
        fprintf(stderr, "ERROR: Could not open \"%s\"\n", argv[1]);
        exit(EXIT_FAILURE);
    }

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
    
    fprintf(stderr, "INFO: Read %d processes from \"%s\"\n", n, argv[1]);


//  2. Whenever there is a running process or the job_dispatch queue or round robin queue is not empty:

    while (current_process || job_dispatch_queue || level0_queue || level1_queue || level2_queue)
    {

        //Unload any arrived pending processes from the Job Dispatch queue  dequeue  process  from  Job  Dispatch  queue  and  enqueue  on  Round  Robin queue;
        while (job_dispatch_queue && job_dispatch_queue->arrival_time <= timer)
        {
            process = deqPcb(&job_dispatch_queue);
            *findspot(process) = enqPcb(*findspot(process), process);
        }



//      i. If there is a currently running process;
        if (current_process)
        {
//          a. Decrement the process's remaining_cpu_time variable by quantum;
            current_process->remaining_cpu_time -= quantum;
            
//          b. If the process's allocated time has expired:
            if (current_process->remaining_cpu_time == 0)
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
            // if there are other processes in the round robin queue.
            else if (round_robin_queue)
            {
                // suspend the current process and send to back of the queue
                round_robin_queue = enqPcb(round_robin_queue, suspendPcb(current_process));
                current_process->status = PCB_SUSPENDED;
                current_process = NULL;
                }



        }

//      ii. If there is no running process and there is a process ready to run:
        if (!current_process && round_robin_queue)
        {
//          Dequeue the process at the head of the queue, set it as currently running and start it
            current_process = deqPcb(&round_robin_queue);
           
            //if the process is suspended, resume it
            if (current_process->status == PCB_SUSPENDED)
            {
                //printf("Process %d is running with %d units of time left\n", current_process->pid, current_process->remaining_cpu_time);

                current_process = resumePcb(current_process);
        
            }
            else{
            current_process = startPcb(current_process);
            }

            response_time = timer - current_process->arrival_time;
            av_response_time += response_time;
        }
        //calculate quantum from time_quantum
        if(current_process){
            if (current_process->remaining_cpu_time  - time_quantum >= 0){
                quantum = time_quantum ;
            }
            else 
            {
               quantum = current_process->remaining_cpu_time;
            }
            
        
        }
        else{
            quantum = 1;
        }
        sleep(quantum);

//      iii. Let the dispatcher sleep for quantum;
        
        
//      iv. Increment the dispatcher's timer;
        timer += quantum;
        
//      v. Go back to 2.
    }

    // start process,  at bottom then compute quantum - its how much time is left for each process. if service time is less that time quantum then  set to service time. if service time is greater than time quantum then set to time quantum. if there is no current process then set quantum to 1. so it will catch later processes coming in. 

//  print out average turnaround time and average wait time
    av_turnaround_time = av_turnaround_time / n;
    av_wait_time = av_wait_time / n;
    av_response_time = av_response_time / n;
    printf("average turnaround time = %f\n", av_turnaround_time);
    printf("average wait time = %f\n", av_wait_time);
    printf("average response time = %f\n", av_response_time);
    
//  3. Terminate the FCFS dispatcher
    exit(EXIT_SUCCESS);



//todo fix response time

    // problem, when there is time waiting before the first process, then the quantum is set to be the time waiting and the time_quatum.

}

