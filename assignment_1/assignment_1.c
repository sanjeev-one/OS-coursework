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

int *group_lineup; // array to store the groupid of students

int no_of_students_arrived = 0; // number of students arrived

int current_student_id = -1; // current student id

void *teacher_routine(void *);
void *student_routine(void *);

// declare global mutex and condition variables
pthread_mutex_t arriving_mutex = PTHREAD_MUTEX_INITIALIZER;	 // static initialization
pthread_mutex_t assigning_mutex = PTHREAD_MUTEX_INITIALIZER; // static initialization

// condition varibles - all_students_arrived, all_students_assigned, one_student_assigned,
pthread_cond_t  all_students_assigned, student_arrived, id_recieved;

//part 2 vars
pthread_mutex_t lab_room_mutex = PTHREAD_MUTEX_INITIALIZER; // static initialization
pthread_cond_t lab_room_available; // static initialization
int current_lab_room = 0; // counter for lab rooms


int *group_to_queue_map; // New array to map group ID to queue ID

int *lab_queue; // queue to track active labs
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
int queue_front = 0;
int queue_rear = -1;
int queue_size = 0;
bool is_queue_empty() {
    return queue_size == 0;
}

bool is_queue_full() {
    return queue_size == K_no_of_tutors;
}

void enqueue(int group_id) {
    if (!is_queue_full()) {
        queue_rear = (queue_rear + 1) % K_no_of_tutors;
        lab_queue[queue_rear] = group_id;
        queue_size++;
    }
}

int dequeue() {
    int group_id = -1;
    if (!is_queue_empty()) {
        group_id = lab_queue[queue_front];
        queue_front = (queue_front + 1) % K_no_of_tutors;
        queue_size--;
    }
    return group_id;
}


void shuffle(int *array, int n)
{
	for (int i = 0; i < n - 1; i++)
	{
		// Generate a random index j such that i <= j < n
		int j = i + rand() / (RAND_MAX / (n - i) + 1);

		// Swap array[i] with array[j]
		int temp = array[i];
		array[i] = array[j];
		array[j] = temp;
	}
}

int main(int argc, char **argv)
{
	pthread_t *threads; // system thread id
	int rc;
	int *t_ids; // user-defined thread id
	int k;

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
	// Dynamically allocate memory for the group id array
	group_lineup = (int *)malloc(N_no_of_students * sizeof(int));
	if (group_lineup == NULL)
	{
		perror("Failed to allocate memory");
		return 1;
	}

	// Initialize the array elements to -1
	for (int i = 0; i < N_no_of_students; i++)
	{
		group_lineup[i] = -1;
	}

	 // Allocate lab_queue dynamically
    lab_queue = malloc(K_no_of_tutors * sizeof(int));
    if (lab_queue == NULL) {
        perror("Failed to allocate memory for lab_queue");
        return 1;
    }
	

	// Allocate group_to_queue_map
    group_to_queue_map = malloc(M_no_of_groups * sizeof(int));
    if (group_to_queue_map == NULL) {
        perror("Failed to allocate memory for group_to_queue_map");
        free(lab_queue); // Don't forget to free lab_queue if this allocation fails
        return 1;
    }

    // Initialize group_to_queue_map to -1 (indicating no queue assigned)
    for (int i = 0; i < M_no_of_groups; i++) {
        group_to_queue_map[i] = -1;
    }

	// Initialize condition variable objects //todo add more

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
	rc = pthread_cond_init(&id_recieved, NULL);
	if (rc)
	{
		printf("ERROR; return code from pthread_cond_init() is %d\n", rc);
		exit(-1);
	}

	//part 2

	rc = pthread_cond_init(&lab_room_available, NULL);
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
	rc = pthread_create(&threads[0], NULL, teacher_routine, NULL); //(void *)&farmer_pace);
	if (rc)
	{
		printf("ERROR; return code from pthread_create() (teacher) is %d\n", rc);
		exit(-1);
	}

	// create student threads

	for (k = 0; k < N_no_of_students; k++)
	{

		t_ids[k] = k;
		rc = pthread_create(&threads[k], NULL, student_routine, (void *)&t_ids[k]);
		if (rc)
		{
			printf("ERROR; return code from pthread_create() (student) is %d\n", rc);
			exit(-1);
		}
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
	pthread_mutex_lock(&arriving_mutex);
	while (no_of_students_arrived < N_no_of_students)
	{

		pthread_cond_wait(&student_arrived, &arriving_mutex);
	}
	pthread_mutex_unlock(&arriving_mutex);

	// all the students have arrived

	pthread_mutex_lock(&assigning_mutex);

	// wait for all students to be assigned
	// while (no_of_students_arrived == N_no_of_students) // the lineup is full
	//	{
	printf("  Teacher: Iblah blah assign the students  ve.\n");

	//teacher generates assingments
	// each student group id is the mod of their position (student id) with M
	// 1 1 1 1 1 1 1 , N = 7, M = 3 , n/m = 2 + 1, 3 2. 2; 0 1 2 0 1 2 0
	for (int i = 0; i < N_no_of_students; i++)
	{
		group_lineup[i] = i % M_no_of_groups;

		// change to consumer producer
		//printf("Teacher: Student %d is assigned to group %d and cond signaled\n", i, group_lineup[i]);
	}

	// shuffle student Id order to randomize group assignment
	//  when students check what group they are in then it will be random
	//  group lineup index is the student id and the value is what group the student is in
	shuffle(group_lineup, N_no_of_students);
	printf("Teacher: I am shuffling.\n");
	//pthread_mutex_unlock(&assigning_mutex);

//teacher sends out assingments

	for (int i = 0; i < N_no_of_students; i++){

		//Teacher: student [id] is in group [id].
		current_student_id = i;
		printf("Teacher: Student %d is in group %d.\n", i, group_lineup[i]);
		pthread_cond_broadcast(&all_students_assigned);
		//pthread_mutex_unlock(&assigning_mutex);
		pthread_cond_wait(&id_recieved, &assigning_mutex);
	}
	pthread_mutex_unlock(&assigning_mutex);



	printf("Teacher: I have assigned all students to a group.\n");

	//}
	//pthread_cond_broadcast(&all_students_assigned);
/////////////////////////////////////////////////
 /// part 2

 //group id assignment to student - you have done in Exercise 3
while (current_lab_room < M_no_of_groups){
//wait for lab room to become available
printf("Teacher: I’m waiting for lab room to become available\n");
//signal tutor to take group gid for exercise
//signal students in group gid to enter and start exercise
printf("Teacher: The lab is now available. Students in group %d can enter
the room and start your lab exercise.\n", gid);
gid++;
}
//signal tutor to exit
printf("Teacher: There are no students waiting. Tutor, you can go home
now\n");
printf("Teacher: I can now go home.\n");

	return NULL;
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

	pthread_mutex_lock(&assigning_mutex);
	while (current_student_id < *myid) //wait till the array slot is not -1
	{
		//printf("Student %d: I'm waiting to be signaled to check if im assigned to a lab.(group id is %d)\n", *myid, group_lineup[*myid]);
		pthread_cond_wait(&all_students_assigned, &assigning_mutex);
	}
	
	printf("Student %d: I am in group %d.\n", *myid, group_lineup[*myid]);	// this thread have been assigned
	pthread_cond_signal(&id_recieved);
	pthread_mutex_unlock(&assigning_mutex);

  	//part 2

	//group id assignment from teacher – you have done in Exercise 3
//wait for teacher to call to enter lab and conduct exercise
	pthread_mutex_lock(&assigning_mutex);

	printf("Student %d in group %d: My group is called. I will enter the lab room now.\n", *myid, group_lineup[*myid]);
	pthread_mutex_unlock(&assigning_mutex);

//signal tutor after all students in group are in lab
//wait for tutor to call the end of lab exercise
	printf("Student %d in group %d: Thanks Tutor. Bye!\n", *myid, group_lineup[*myid]);
	pthread_mutex_unlock(&assigning_mutex);

//signal tutor the room is vacated




	return NULL;
}

void * tutor_routine(void *arg){
	while(1){
	//wait for teacher to assign a group of students
	printf("Tutor: The lab room is vacated and ready for one group\n");

	//get group id
//if signalled by teacher to exit
printf("Tutor: Thanks Teacher. Bye!\n");
exit
//wait for all students in group gid to enter room
printf("Tutor: All students in group %d have entered the room. You can
start your exercise now.\n", gid);
//students in group gid conduct the lab exercise
//signal students the end of exercise
printf("Tutor: Students in group %d have completed the lab exercise in %d
units of time. You may leave this room now.\n", gid, ex_time);
//wait for lab to become empty
//signal teacher the room is vacated






	}

	return NULL;
}
