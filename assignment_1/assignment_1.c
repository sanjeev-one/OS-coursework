#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

int N_no_of_students; // total number of students
int M_no_of_groups;	  // total number of groups
int K_no_of_tutors;	  // total number of tutors / lab rooms
int T_time_limit;	  // time limit for the lab

int *class_lineup; // array to store the lineup of students

int no_of_students_arrived = 0; // number of students arrived

void *teacher_routine(void *);
void *student_routine(void *);

// declare global mutex and condition variables
pthread_mutex_t arriving_mutex = PTHREAD_MUTEX_INITIALIZER; // static initialization

// condition varibles - all_students_arrived, all_students_assigned, one_student_assigned,
pthread_cond_t all_students_arrived, all_students_assigned, group_id_produced, group_id_consumed, teacher_ready, student_arrived;
int all_students_arrived_flag = 0;

int main(int argc, char **argv)
{
	pthread_t *threads; // system thread id
	int rc;
	int *t_ids; // user-defined thread id

	// ask for the total number of students.
	printf("Enter N: the total number of students in the class: ");
	scanf("%d", &N_no_of_students);
	printf("\n\n");

	// ask for number of groups
	printf("Enter M: the number of groups: ");
	scanf("%d", &M_no_of_groups);
	printf("\n\n");

	// ask for the number of tutors
	printf("Enter K: the number of tutors: ");
	scanf("%d", &K_no_of_tutors);
	printf("\n\n");

	// ask for the time limit
	printf("Enter T: the time limit for each group of students to do the lab: ");
	scanf("%d", &T_time_limit);
	printf("\n\n");

	// Declare an array of size M and initialize all elements to 0
	// Dynamically allocate memory for the class_lineup array
	class_lineup = (int *)malloc(N_no_of_students * sizeof(int));
	if (class_lineup == NULL)
	{
		perror("Failed to allocate memory");
		return 1;
	}

	// Initialize the array elements to 0
	for (int i = 0; i < N_no_of_students; i++)
	{
		class_lineup[i] = 0;
	}

	// Initialize condition variable objects //todo add more
	rc = pthread_cond_init(&all_students_arrived, NULL);
	if (rc)
	{
		printf("ERROR; return code from pthread_cond_init() is %d\n", rc);
		exit(-1);
	}
	rc = pthread_cond_init(&all_students_assigned, NULL);
	if (rc)
	{
		printf("ERROR; return code from pthread_cond_init() is %d\n", rc);
		exit(-1);
	}
	rc = pthread_cond_init(&student_arrived, NULL);
	if (rc)
	{
		printf("ERROR; return code from pthread_cond_init() is %d\n", rc);
		exit(-1);
	}
	// todo add tutors
	threads = malloc((N_no_of_students + 1) * sizeof(pthread_t)); // total is no of students + 1 to include teacher
	if (threads == NULL)
	{
		fprintf(stderr, "threads out of memory\n");
		exit(1);
	}
	t_ids = malloc((N_no_of_students + 1) * sizeof(int)); // total is no of students + 1 to include teacher
	if (t_ids == NULL)
	{
		fprintf(stderr, "t out of memory\n");
		exit(1);
	}

	// create the teacher thread.
	rc = pthread_create(&threads[0], NULL, teacher_routine, ); //(void *)&farmer_pace);
	if (rc)
	{
		printf("ERROR; return code from pthread_create() (teacher) is %d\n", rc);
		exit(-1);
	}

	// create student threads

	for (k = 1; k < N_no_of_students; k++)
	{

		t_ids[k] = k;
		rc = pthread_create(&threads[k], NULL, student_routine, (void *)&t_ids[k]);
		if (rc)
		{
			printf("ERROR; return code from pthread_create() (student) is %d\n", rc);
			exit(-1);
		}

		// join threads
		//  join student threads.
		for (k = 1; k < N_no_of_students; k++)
		{
			pthread_join(threads[k], NULL);
		}
		// todo do i need to join teacher thread? - or no bc students wait for teacher to be done
		//  terminate the teacher thread using pthread_cancel().
		pthread_cancel(threads[0]);

		// deallocate allocated memory
		free(threads);
		free(t_ids);

		// destroy mutex and condition variable objects
		// pthread_mutex_destroy(&melon_box_mutex);
		// pthread_cond_destroy(&consumer_cond);

		pthread_exit(EXIT_SUCCESS);

		// exit(0);
	}

	void *teacher_routine(void *arg)
	{

		// find the group size
		//   Each group will have N/M
		// students if N is divisible by M. If N is not divisible by M, the first N%M groups will
		// have ⌊N/M⌋+1 students each and the remaining groups will have ⌊N/M⌋ students each.//
		int group_size = N_no_of_students / M_no_of_groups;
		int remaining_students = N_no_of_students % M_no_of_groups;
		if (remaining_students != 0)
		{
			group_size++;
		} // todo fix

		printf("Teacher: I'm waiting for all students to arrive.\n");

		while (no_of_students_arrived < N_no_of_students)
		{

			pthread_mutex_lock(&arriving_mutex);
			pthread_cond_wait(&student_arrived, &arriving_mutex);
			pthread_mutex_unlock(&arriving_mutex);
		}

		// all the studetns have arrived

		pthread_mutex_lock(&assigning_mutex);

		// wait for all students to be assigned
		while
			!is_all_ones(class_lineup, N_no_of_students) // the box is full
			{
				printf("  Teacher: Iblah blah assign the students  ve.\n");
				// pthread_cond_wait(&all_students_arrived, &arriving_mutex);
			}
		pthread_cond_signal(&all_students_arrived);

		pthread_mutex_unlock(&assigning_mutex);

		sleep(*work_pace);

		if (direction == 0) // arrived at the southern bank
			printf("boat has arrived at the southern bank.\n");
		else // arrived at the northen bank
			printf("boat has arrived at the northern bank.\n");

		direction = (direction + 1) % 2; // change the direction
	}

	void *student_routine(void *arg)
	{
		int *myid;
		myid = (int *)arg;
		printf("Student %d: I have arrived and wait for being assigned to a group.\n", *myid);
		pthread_mutex_lock(&arriving_mutex);
		no_of_students_arrived++;
		pthread_cond_signal(&student_arrived);

		pthread_mutex_unlock(&arriving_mutex);

		// wait for all students to be assigned

		// wait for the teacher to be ready

		if (no_of_melons == 0) // the box is empty
		{
			printf("Consumer %d: Oh no! the melon box is empty and I'll leave without melons!\n", *myid);
			pthread_mutex_unlock(&melon_box_mutex);
			pthread_exit(EXIT_SUCCESS);
		}
		printf("Consumer %d: I'm lucky to get one melon out of %d melons!\n", *myid, no_of_melons);
		no_of_melons--;						 // take one melon from the box
		pthread_cond_signal(&consumer_cond); // signal the farmer one melon is taken from the box
		pthread_mutex_unlock(&melon_box_mutex);
		pthread_exit(EXIT_SUCCESS);
	}
}

// check if all elements in the array are 1
bool is_all_ones(int arr[], int size)
{
	for (int i = 0; i < size; i++)
	{
		if (arr[i] != 1)
		{
			return false; // If any element is not 1, return false
		}
	}
	return true; // If all elements are 1, return true
}