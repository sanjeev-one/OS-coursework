#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int no_of_consumers;
int no_of_melons=0; //initialized to 0
int melon_box_capacity; 

void * farmer_routine(void *);
void * consumer_routine(void *);

//declare global mutex and condition variables

... //please fill in


int main(int argc, char ** argv)
{
    pthread_t *threads; //system thread id
	int *t_ids; //user-defined thread id
	int farmer_pace, consumer_rate;
 	int k, rc, t;
	
	// ask for malon box capacity.
	printf("Enter melon box capacity (int): \n");
	scanf("%d", &melon_box_capacity);
	
	// ask for the total number of consumers.
	printf("Enter the total number of consumers (int): \n");
	scanf("%d", &no_of_consumers);
	
	//ask for farmer's working pace
	printf("Enter farmer's working pace (int): \n");
	scanf("%d", &farmer_pace);
	
	//ask for consumers' arrival rate
	printf("Enter consumers arriving rate (int): \n");
	scanf("%d", &consumer_rate);
		
	//Initialize mutex and condition variable objects 

	... //please fill in

	threads = malloc((no_of_consumers+1) * sizeof(pthread_t)); //total is No_Of_Consuers + 1 to include farmer
	if(threads == NULL){
        fprintf(stderr, "threads out of memory\n");
		exit(1);
	}	
	t_ids = malloc((no_of_consumers+1) * sizeof(int)); //total is No_Of_Consuers + 1 to include farmer
	if(t_ids == NULL){
		fprintf(stderr, "t out of memory\n");
		exit(1);
	}	
	
	//create the farmer thread.
	rc = pthread_create(&threads[0], NULL, farmer_routine, (void *) &farmer_pace); //farmer routine takes farmer_pace as the arg
	if (rc) {
		printf("ERROR; return code from pthread_create() (farmer) is %d\n", rc);
		exit(-1);
	}

	//create consumers according to the arrival rate
	srand(time(0));
	for (k = 1; k<no_of_consumers; k++)
	{
		sleep((int)rand() % consumer_rate); //sleep a few second before creating a thread
		t_ids[k] = k;
		rc = pthread_create(&threads[k], NULL, consumer_routine, (void *)&t_ids[k]); //consumer routine takes thread id as the arg
		if (rc) {
			printf("ERROR; return code from pthread_create() (consumer) is %d\n", rc);
			exit(-1);
		}
	}
    
	//join consumer threads.
	for (k = 1; k<no_of_consumers; k++) 
	{
		pthread_join(threads[k], NULL);
	}
	
	//terminate the farmer thread using pthread_cancel().
	pthread_cancel(threads[0]); 
	
	//deallocate allocated memory
	free(threads);
	free(t_ids);
	
	//destroy mutex and condition variable objects
	pthread_mutex_destroy(&melon_box_mutex);
	pthread_cond_destroy(&consumer_cond);
	
	pthread_exit(EXIT_SUCCESS);
}

void * farmer_routine(void * arg)
{
	int *work_pace;
	work_pace = (int *) arg;
	while (1)
	{
		sleep(*work_pace);
        	
		... //please fill in

	}
}

void * consumer_routine(void * arg)
{

	... //please fill in

}
