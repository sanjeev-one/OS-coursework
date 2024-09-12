#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

void initialize_cond_var(pthread_cond_t *cond) {
	int rc = pthread_cond_init(cond, NULL);
	if (rc) {
		fprintf(stderr, "pthread_cond_init: %s\n", strerror(rc));
		exit(1);
	}
}


int N_no_of_students; // total number of students
int M_no_of_groups;	  // total number of groups
int K_no_of_tutors;	  // total number of tutors / lab rooms
int T_time_limit;	  // time limit for the lab

int *group_lineup; // array to store the groupid of students

int no_of_students_arrived = 0; // number of students arrived

int current_student_id = -1; // current student id

void *teacher_routine(void *);
void *student_routine(void *);
void *tutor_routine(void *);

// declare global mutex and condition variables
pthread_mutex_t arriving_mutex = PTHREAD_MUTEX_INITIALIZER;	 // static initialization
pthread_mutex_t assigning_mutex = PTHREAD_MUTEX_INITIALIZER; // static initialization

// condition varibles - all_students_arrived, all_students_assigned, one_student_assigned,
pthread_cond_t  a_student_assigned, student_arrived, id_recieved;

//part 2 vars
pthread_mutex_t lab_room_map_mutex = PTHREAD_MUTEX_INITIALIZER; // static initialization
pthread_mutex_t lab_room_size_capacity = PTHREAD_MUTEX_INITIALIZER; // static initialization
pthread_cond_t lab_room_available, students_enter_lab,all_students_entered,students_lab_over,all_students_left_lab, tutor_go_home, tutor_went_home; // static initialization
int current_lab_room = 0; // counter for lab rooms


int *group_to_lab_map; // New array to map group ID(index) to lab ID (value)
int *lab_to_group_map; // New array to map lab ID(index) to group ID (value)
int *lab_room_capacity; // New array to store the number of students in each lab room
int *lab_queue; // queue to track active labs
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
int queue_front = 0;
int queue_rear = -1;
int queue_size = 0;

int group_size(int group_id) {
	int group_size = N_no_of_students / M_no_of_groups;
	int remaining_students = N_no_of_students % M_no_of_groups;
	if (remaining_students != 0) {
		group_size++;
	}
	return group_size;
}


bool is_queue_empty() {
    return queue_size == 0;
}

bool is_queue_full() {
    return queue_size == K_no_of_tutors;
}

void enqueue(int lab_id) {
    if (!is_queue_full()) {
        queue_rear = (queue_rear + 1) % K_no_of_tutors;
        lab_queue[queue_rear] = lab_id;
        queue_size++;
    }
}

int dequeue() {
    int lab_id = -1;
    if (!is_queue_empty()) {
        lab_id = lab_queue[queue_front];
        queue_front = (queue_front + 1) % K_no_of_tutors;
        queue_size--;
    }
    return lab_id;
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
	

	// Allocate group_to_lab_map
    group_to_lab_map = malloc(M_no_of_groups * sizeof(int));
    if (group_to_lab_map == NULL) {
        perror("Failed to allocate memory for group_to_lab_map");
        free(lab_queue); // Don't forget to free lab_queue if this allocation fails
        return 1;
    }

    // Initialize group_to_lab_map to -1 (indicating no queue assigned)
    for (int i = 0; i < M_no_of_groups; i++) {
        group_to_lab_map[i] = -1;
    }



	// Allocate lab_to_group_map
    lab_to_group_map = malloc(K_no_of_tutors * sizeof(int));
    if (lab_to_group_map == NULL) {
        perror("Failed to allocate memory for lab_to_group_map");
        free(lab_queue); // Don't forget to free lab_queue if this allocation fails
        return 1;
    }

    // Initialize lab_to_group_map to -1 (indicating no queue assigned)
    for (int i = 0; i < K_no_of_tutors; i++) {
        lab_to_group_map[i] = -1;
    }



	// Allocate lab_room_capacity
    lab_room_capacity = malloc(K_no_of_tutors * sizeof(int));
    if (lab_room_capacity == NULL) {
        perror("Failed to allocate memory for lab_room_capacity");
        free(lab_queue);//todo free others // Don't forget to free lab_queue if this allocation fails
        return 1;
    }

    // Initialize lab_room_capacity to 0 
    for (int i = 0; i < K_no_of_tutors; i++) {
        lab_room_capacity[i] = 0;
    }

	// Initialize condition variable objects //todo add more
	initialize_cond_var(&lab_room_available);
    initialize_cond_var(&students_enter_lab);
    initialize_cond_var(&all_students_entered);
    initialize_cond_var(&students_lab_over);
    initialize_cond_var(&all_students_left_lab);
    initialize_cond_var(&tutor_go_home);
    initialize_cond_var(&tutor_went_home);
	initialize_cond_var(&a_student_assigned);

	

	threads = malloc((N_no_of_students + K_no_of_tutors + 1) * sizeof(pthread_t)); // total is no of students + k + 1 to include teacher
	if (threads == NULL)
	{
		fprintf(stderr, "threads out of memory\n");
		exit(1);
	}
	t_ids = malloc((N_no_of_students + K_no_of_tutors ) * sizeof(int)); // total is no of students and no of tutors 
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
		rc = pthread_create(&threads[k+1], NULL, student_routine, (void *)&t_ids[k]);
		if (rc)
		{
			printf("ERROR; return code from pthread_create() (student) is %d\n", rc);
			exit(-1);
		}
	}

	//create tutor threads
	for (k = 0; k < K_no_of_tutors; k++)
	{

		t_ids[k+N_no_of_students] = k;
		rc = pthread_create(&threads[k+1+N_no_of_students], NULL, tutor_routine, (void *)&t_ids[k+N_no_of_students]);
		if (rc)
		{
			printf("ERROR; return code from pthread_create() (tutor) is %d\n", rc);
			exit(-1);
		}
	}

	// join threads
	//  join student  and teacher threads. tutors are in a loop
	for (k = 0; k < N_no_of_students +1; k++)
	{
		pthread_join(threads[k], NULL);
	}
	//  terminate the teacher thread using pthread_cancel().
	//pthread_cancel(threads[0]);

	// deallocate allocated memory
	free(threads);
	free(t_ids);

	// destroy mutex and condition variable objects
	// pthread_mutex_destroy(&melon_box_mutex);
	// pthread_cond_destroy(&consumer_cond);

	pthread_exit(EXIT_SUCCESS);

	 exit(0);
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
		pthread_cond_broadcast(&a_student_assigned);
		//pthread_mutex_unlock(&assigning_mutex);
		pthread_cond_wait(&id_recieved, &assigning_mutex);
	}
	pthread_mutex_unlock(&assigning_mutex);



	printf("Teacher: I have assigned all students to a group.\n");

	//}
	//pthread_cond_broadcast(&a_student_assigned);
/////////////////////////////////////////////////
 /// part 2

 //group id assignment to student - you have done in Exercise 3
// 2 arrays- one to map whcih group (index) is in what lab room (value)
// one to show how many studetns have gotten into the lab room

// one queue to track which labs are available


 int group_id = 0;
 	pthread_mutex_lock(&lab_room_map_mutex);
while (group_id < M_no_of_groups){
	//wait for a lab room to become available
	printf("Teacher: I’m waiting for a lab room to become available\n");
	pthread_cond_wait(&lab_room_available, &lab_room_map_mutex);


	// get which lab is available
	pthread_mutex_lock(&queue_mutex);
	int lab_id = dequeue();
	pthread_mutex_unlock(&queue_mutex);

	// maybe use diff mutex - lab room map is locked rn

	// setup the array values
	group_to_lab_map[group_id] = lab_id;
	lab_to_group_map[lab_id] = group_id;

	
	pthread_cond_broadcast(&students_enter_lab);
	printf("Teacher: The lab %d is now available. Students in group %d can enter the room and start your lab exercise.\n", lab_id, group_id);
	
	group_id++;

}
pthread_mutex_unlock(&lab_room_map_mutex);


//signal tutor to exit
printf("Teacher: There are no students waiting. Tutor, you can go home now\n");
	pthread_cond_broadcast(&tutor_go_home);

//wait for tutor to exit //todo loop count tutors leaving till all tutors are done
int tutor_count_left = 0;
pthread_mutex_lock(&lab_room_map_mutex);
while(tutor_count_left < K_no_of_tutors){
	pthread_cond_wait(&tutor_went_home, &lab_room_map_mutex);
	tutor_count_left++;
}
pthread_mutex_unlock(&lab_room_map_mutex);


printf("Teacher: I can now go home.\n");
	pthread_exit(EXIT_SUCCESS);
	return NULL;
}

void *student_routine(void *arg)
{
	int *myid;
	int group_id;
	int lab_id;
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
		pthread_cond_wait(&a_student_assigned, &assigning_mutex);
	}
	
	printf("Student %d: I am in group %d.\n", *myid, group_lineup[*myid]);
	group_id = group_lineup[*myid];
		// this thread have been assigned
	pthread_cond_signal(&id_recieved);
	pthread_mutex_unlock(&assigning_mutex);

  	//part 2

	//group id assignment from teacher – you have done in Exercise 3
//wait for teacher to call to enter lab and conduct exercise
	pthread_mutex_lock(&lab_room_map_mutex);
	lab_id = group_to_lab_map[group_id];
	printf("Student %d in group %d: My group is called. I will enter the lab %d room now.\n", *myid, group_id, lab_id);
	pthread_mutex_unlock(&lab_room_map_mutex);

	//increase capacity of lab room array 

	pthread_mutex_lock(&lab_room_size_capacity);
	lab_room_capacity[lab_id]++;
	if (lab_room_capacity[lab_id] == group_size(group_id)){
		pthread_cond_broadcast(&all_students_entered);
	}
	pthread_mutex_unlock(&lab_room_size_capacity);
	// tutor should now signal to start lab exercise


//signal tutor after all students in group are in lab
//wait for tutor to call the end of lab exercise
// check array to make sure lab is done
	pthread_mutex_lock(&lab_room_size_capacity);
	while(lab_room_capacity[lab_id] > 0){ //tutor will switch capacity to negative of the max capacity
		pthread_cond_wait(&students_lab_over, &lab_room_size_capacity);
	}
	//students increment lab room capacity back to 0 as they leave
	lab_room_capacity[lab_id]++;


	printf("Student %d in group %d: Thanks Tutor %d. Bye!\n", *myid, group_id, lab_id);
	// lab capacity now reset to 0

//signal tutor the room is vacated
	if (lab_room_capacity[lab_id] == 0){
		pthread_cond_broadcast(&all_students_left_lab);
	}



	return NULL;
}

void * tutor_routine(void *arg){
	while(1){
	int gid;
	

	printf("Tutor: The lab room is vacated and ready for one group\n");
	// add labid to queue to show its available
	pthread_mutex_lock(&queue_mutex);
	enqueue(*(int *)arg);
	pthread_mutex_unlock(&queue_mutex); //todo if tutor goes through lab before teacher assigns next lab - fix last student changes lab id to -1
	pthread_cond_broadcast(&lab_room_available);
	//wait for teacher to assign a group of students

	//todo if tutor goes through lab before teacher assigns next lab - fix last student changes lab id to -1
	//todo wait for teacher to asign students to lab to know which group id is corresponding to this lab id - 	int gid = lab_to_group_map[*(int *)arg];
	gid = lab_to_group_map[*(int *)arg];
	pthread_mutex_lock(&lab_room_size_capacity);
	while(group_to_lab_map[gid] == -1){
		pthread_cond_wait(&students_enter_lab, &lab_room_size_capacity);
		gid = lab_to_group_map[*(int *)arg];
	}
	pthread_mutex_unlock(&lab_room_size_capacity);

	// //if signalled by teacher to exit
	// if exit
	// printf("Tutor: Thanks Teacher. Bye!\n");
	// exit
	// if lab index is -2 then exit
	if (lab_to_group_map[*(int *)arg] == -2){
		printf("Tutor: Thanks Teacher. Bye!\n");
		pthread_exit(EXIT_SUCCESS); //todo or exit?
	}



	//wait for students to enter lab
	// while index of lab group value (size of students in lab ) is smaller than group size
	// wait for students to enter lab
	pthread_mutex_lock(&lab_room_size_capacity);
	while(lab_room_capacity[*(int *)arg] < group_size(gid)){
		pthread_cond_wait(&students_enter_lab, &lab_room_size_capacity);
	}
	pthread_mutex_unlock(&lab_room_size_capacity);
	printf("Tutor: All students in group %d have entered the room. You can start your exercise now.\n", gid);
	//students in group gid conduct the lab exercise
	//signal students the end of exercise

	//get group id
	//wait time for lab exercise
	// A group may take a minimum of T/2 units of time and a maximum of T units of time to complete  their  lab  exercise  (in  your  program,  a  unit  of  time  is  assumed  to  be  one   second).
	int ex_time = (rand() % (T_time_limit - T_time_limit/2 + 1)) + T_time_limit/2;
	sleep(ex_time);
	pthread_mutex_lock(&lab_room_size_capacity);
	lab_room_capacity[*(int *)arg] = -group_size(gid); //students increment lab room capacity array
	pthread_mutex_unlock(&lab_room_size_capacity);

	pthread_cond_broadcast(&students_lab_over); //students go decrement lab room capacity array
	printf("Tutor: Students in group %d have completed the lab exercise in %d units of time. You may leave this room now.\n", gid, ex_time);
//wait for lab to become empty
	pthread_mutex_lock(&lab_room_size_capacity);
	while (lab_room_capacity[*(int *)arg] < 0){
		pthread_cond_wait(&all_students_left_lab, &lab_room_size_capacity);
	}
	pthread_mutex_unlock(&lab_room_size_capacity);



//signal teacher the room is vacated -- at top
	// pthread_cond_broadcast(&lab_room_available); //todo hmmm






	}

	return NULL;
}


//todo last student changes lab id to -1 - 
//todo make sure excersise time does not lock mutex to let other threads do stuff