#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct passenger_object
{
    int id;
    char direction[4];
} passenger_obj;

void * boat_routine(void *);
void * passenger_routine(void *);

//declare global mutex and condition variables
pthread_mutex_t boating_mutex = PTHREAD_MUTEX_INITIALIZER; //static initialization

// condition varibles - all_students_arrived, all_students_assigned, one_student_assigned,  
pthread_cond_t all_students_arrived, all_students_assigned, one_student_assigned;

int main(int argc, char ** argv)
{
	int N_no_of_students; //total number of students
	int M_no_of_groups; //total number of groups
	int K_no_of_tutors; //total number of tutors / lab rooms
	int T_time_limit; //time limit for the lab

	

	// ask for the total number of students.
	printf("Enter N: the total number of students in the class: ");
	scanf("%d", &N_no_of_students);
	printf("\n\n");
	
	//ask for number of groups
	printf("Enter M: the number of groups: ");
	scanf("%d", &M_no_of_groups);
	printf("\n\n");
	
	//ask for the number of tutors
	printf("Enter K: the number of tutors: ");
	scanf("%d", &K_no_of_tutors);
	printf("\n\n");

	//ask for the time limit
	printf("Enter T: the time limit for each group of students to do the lab: ");
	scanf("%d", &T_time_limit);
	printf("\n\n");
				
	// Declare an array of size M and initialize all elements to 0
	int c[N_no_of_students] = {0}; // initialize all elements to 0

	

    exit(0);
}

void * teacher_routine(void * arg)
{
	printf("Teacher: I'm waiting for all students to arrive.\n");
    while (1)
    {
        pthread_mutex_lock(&boating_mutex);
        if (direction == 0){//moving n2s
            printf("boat to move n2s.\n");
            pthread_cond_signal(&n2s_cond); //take one passenger from n2s if any
        }
        else{//moving s2n
            printf("boat to move s2n.\n");
            pthread_cond_signal(&s2n_cond); //take one passenger from s2n if any
        }
        pthread_mutex_unlock(&boating_mutex);

        sleep (*work_pace);

        if (direction == 0) //arrived at the southern bank
            printf("boat has arrived at the southern bank.\n");
        else //arrived at the northen bank
            printf("boat has arrived at the northern bank.\n");
    
        direction = (direction+1) % 2; //change the direction
    }
}

void * student_routine(void * arg)
{
	passenger_obj *myid;
	
	myid = (passenger_obj *)arg;
    printf("Passenger %s %d arrives and waits to cross the river.\n", myid->direction, myid->id);
    pthread_mutex_lock(&boating_mutex);
    if (!strcmp(myid->direction, "n2s")){
		pthread_cond_wait(&n2s_cond, &boating_mutex);
		printf("passenger %s %d is now on the boat.\n", myid->direction, myid->id);	
	}		
	else if (!strcmp(myid->direction, "s2n")){
		pthread_cond_wait(&s2n_cond, &boating_mutex);
		printf("passenger %s %d: on the boat.\n", myid->direction, myid->id);
	}		
    pthread_mutex_unlock(&boating_mutex);
    pthread_exit(EXIT_SUCCESS);

}
