teacher doesnt reasssign gid if its -2 after all the studetns leave, eg the studetns all leave, labtogroupmap,=-2. then tutor is waitng for studetns to show up ina group id of -2. need tutor to wait for new teacher reassignemtn. 

## Variables
available lab queue
lab id to group id
group id to lab id
tutor status = -1 - init waiting for teacher, 0 in queue, 1 popped, 2 ready to get studetns, 3 exit

teacher flag = 1 wating for tutor, 0 doing stuff, 2 group assigned, 3 go home
## Teacher:
part 1
group id = 0
loop group id < groups ----one gfroup at a time == signal?
    im waiting for a lab room to be available - while queu empty; teacher flag =1 
     wait for tutor signal
    popopopo
    signal tutor that room popped, tutor statuss =1 (0 tutor in queue) teacher status = 0

    tutor say - ready to get group students = 2?
    
    
    

    assign groupid / labid to map arrays
    tell students to join the labid -- students tell tutor to start exp/    tutor tells students to exit, students exit

after loop
there are no students, - set tutor status = 3
tutor can go home
tutor says i went home
teacher goes home


## Tutor:
status = -1
add lab to queue
wait for teacher to be waiting for lab - teacher: what lab ready 
check teacher status if its 1
(status = 0)
[teacher pop po ppo] [status = 1]
[tutor status 1 - popped]
tutor see ok i popped - 1
send back tutor =2 (ready to get students) - tutor_ready_for_students

wait for students to enter group - all students have entered condition
"all students have entered you can start the lab" 
--->count lab time
=---> flip student count
signal students to leave 
student in group id have completed the lab in X units of time
wait till there are no more students
go back --> to top
## Student:


part 1

wait for teacher to tell students in correct group to enter lab
check student to group , group to lab to find correct lab to increment 

student id: my gruop id is called i will enter the lab
-- loop through and increase capacity array - last student sets labid_gid map to -1 so tutor wont go again

wait for tutor to tell to start leaving lab
loop through and increase capacity array to negative value
signal tutor that lab is vacated
exit


