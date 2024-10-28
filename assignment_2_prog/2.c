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

    PcbPtr job_dispatch_queue = NULL;
    PcbPtr round_robin_queue = NULL;

    PcbPtr queues[3] = {NULL, NULL, NULL};
    #define level0_queue queues[0]
    #define level1_queue queues[1]
    #define level2_queue queues[2]

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
    int time_quantum[3] = {t0, t1, t2};
    //int quantum;

    
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


//  1. Populate the job_dispatch queue
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
        printf("top \n timer: %d\n", timer);

        //Unload any arrived pending processes from the Job Dispatch queue  dequeue  process  from  Job  Dispatch  queue  and  enqueue  on  Round  Robin queue;
        while (job_dispatch_queue && job_dispatch_queue->arrival_time <= timer)
        {
            process = deqPcb(&job_dispatch_queue);
            queues[process->priority] = enqPcb(queues[process->priority], process);
        }

        //starvation stuff


//      i. If there is a currently running process;
        if (current_process) //check if there is high priority process
        {   

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
                if (current_process->service_time - current_process->remaining_cpu_time == time_quantum[1])
                {
                    current_process->priority = 2;
                    queues[2] = enqPcb(queues[2], suspendPcb(current_process));
                }
                else
                {
                   //current_process->priority = 1;
                    queues[1] = enqHeadPcb(queues[1], suspendPcb(current_process));
                }
                current_process->status = PCB_SUSPENDED;
                current_process = deqPcb(&level0_queue);
            }
           }
            if (current_process->priority == 2){
                //level 2 queue so check that level 0 and level 1 are empty
                if (level0_queue || level1_queue){
                    //suspend the current process and send to back of the queue of the correct queue
                    // downgrade
                    if (current_process->service_time - current_process->remaining_cpu_time == time_quantum[2])
                    {
                        queues[2] = enqPcb(queues[2], suspendPcb(current_process));

                    }
                    else 
                    {
        
                        queues[2] = enqHeadPcb(queues[2], suspendPcb(current_process));
                    }
                
                    current_process->status = PCB_SUSPENDED;
                    current_process = next(queues);
                }
            }


//          a. Decrement the process's remaining_cpu_time variable by quantum;
            current_process->remaining_cpu_time -= current_process->quantum; //each process has a different quantum
            printf("just decremented time \n Process %d is running with %d units of time left\n", current_process->pid, current_process->remaining_cpu_time);



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
            // if there are other processes in the other queues queue and process is not done.
            else if (level0_queue || level1_queue || level2_queue)
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
                printf("checking if status is suspended: %d\n", current_process->status);

            
                current_process->status = PCB_SUSPENDED;
                current_process = NULL;
                }



        }

//      ii. If there is no running process and there is a process ready to run:
        if (!current_process && (level0_queue || level1_queue || level2_queue))
        {
//          Dequeue the process at the head of the queue, set it as currently running and start it
            // find what queue to get the process from

            current_process = next(queues);
           
            //if the process is suspended, resume it
            if (current_process->status == PCB_SUSPENDED)
            {
                //printf("Process %d is running with %d units of time left\n", current_process->pid, current_process->remaining_cpu_time);

                current_process = resumePcb(current_process);
                //todo test if process status changes
                printf("checking if status is resumed {SHOULD BE RESUMED}: %d\n", current_process->status);
        
            }
            else{
            current_process = startPcb(current_process);
            }

            response_time = timer - current_process->arrival_time;
            av_response_time += response_time;
        }
        //calculate quantum from time_quantum for the new process
        if(current_process){
            print("calculating quantum\n");
            if (current_process->remaining_cpu_time  - time_quantum[current_process->priority] >= 0){
                current_process->quantum = time_quantum[current_process->priority];
            }
            else 
            {
               current_process->quantum = current_process->remaining_cpu_time;
            }
        printf("setting quantum to %d\n", current_process->quantum);

        
        }
        else{
            printf("setting quantum to 1\n");
            current_process->quantum = 1;
        }
        printf("sleeping \n\n current process quantum: %d\n", current_process->quantum);
        sleep(current_process->quantum);

//      iii. Let the dispatcher sleep for quantum;
        
        
//      iv. Increment the dispatcher's timer;
        timer += current_process->quantum;
        
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




    // problem, when there is time waiting before the first process, then the quantum is set to be the time waiting and the time_quatum.

}

