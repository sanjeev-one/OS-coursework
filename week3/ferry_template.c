#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct passenger_object
{
    int id;
    char direction[4]; //either n2s or n2s
} passenger_obj;

void * boat_routine(void *); 
void * passenger_routine(void *);

//declare global mutex and condition variables
pthread_mutex_t boating_mutex;
pthread_cond_t n2s_cond;
pthread_cond_t s2n_cond;

char boat_direction[4] = "n2s"; //initial direction


int main(int argc, char ** argv)
{
	int no_of_passengers; //total number of passengers to be created
	int boat_pace, passenger_rate;
	pthread_t *c_thrd_ids, b_thrd_id; //system thread id
	passenger_obj *passenger; //user-defined thread id
 	int drct, n_c, s_c;
	int k, rc;
	
	// ask for the total number of passengers.
	printf("Enter the total number of passengers (int): ");
	scanf("%d", &no_of_passengers);
        printf("\n\n");
	
	//ask for boat's working pace
	printf("Enter boat's working pace (int): ");
	scanf("%d", &boat_pace);
        printf("\n\n");
	
	//ask for passengers' arrival rate
	printf("Enter passengers arrival rate (int): ");
	scanf("%d", &passenger_rate);
        printf("\n\n");
		
	c_thrd_ids = malloc((no_of_passengers) * sizeof(pthread_t)); //passenger thread ids
	if(c_thrd_ids == NULL){
		fprintf(stderr, "threads out of memory\n");
		exit(1);
	}	
	
	passenger = malloc((no_of_passengers) * sizeof(passenger_obj)); //total is no_Of_passengers 
	if(passenger == NULL){
		fprintf(stderr, "t out of memory\n");
		exit(1);
	}	

	//Initialize condition variable objects 

	if(pthread_mutex_init(&boating_mutex, NULL) != 0) {
		fprintf(stderr, "Error: mutex init failed\n");
		exit(1);
	}

	if (pthread_cond_init(&s2n_cond, NULL) != 0) {
		fprintf(stderr, "Error: cond init failed\n");
		exit(1);
	}

	if (pthread_cond_init(&n2s_cond, NULL) != 0) {
		fprintf(stderr, "Error: cond init failed\n");
		exit(1);
	}
	
	

	
	//create the boat thread.
	if (pthread_create(&b_thrd_id, NULL, boat_routine, (void *) &boat_pace)){
		printf("ERROR; return code from pthread_create() (boat) is %d\n", rc);
		exit(-1);
	}

	//create consumers according to the arrival rate
	n_c = s_c = 0;
	srand(time(0));
    for (k = 0; k<no_of_passengers; k++)
    {
		sleep((int)rand() % passenger_rate); 
		drct = (int)rand()% 2;
		if (drct == 0){
			strcpy(passenger[k].direction, "n2s");
			passenger[k].id = n_c;
			n_c++;
		}
		else if (drct == 1){
			strcpy(passenger[k].direction, "s2n");
			passenger[k].id = s_c;
			s_c++;
		}
		rc = pthread_create(&c_thrd_ids[k], NULL, passenger_routine, (void *)&passenger[k]);
		if (rc) {
			printf("ERROR; return code from pthread_create() (consumer) is %d\n", rc);
			exit(-1);
		}
    }
    
	//join passenger threads
    for (k = 0; k<no_of_passengers; k++) 
    {
		pthread_join(c_thrd_ids[k], NULL);
    }
	
	//After all passenger threads exited, terminate the cashier thread
    pthread_cancel(b_thrd_id); 
			
	//deallocate allocated memory
	free(c_thrd_ids);
	free(passenger);

	//destroy mutex and condition variable objects
	pthread_mutex_destroy(&boating_mutex);
	pthread_cond_destroy(&n2s_cond);
	pthread_cond_destroy(&s2n_cond);	

    exit(0);
}

void * boat_routine(void * arg)
{

	// you need to write a routine for boat.
	
	while (1)
	{
	pthread_mutex_lock(&boating_mutex);

	if (boat_direction == "n2s"){
		pthread_cond_signal(&n2s_cond);
	}
	else if (boat_direction == "s2n"){
		pthread_cond_signal(&s2n_cond);
		
	}
	pthread_mutex_unlock(&boating_mutex);
	sleep (*(int *) arg);

	if (boat_direction == "n2s"){
		printf("The ferry has arrived at the southern bank.\n");
	}
	else if (boat_direction == "s2n"){
		printf("The ferry has arrived at the northern bank.\n");
		
	}
    strcpy(boat_direction, strcmp(boat_direction, "n2s") == 0 ? "s2n" : "n2s");

	}
	
}

void * passenger_routine(void * arg)
{
//passenger appears
pthread_mutext_lock(&boating_mutex);
if (strcmp(((passenger_obj *)arg)->direction, "n2s") == 0){
	pthread_cond_wait(&n2s_cond, &boating_mutex);
	


}
else if (strcmp(((passenger_obj *)arg)->direction, "s2n") == 0){
	pthread_cond_wait(&s2n_cond, &boating_mutex);
	

}
}