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

/*

•	Lab Queue: Holds available labs (|lab_queue|).
•	lab_to_group_map: Maps lab ID (index) to group ID (value). When a group leaves, it is reset to |-2| (lab empty).
•	group_to_lab_map: Maps group ID (index) to lab ID (value).
•	lab_room_capacity: Tracks how many students are in each lab. 
•	tutor_status: Manages tutor states:
o	|-1| (waiting for teacher),
o	|0| (in the lab queue),
o	|1| (assigned a group),
o	|2| (ready for students),
o	|3| (exiting).
•	teacher_status: Manages teacher states
o	|0| (doing work),
o	|1| (waiting for a tutor to assign lab),
o	|2| (group assigned),
o	|3| (tutors can go home).


 */
int N_no_of_students; // total number of students
int M_no_of_groups;	  // total number of groups
int K_no_of_tutors;	  // total number of tutors / lab rooms
int T_time_limit;	  // time limit for the lab

int *group_lineup; // array to store the groupid of students

int *tutor_status; // array to store the status of tutors

int no_of_students_arrived = 0; // number of students arrived
int tutor_count_left = 0; //exiting tutor counter

int current_student_id = -1; // current student id

int teacher_status = 0; // 0 - part 1, 1 - part 2
//int *tutor_status; // 
void *teacher_routine(void *);
void *student_routine(void *);
void *tutor_routine(void *);

// declare global mutex and condition variables
pthread_mutex_t arriving_mutex = PTHREAD_MUTEX_INITIALIZER;	 // static initialization
pthread_mutex_t assigning_mutex = PTHREAD_MUTEX_INITIALIZER; // static initialization

// condition variables - 
pthread_cond_t  a_student_assigned, student_arrived, id_recieved; // static initialization

//part 2 vars
pthread_mutex_t tutor_status_mutex = PTHREAD_MUTEX_INITIALIZER; // static initialization
pthread_mutex_t lab_room_map_mutex = PTHREAD_MUTEX_INITIALIZER; // static initialization
pthread_mutex_t lab_room_size_capacity = PTHREAD_MUTEX_INITIALIZER; // static initialization
pthread_mutex_t teacher_status_mutex = PTHREAD_MUTEX_INITIALIZER; // static initialization
pthread_mutex_t tutor_left_mutex = PTHREAD_MUTEX_INITIALIZER; // static initialization
pthread_cond_t lab_room_available,group_assigned,tutor_ready_for_students,teacher_waiting_for_available_lab, students_can_enter_lab,all_students_entered,students_lab_over,all_students_left_lab, tutor_go_home, tutor_went_home; // static initialization
int current_lab_room = 0; // counter for lab rooms
//flag for part 2 for tutors
int start_part2 = 0;
pthread_mutex_t start_part2_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start_part2_cv;


int *group_to_lab_map; // New array to map group ID(index) to lab ID (value)
int *lab_to_group_map; // New array to map lab ID(index) to group ID (value)
int *lab_room_capacity; // New array to store the number of students in each lab room
int *lab_queue; // queue to track active labs
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
int queue_front = 0;
int queue_rear = -1;
int queue_size = 0;
// calculates group size
int group_size(int group_id) {
	if(group_id < N_no_of_students % M_no_of_groups)
	{
		return N_no_of_students / M_no_of_groups + 1;
	}
	else
	{
		return N_no_of_students / M_no_of_groups;
	}
}

/*
Functions for queue setup and use
*/
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

/*
Shuffle funciton so teacher can randomly assign groups
*/
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
	if(K_no_of_tutors > M_no_of_groups){
		K_no_of_tutors =  M_no_of_groups;
	}

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

	// Dynamically allocate memory for the group id array
	tutor_status = (int *)malloc(K_no_of_tutors * sizeof(int));
	if (tutor_status == NULL)
	{
		perror("Failed to allocate memory");
		return 1;
	}

	// Initialize the array elements to 0
	for (int i = 0; i < K_no_of_tutors; i++)
	{
		tutor_status[i] = 0;
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
        free(lab_queue); //free lab_queue if this allocation fails
        return 1;
    }

    // Initialize group_to_lab_map to -1 (indicating no lab assigned)
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
	initialize_cond_var(&start_part2_cv);
	initialize_cond_var(&a_student_assigned);
	initialize_cond_var(&student_arrived);
	initialize_cond_var(&id_recieved);
	initialize_cond_var(&lab_room_available);
    initialize_cond_var(&students_can_enter_lab);
    initialize_cond_var(&all_students_entered);
    initialize_cond_var(&students_lab_over);
    initialize_cond_var(&all_students_left_lab);
    initialize_cond_var(&tutor_go_home);
    initialize_cond_var(&tutor_went_home);
	initialize_cond_var(&group_assigned);
	initialize_cond_var(&teacher_waiting_for_available_lab);
	initialize_cond_var(&tutor_ready_for_students);
	
	//setup threads
	threads = malloc((N_no_of_students + K_no_of_tutors + 1) * sizeof(pthread_t)); // total is no of students + tutors + 1 to include teacher
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
	// join all threads
	for (k = 0; k < N_no_of_students + K_no_of_tutors + 1; k++)
	{
		rc = pthread_join(threads[k], NULL);
		if (rc)
		{
			printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
		}
	}

	

	// destroy mutex and condition variable objects
	// pthread_mutex_destroy(&melon_box_mutex);
	// pthread_cond_destroy(&consumer_cond);
// Deallocate allocated memory
    free(threads);
    free(t_ids);
    free(group_lineup);
    free(tutor_status);
    free(lab_queue);
    free(group_to_lab_map);
    free(lab_to_group_map);
    free(lab_room_capacity);

    // Destroy mutexes and condition variables
    pthread_mutex_destroy(&arriving_mutex);
    pthread_mutex_destroy(&assigning_mutex);
    pthread_mutex_destroy(&tutor_status_mutex);
    pthread_mutex_destroy(&lab_room_map_mutex);
    pthread_mutex_destroy(&lab_room_size_capacity);
    pthread_mutex_destroy(&teacher_status_mutex);
    pthread_mutex_destroy(&tutor_left_mutex);
    pthread_mutex_destroy(&start_part2_mutex);
    pthread_mutex_destroy(&queue_mutex);

    pthread_cond_destroy(&a_student_assigned);
    pthread_cond_destroy(&student_arrived);
    pthread_cond_destroy(&id_recieved);
    pthread_cond_destroy(&lab_room_available);
    pthread_cond_destroy(&group_assigned);
    pthread_cond_destroy(&tutor_ready_for_students);
    pthread_cond_destroy(&teacher_waiting_for_available_lab);
    pthread_cond_destroy(&students_can_enter_lab);
    pthread_cond_destroy(&all_students_entered);
    pthread_cond_destroy(&students_lab_over);
    pthread_cond_destroy(&all_students_left_lab);
    pthread_cond_destroy(&tutor_go_home);
    pthread_cond_destroy(&tutor_went_home);
    pthread_cond_destroy(&start_part2_cv);

    return 0;
}

void *teacher_routine(void *arg)
{

//•	Announce teacher is ready. Print “I'm waiting for all students to arrive.”
	printf("Teacher: I'm waiting for all students to arrive.\n");
	//•	Wait for the student threads to be created. Count each signal 
	//from when a student is made. (|no_of_students_arrived < N_no_of_students|)
	pthread_mutex_lock(&arriving_mutex);
	while (no_of_students_arrived < N_no_of_students)
	{

		pthread_cond_wait(&student_arrived, &arriving_mutex);
	}
	pthread_mutex_unlock(&arriving_mutex);

	// all the students have arrived
	printf("Teacher: All students have arrived. I start to assign group ids to students.\n");

	pthread_mutex_lock(&assigning_mutex);

	// wait for all students to be assigned
//•	Generate the group assignments for the students. The assignments are stored 
//in the array |group_lineup|. Index is the student id and the value is their group id. 
//The group is calculated using the formula |group_lineup[i] = i % M_no_of_groups;|.
	// each student group id is the mod of their position (student id) with M
	// 1 1 1 1 1 1 1 , N = 7, M = 3 , n/m = 2 + 1, 3 2. 2; 0 1 2 0 1 2 0
	for (int i = 0; i < N_no_of_students; i++)
	{
		group_lineup[i] = i % M_no_of_groups;

	}

	//•	The teacher shuffles the array to randomize the assignments before sending them out.
	//  group lineup index is the student id and the value is what group the student is in
	shuffle(group_lineup, N_no_of_students);

//•	The teacher sends out the group assignments one by one and 
//verifies that each student got their assignment before proceeding to the next. 
	for (int i = 0; i < N_no_of_students; i++){

		//Teacher: student [id] is in group [id].
		current_student_id = i;
		printf("Teacher: Student %d is in group %d.\n", i, group_lineup[i]);
		pthread_cond_broadcast(&a_student_assigned);
		//pthread_mutex_unlock(&assigning_mutex);
		pthread_cond_wait(&id_recieved, &assigning_mutex);
	}
	pthread_mutex_unlock(&assigning_mutex);


//•	Announce that all students have been assigned
	printf("Teacher: I have assigned all students to a group.\n");

	
 /// part 2

 

// Signal tutors to start part 2
//•	Set flag to start part 2 and wake up tutors. 
pthread_mutex_lock(&start_part2_mutex);
start_part2 = 1;
pthread_cond_broadcast(&start_part2_cv);
pthread_mutex_unlock(&start_part2_mutex);

 int group_id_teacher = 0;
 int lab_id;
while (group_id_teacher < M_no_of_groups){
	printf("Teacher: I’m waiting for lab rooms to become available\n");
	pthread_mutex_lock(&teacher_status_mutex);
	teacher_status = 1; //1 is waiting for lab room to become available
	pthread_cond_broadcast(&teacher_waiting_for_available_lab);
	pthread_mutex_unlock(&teacher_status_mutex);


	//wait for a lab room to become available
	pthread_mutex_lock(&queue_mutex);
	while(is_queue_empty() ){
	pthread_cond_wait(&lab_room_available, &queue_mutex); 
	}
	//•	Pop a lab room from the queue.
	lab_id = dequeue();
	pthread_mutex_unlock(&queue_mutex);



/*•	Assign the |group_id| to the popped |lab_id| by updating |group_to_lab_map| and |lab_to_group_map|. 
This is how the tutors and students keep track of what group is assigned to what lab.  
Two arrays are used to prevent having to search through an array for a value. 
*/	pthread_mutex_lock(&lab_room_map_mutex);
	group_to_lab_map[group_id_teacher] = lab_id;
	lab_to_group_map[lab_id] = group_id_teacher;
	pthread_cond_broadcast(&group_assigned); 	//signal tutor to that group is assigned
	pthread_mutex_unlock(&lab_room_map_mutex);

	//wait for tutor to be ready to get students
	pthread_mutex_lock(&tutor_status_mutex);
	while(tutor_status[lab_id] != 2){
		pthread_cond_wait(&tutor_ready_for_students, &tutor_status_mutex);
	}
	pthread_mutex_unlock(&tutor_status_mutex);
	//now tutor is ready to get students and teacher can notify students to enter 	
	printf("Teacher: The lab %d is now available. Students in group %d can enter the room and start your lab exercise.\n", lab_id, group_id_teacher);
	pthread_mutex_lock(&lab_room_map_mutex);
	pthread_cond_broadcast(&students_can_enter_lab); //Signal that students in the |group_id| to enter the lab.
	pthread_mutex_unlock(&lab_room_map_mutex);
//•	Wait for all students to enter the lab (|pthread_cond_wait(&all_students_entered, &lab_room_size_capacity);|).
	pthread_mutex_lock(&lab_room_size_capacity);
	while(lab_room_capacity[lab_id] < group_size(group_id_teacher)){
		pthread_cond_wait(&all_students_entered, &lab_room_size_capacity);
	}	
	pthread_mutex_unlock(&lab_room_size_capacity);

//•	Move to the next |group_id|.
	group_id_teacher++;

}

//signal tutor to exit
//•	Set |teacher_status = 3| (tutors can go home).
printf("Teacher: There are no students waiting. Tutor %d, you can go home now\n", lab_id);
pthread_mutex_lock(&teacher_status_mutex);
teacher_status = 3;
pthread_mutex_unlock(&teacher_status_mutex);

pthread_mutex_lock(&lab_room_map_mutex);
//	Signal all tutors that it's time to exit.
pthread_cond_broadcast(&group_assigned);
pthread_mutex_unlock(&lab_room_map_mutex);

//wait for tutor to exit 
pthread_mutex_lock(&tutor_left_mutex);

//•	Wait for confirmation from all tutors (|tutor_count_left| matches total tutors).
while(tutor_count_left < K_no_of_tutors){
	pthread_cond_wait(&tutor_went_home, &tutor_left_mutex);

} 

//teacher exits
printf("Teacher: All students and tutors have left. I can now go home.\n");
	pthread_exit(EXIT_SUCCESS);
}

void *student_routine(void *arg)
{
	int *myid;
	int group_id;
	int lab_id;
	myid = (int *)arg;

	//•	Announce each student’s creation and increment counter so teacher knows when all students arrive. 
	printf("Student %d: I have arrived and wait for being assigned to a group.\n", *myid);
	pthread_mutex_lock(&arriving_mutex);
	no_of_students_arrived++;
	pthread_cond_signal(&student_arrived);

	pthread_mutex_unlock(&arriving_mutex);

//•	Wait for the teacher to assign a lab to the student’s group.
//•	The correct student wakes up as the teacher is incrementing 
//the current id one by one so only one thread would wake up at a time. |current_student_id < *myid|
	pthread_mutex_lock(&assigning_mutex);
	while (current_student_id < *myid) 
	{
		pthread_cond_wait(&a_student_assigned, &assigning_mutex);
	}
	//•	Check |group_lineup| to get the random group assignment.
	//•	Announce group
	printf("Student %d:OK, I'm in group %d and waiting for my turn to enter a lab room.\n", *myid, group_lineup[*myid]);
	group_id = group_lineup[*myid];
	pthread_cond_signal(&id_recieved);
	pthread_mutex_unlock(&assigning_mutex);

  	//part 2

	//•	Check |group_to_lab_map| to find the assigned |lab_id|.
	pthread_mutex_lock(&lab_room_map_mutex);
	//•	If the |lab_id| is |-1|then wait until a |students_can_enter_lab|signal to recheck if they student’s has been assigned a lab. 
	lab_id = group_to_lab_map[group_id];
	while(lab_id == -1){
		pthread_cond_wait(&students_can_enter_lab, &lab_room_map_mutex);
		lab_id = group_to_lab_map[group_id];
	}
	pthread_mutex_unlock(&lab_room_map_mutex);
	//•	Announce student is entering lab
	printf("Student %d in group %d: My group is called. I will enter the lab %d room now.\n", *myid, group_id, lab_id);

	//increase capacity of lab room array 
	pthread_mutex_lock(&lab_room_size_capacity);
	lab_room_capacity[lab_id]++;
	//•	If the |lab_room_capacity| is equal to the |group_size(group_id)| then the last student entered. The last student broadcasts that | all_students_entered|
	if (lab_room_capacity[lab_id] == group_size(group_id)){
		//printf("Student %d in group %d: I am the last student to enter the lab room %d. I will signal the tutor to start the lab exercise.\n", *myid, group_id, lab_id);
//signal tutor after all students in group are in lab
		pthread_cond_broadcast(&all_students_entered);
	}
	pthread_mutex_unlock(&lab_room_size_capacity);
	// tutor should now signal to start lab exercise


//wait for tutor to call the end of lab exercise
	pthread_mutex_lock(&lab_room_size_capacity);
	while(lab_room_capacity[lab_id] > 0){ //tutor will switch capacity to negative of the max capacity
		pthread_cond_wait(&students_lab_over, &lab_room_size_capacity);
	}
	//students increment lab room capacity back to 0 as they leave
	lab_room_capacity[lab_id]++;


	printf("Student %d in group %d: Thanks Tutor %d. Bye!\n", *myid, group_id, lab_id);
	// lab capacity now reset to 0

//the last student signal tutor the room is vacated
	if (lab_room_capacity[lab_id] == 0){
		printf("Student %d in group %d: I am the last student to leave the lab room %d. I will signal the tutor to leave.\n", *myid, group_id, lab_id);
		
		pthread_mutex_lock(&lab_room_map_mutex);
		//•	The last student signals the tutor that all teachers left the lab by setting the group id to -2. 
		lab_to_group_map[lab_id] = -2;
		pthread_mutex_unlock(&lab_room_map_mutex);
		pthread_cond_broadcast(&all_students_left_lab);
	}
	pthread_mutex_unlock(&lab_room_size_capacity);

	pthread_exit(EXIT_SUCCESS);  

}

void * tutor_routine(void *arg){

	 //Wait for the teacher to start part 2. |start_part2| is set to 1 by the teacher.
    pthread_mutex_lock(&start_part2_mutex);
    while (!start_part2) {
        pthread_cond_wait(&start_part2_cv, &start_part2_mutex);
    }
    pthread_mutex_unlock(&start_part2_mutex);
	int myid = *(int *)arg;

	int gid;

	while(1){
	// mark that tutor got created
	pthread_mutex_lock(&tutor_status_mutex);
	tutor_status[*(int *) arg] = 0; 
	pthread_mutex_unlock(&tutor_status_mutex);
	
	//Wait for the teacher to signal that a lab is needed (|teacher_status == 1|). And make sure teacher isn’t telling tutors to exit (|teacher_status == 3|).
	pthread_mutex_lock(&teacher_status_mutex);
	while(teacher_status != 1 && teacher_status != 3){
		pthread_cond_wait(&teacher_waiting_for_available_lab, &teacher_status_mutex);
	}
	pthread_mutex_unlock(&teacher_status_mutex);

	//Add the tutor's |lab_id| to the available lab queue
	pthread_mutex_lock(&queue_mutex);
	enqueue(*(int *)arg);
	pthread_cond_broadcast(&lab_room_available);
	pthread_mutex_unlock(&queue_mutex);
	
	//Print out that the lab room is available.
	printf("Tutor %d: The lab room %d is vacated and ready for one group\n", *(int *)arg, *(int *)arg);
	pthread_mutex_lock(&lab_room_map_mutex);

	//Wait for a group to be assigned to a tutors lab. |lab_to_group_map[myid] == -1| when there is no assignment
	while (lab_to_group_map[myid] == -1 ){
		//unused tutors are waiting here

		pthread_cond_wait(&group_assigned, &lab_room_map_mutex);
		//check if teacher is trying to exit. This is for tutors that never get a lab. 
	pthread_mutex_lock(&teacher_status_mutex);
	if (teacher_status == 3) {
            printf("Tutor %d: Thanks Teacher. Bye!\n", myid);
            pthread_mutex_unlock(&teacher_status_mutex);

            pthread_mutex_lock(&tutor_left_mutex);
            tutor_count_left++;
            pthread_cond_broadcast(&tutor_went_home);
            pthread_mutex_unlock(&tutor_left_mutex);

            pthread_exit(EXIT_SUCCESS);
	}
            pthread_mutex_unlock(&teacher_status_mutex);
	}
	pthread_mutex_unlock(&lab_room_map_mutex);

//Once assigned,  the tutor prepares to receive students (|tutor_status = 2|).
	pthread_mutex_lock(&tutor_status_mutex);
	tutor_status[*(int *) arg] = 2; //tutor is ready for students
	pthread_cond_broadcast(&tutor_ready_for_students);
	pthread_mutex_unlock(&tutor_status_mutex);


	//tutor reads out the assigned group
	pthread_mutex_lock(&lab_room_map_mutex);
	gid = lab_to_group_map[myid];
	pthread_mutex_unlock(&lab_room_map_mutex);

	//Wait for all students to enter the lab (|all_students_entered|).
	pthread_mutex_lock(&lab_room_size_capacity);
	while(lab_room_capacity[myid] < group_size(gid)){
		//printf("Tutor %d: I'm waiting for all students in group %d to enter the lab room %d with group size %d.\n", myid, gid, myid, group_size(gid));
		pthread_cond_wait(&all_students_entered, &lab_room_size_capacity);
	}	
	pthread_mutex_unlock(&lab_room_size_capacity);
	
	
	printf("Tutor %d: All students in group %d have entered the room. You can start your exercise now.\n", *(int *)arg, gid);
	//Conduct the lab exercise (random time between |T/2| and |T|).
	int ex_time = (rand() % (T_time_limit - T_time_limit/2 + 1)) + T_time_limit/2;
	sleep(ex_time);
	pthread_mutex_lock(&lab_room_size_capacity);
	lab_room_capacity[*(int *)arg] = -group_size(gid); //modify lab room capacity to signal students.
	printf("Tutor %d: Students in group %d have completed the lab exercise in %d units of time. You may leave this room now.\n",myid, gid, ex_time);
	//Signal students to start leaving (|students_lab_over|).
	pthread_cond_broadcast(&students_lab_over); //students go increment lab room capacity array to 0
	pthread_mutex_unlock(&lab_room_size_capacity);

//Wait for all students to exit (|lab_to_group_map[lab_id] = -2|).
	pthread_mutex_lock(&lab_room_map_mutex); //when there are more tutors than groups this lock never happens
	while (lab_to_group_map[myid] != -2){
		printf("Tutor %d: I'm waiting for the students to leave the lab room %d.\n", myid, myid);
		pthread_cond_wait(&all_students_left_lab, &lab_room_map_mutex);
		//printf("Tutor %d: I got told all students left\n", myid);
		
	}
			lab_to_group_map[myid] = -1;
	pthread_mutex_unlock(&lab_room_map_mutex);

//Check if teacher is exiting for tutors that just finished a lab after the teacher already declared exiting. 
	pthread_mutex_lock(&teacher_status_mutex);
	if (teacher_status == 3) {
            printf("Tutor %d: Thanks Teacher. Bye!\n", myid);
            pthread_mutex_unlock(&teacher_status_mutex);

            pthread_mutex_lock(&tutor_left_mutex);
            tutor_count_left++;
            pthread_cond_broadcast(&tutor_went_home);
            pthread_mutex_unlock(&tutor_left_mutex);

            pthread_exit(EXIT_SUCCESS);
	}
            pthread_mutex_unlock(&teacher_status_mutex);


	}

	return NULL;
}
