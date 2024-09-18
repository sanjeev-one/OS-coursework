
 ### Global Variables:

- Lab Queue: Manages available labs (`lab_queue`).
- lab_to_group_map: Maps lab ID (index) to group ID (value). When a group leaves, it is reset to `-2` (lab empty).
- group_to_lab_map: Maps group ID (index) to lab ID (value).
- tutor_status: Manages tutor states:
  - `-1` (waiting for teacher),
  - `0` (in the lab queue),
  - `1` (assigned a group),
  - `2` (ready for students),
  - `3` (exiting).
- teacher_status: 
  - `0` (doing work),
  - `1` (waiting for a tutor to assign lab),
  - `2` (group assigned),
  - `3` (tutors can go home).

---

 ### Teacher Routine:

1. Part 1:
   - `group_id = 0`
   - Loop through all groups (`group_id < total_groups`):
     - Set `teacher_status = 1` (waiting for a lab).
     - Wait for a lab room (while the lab queue is empty).
     - Pop a lab room from the queue.
     - Set `tutor_status[lab_id] = 1` (indicating tutor has been assigned to the lab).
     - Set `teacher_status = 0`.
     - Assign the `group_id` to the popped `lab_id` by updating `group_to_lab_map` and `lab_to_group_map`.
     - Signal the tutor that a group has been assigned.
     - Signal students in the `group_id` to enter the lab.
     - Wait for all students to enter the lab (`students_can_enter_lab`).
     - Once the lab exercise completes (students signal lab is vacated), reset the `lab_to_group_map[lab_id] = -2`.
     - Move to the next `group_id`.

2. After All Groups Are Done:
   - Set `teacher_status = 3` (tutors can go home).
   - Signal all tutors that it's time to exit.
   - Wait for confirmation from all tutors (`tutor_count_left` matches total tutors).
   - Exit.

---

 ### Tutor Routine:

1. Initialization:
   - Wait for the teacher to start part 2.
   - Main Loop:
     - Add the tutor's `lab_id` to the available lab queue and set `tutor_status = 0`.
     - Wait for the teacher to signal that a lab is needed (`teacher_status == 1`).
     - Once assigned (`tutor_status = 1`), the tutor prepares to receive students (`tutor_status = 2`).
     - Wait for all students to enter the lab (`all_students_entered`).
     - Conduct the lab exercise (random time between `T/2` and `T`).
     - Signal students to start leaving (`students_lab_over`).
     - Wait for all students to exit (`lab_to_group_map[lab_id] = -2`).
     - Reset `lab_to_group_map[lab_id] = -1`.
     - Return to the top of the loop to wait for the next group (if the teacher hasn't signaled `teacher_status = 3`).
   
2. Exit:
   - If `teacher_status = 3`, increment `tutor_count_left`, signal the teacher (`tutor_went_home`), and exit.

---

 ### Student Routine:

1. Part 1:
   - Wait for the teacher to assign a lab to the student’s group.
   - Check `group_to_lab_map` to find the assigned `lab_id`.
   - Enter the lab (`students_can_enter_lab` signal).
   - Increment the `lab_room_capacity` and signal the tutor if the student is the last to enter.
   
2. Lab Exercise:
   - Wait for the tutor to signal the end of the lab session (`students_lab_over`).
   - Start leaving the lab. Each student decrements the `lab_room_capacity`.
   - The last student signals the tutor that the lab is vacated (`all_students_left_lab`) and sets `lab_to_group_map[lab_id] = -2`.
   - Exit.

---

 #### Fixes for Edge Cases (More Tutors than Groups):

1. Tutor Exit Condition:
   - If a tutor has no more groups to handle (`lab_to_group_map[lab_id] = -1` and `teacher_status = 3`), the tutor should exit.
   
2. Reset Group-to-Lab Map:
   - After students leave the lab, `lab_to_group_map[lab_id]` is set to `-2`. The tutor must wait for the teacher to reassign the lab with a new group, preventing the tutor from reusing the same lab until reassigned.

---

 ## Summary of Workflow:

- Teacher waits for a lab, assigns it to a group, and signals both the tutor and students.
- Tutor prepares for students, conducts the lab, and waits for the lab to be vacated before resetting the lab and waiting for the next group.
- Students wait for the teacher's signal to enter the lab, complete the lab, and vacate the room. The last student in each group signals the tutor that the room is vacated.

This approach ensures proper synchronization between teacher, tutors, and students, handling scenarios with more tutors than groups and ensuring tutors exit properly after their work is done.
