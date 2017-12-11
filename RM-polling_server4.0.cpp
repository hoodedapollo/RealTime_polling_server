// Three periodsc threads with rate monotonic, two aperiodsc threads 
/***********************************************
 * compile with g++ -lpthread RMaperiodsci.cpo
 ************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>


#define NTASKS 5
#define NPERIODICTASKS 4
#define NAPERIODICTASKS 1 

#define BUFFER_SIZE 1000 //  over sized in order to never get a segmentation fault 
#define TASK_4 1
#define TASK_5 2
#define CAPACITY 100000 // low enough to have still task in the queue when server polling 
                        // has non capacity left
#define ARRIVED   0
#define STOPPED   1
#define COMPLETED 2

// application specific code
void polling_server_code( );
void task1_code( );
void task2_code( );
void task3_code( );
void task4_code( );
//void task5_code( );

// thread functions 
void *polling_server( void *);
void *task1( void *);
void *task2( void *);
void *task3( void *);
void *task4( void *);
// initialization of mutexes and conditions
 
pthread_mutex_t mutex_task_4 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_stop_task_4 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_toggle = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_printf = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_task_4 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_stop_task_4 = PTHREAD_COND_INITIALIZER;

//int q = 0; // polling server queue index
//int j = 0; // task 1,2,3 queue index
//int queue[BUFFER_SIZE] = {0};  // initialize the buffer to zero
//long int C_task_4, C_task_5;   // computational times of aperiodic task 4 and  5

int toggle;
long int periods[NTASKS];
struct timeval next_arrival_time[NTASKS];
long int WCET[NTASKS];
long int WorstExTime[NTASKS] = {0};
int missed_deadlines[NTASKS];
pthread_attr_t attributes[NTASKS];
struct sched_param parameters[NTASKS];
pthread_t thread_id[NTASKS];

struct sched_param priomax;
struct sched_param priomin;
int main()
{
        periods[0]= 100000000; 
        periods[1]= 200000000; 
        periods[2]= 300000000;
        periods[3]= 400000000;

        priomax.sched_priority=sched_get_priority_max(SCHED_FIFO);
        priomin.sched_priority=sched_get_priority_min(SCHED_FIFO);

// If you are superuser set the priority of the current thread (the thread main) as the max possibile.
// This is done in order to measure the computational time of the thread code. Since it's running with
// the highet priority it cannot preempted so the difference between the ending time istant and the
// beginnig time istant is the computational time of the thread code  

        if (getuid() == 0)                 
        {
                pthread_setschedparam(pthread_self(),SCHED_FIFO,&priomax);  
        }                                                                   

         
        //  Execute all tasks, one at a time and compute the difference between ending time istant and beginng 
        //  time istant. Use the computed values to update the worst case execution time of each task.


        for (int i =1; i < NTASKS; i++)
        {
                for(int n = 0; n < 10; n++)
                {
                        struct timeval timeval1;
                        struct timezone timezone1;
                        struct timeval timeval2;
                        struct timezone timezone2;
                        gettimeofday(&timeval1, &timezone1);

                        // periodic tasks 

                        if (i==1)
                                task1_code();
                        if (i==2)
                                task2_code();
                        if (i==3)
                                task3_code();

                        // aperiodic tasks

                        if (i==4)
                                task4_code();
//                        if (i==5)
//                                task5_code();

                        gettimeofday(&timeval2, &timezone2);

                        WCET[i]= 1000*((timeval2.tv_sec - timeval1.tv_sec)*1000000
                                        +(timeval2.tv_usec-timeval1.tv_usec));

                        printf("WCET[%d] %d-th iteration = %ld\n", i, n, WCET[i]);fflush(stdout);

                        if (WCET[i] > WorstExTime[i] )
                        {
                                WorstExTime[i] = WCET[i];
                        }
                }
        }

        for (int i = 0; i < NTASKS; i++)
        {
                if (i==0)
                {
                        WorstExTime[i] = CAPACITY;
                        printf("\nPolling server Worst Case Execution Time = %ld \n", WorstExTime[i]); fflush(stdout);
                }
                else
                {
                        printf("\nTask %d Worst Case Execution Time = %ld \n", i, WorstExTime[i]); fflush(stdout);
                }
        }
//        C_task_4 = WorstExTime[4];  // make the worst computational times of task 4 and task5 available to the server polling
//        C_task_5 = WorstExTime[5];

        double Ulub = NPERIODICTASKS*(pow(2.0,(1.0/NPERIODICTASKS)) -1);  // RM U_lub

        double U = 0;
        for (int i = 0; i < NPERIODICTASKS; i++)
                U+= ((double)WCET[i])/((double)periods[i]);  // Actual U

        if (U > Ulub) 
        {
                printf("\n U=%lf Ulub=%lf Not schedulable\n", U, Ulub); fflush(stdout);
                return(-1);
        }
        printf("\n U=%lf Ulub=%lf Schedulable\n", U, Ulub);fflush(stdout);

// reinitialize the queue after the execution of task codes to calculate worst execution times 

//        pthread_mutex_lock(&mutex_queue);
//        for(int i = 0; i < BUFFER_SIZE; i++)
//        {
//                queue[i] = 0;
//        }
//        pthread_mutex_unlock(&mutex_queue);

// wait some time to show worst execution times, U and Ulub berfore starting the execution of tasks
        sleep(5);

// give the thread main the lowest priority so it's never going to preempt the other tasks
        if (getuid() == 0)
        {
                pthread_setschedparam(pthread_self(),SCHED_FIFO,&priomin);
        }

// set the threads attributes and store them in the correspondent struct        
//
                parameters[0].sched_priority = priomax.sched_priority;
                parameters[1].sched_priority = priomax.sched_priority - 2;
                parameters[2].sched_priority = priomax.sched_priority - 3;
                parameters[3].sched_priority = priomax.sched_priority - 4;


// periodic threads                
        for (int i =0; i < NPERIODICTASKS; i++)
        {
                pthread_attr_init(&(attributes[i]));
                pthread_attr_setinheritsched(&(attributes[i]), PTHREAD_EXPLICIT_SCHED);

                pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);

                pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
        }

// aperiodic threads
//
        parameters[4].sched_priority = priomax.sched_priority - 1;

        pthread_attr_init(&(attributes[4]));
        pthread_attr_setinheritsched(&(attributes[4]), PTHREAD_EXPLICIT_SCHED);

        pthread_attr_setschedpolicy(&(attributes[4]), SCHED_FIFO);
        
        pthread_attr_setschedparam(&(attributes[4]), &(parameters[4]));

        int iret[NTASKS];
        struct timeval ora;
        struct timezone zona;
        gettimeofday(&ora, &zona);

// calculate the next arrival time and initialize the missed deadlines to zero        
        for (int i = 0; i < NPERIODICTASKS; i++)
        {
                long int periods_micro = periods[i]/1000;
                next_arrival_time[i].tv_sec = ora.tv_sec + periods_micro/1000000;
                next_arrival_time[i].tv_usec = ora.tv_usec + periods_micro%1000000;
                missed_deadlines[i] = 0;
        }

// thread creation
        iret[0] = pthread_create( &(thread_id[0]), &(attributes[0]), polling_server, NULL);
        iret[1] = pthread_create( &(thread_id[1]), &(attributes[1]), task1, NULL);
        iret[2] = pthread_create( &(thread_id[2]), &(attributes[2]), task2, NULL);
        iret[3] = pthread_create( &(thread_id[3]), &(attributes[3]), task3, NULL);
        iret[4] = pthread_create( &(thread_id[4]), &(attributes[4]), task4, NULL);

// join all the created threads so that the thread main waits for the threads to exit before exiting itslef
        pthread_join( thread_id[0], NULL);
        pthread_join( thread_id[1], NULL);
        pthread_join( thread_id[2], NULL);
        pthread_join( thread_id[3], NULL);

        exit(0);
}


void polling_server_code()
{
        struct timespec Capacity;
        Capacity.tv_sec = 100;
        Capacity.tv_nsec = CAPACITY;

        pthread_mutex_lock(&mutex_printf);
        printf("    PS START\n    |\n"); fflush(stdout);
        pthread_mutex_unlock(&mutex_printf);

// set the priority of the aperiodic task 4 as the second maximum possible (second only to the polling
// server so it can not be preempted by the periodic  tasks, but only by the polling server.
        if (getuid() == 0)
        {
                pthread_setschedprio(thread_id[4], priomax.sched_priority - 1);
        }
// if the aperiod thread is just arrived then signal the condition variable to let it start
// from the beginnig         
        if (toggle == ARRIVED)
        {
                pthread_mutex_lock(&mutex_task_4);
                pthread_cond_signal(&cond_task_4);
                pthread_mutex_unlock(&mutex_task_4);
        }

// make the server wait so the aperiodic thread can run wheter it is just arrived or it was stopped
// during its execution by the previous execution of the polling server       
//
// PROBLEM ?????????????????????? pthread_cond_timedwait is not waiting WTF???
// wait time must be specified in absolute time not relative time so you must add the actual time wiith
// get time of day
        pthread_mutex_lock(&mutex_stop_task_4);
        int timedwait_ret = pthread_cond_timedwait(&cond_stop_task_4,&mutex_stop_task_4,&Capacity);
        perror("timedwait");
        printf("timedwait return value %d\n", timedwait_ret); fflush(stdout);
        
// debug line to test if the polling server realse the CPU while waiting 
// pthread_cond_wait(&cond_stop_task_4,&mutex_stop_task_4);
                                                            
        pthread_mutex_unlock(&mutex_stop_task_4);

// if the waiting time expired before the aperiodic task was completed set the toggle accordingly
// so the next time the polling server runs it will not make the aperiodic thread start from the beginning        
        if ( toggle != COMPLETED)
        {
                pthread_mutex_lock(&mutex_toggle);
                toggle = STOPPED;
                pthread_mutex_unlock(&mutex_toggle);
        }

// set the priority of the aperiodic task as the lowest possible so it cannot preempt the periodic tasks 
// ( actually in this way if none of the periodic tasks needs the processor it still can be scheduled in
// a background fashion. For this reason we are looking for something that actually prevent the thread
// from running unless we give it a priority. we are looking for a command that makes it unschedulable 
// until we assingn it its priority as the second maximum one.
        if (getuid() == 0)
        {
                pthread_setschedprio(thread_id[4], priomin.sched_priority);
        }

        pthread_mutex_lock(&mutex_printf);
        printf("    |\n    PS END\n"); fflush(stdout);
        pthread_mutex_unlock(&mutex_printf);
}        


void *polling_server( void *ptr)
{
        int i=0;
        struct timespec waittime;
        waittime.tv_sec=0; /* seconds */
        waittime.tv_nsec = periods[0]; /* nanoseconds */

        /* forcing the thread to run on CPU 0 */
        cpu_set_t cset;
        CPU_ZERO( &cset );
        CPU_SET( 0, &cset);

        pthread_setaffinity_np( pthread_self(), sizeof(cpu_set_t), &cset);


        while(1) 
        {
                polling_server_code();
                struct timeval ora;
                struct timezone zona;
                gettimeofday(&ora, &zona);
                long int timetowait= 1000*((next_arrival_time[0].tv_sec - ora.tv_sec)*1000000
                                +(next_arrival_time[0].tv_usec-ora.tv_usec));
                if (timetowait <0)
                        missed_deadlines[0]++;
                waittime.tv_sec = timetowait/1000000000;
                waittime.tv_nsec = timetowait%1000000000;
                nanosleep(&waittime, NULL);
                long int periods_micro=periods[0]/1000;
                next_arrival_time[0].tv_sec = next_arrival_time[0].tv_sec +
                        periods_micro/1000000;
                next_arrival_time[0].tv_usec = next_arrival_time[0].tv_usec +
                        periods_micro%1000000;
        }
}

void task1_code()
{

        // the task does something...

        double uno;
        printf("            1 START\n            |\n"); fflush(stdout);
        for (int i = 0; i < 10; i++)
        {
                for (int j = 0; j < 1000; j++)
                        uno = rand()%10;
        }

        if (uno == 0)
        {
                  printf("            |--> 4a arrival\n");fflush(stdout);
                  pthread_mutex_lock(&mutex_toggle);
                  toggle = ARRIVED;
                  pthread_mutex_unlock(&mutex_toggle);
        }
         printf("            |\n            1 END\n\n"); fflush(stdout);
}

void *task1( void *ptr)
{
        int i=0;
        struct timespec waittime;
        waittime.tv_sec=0; /* seconds set to zero since task period is less than zero*/
        waittime.tv_nsec = periods[1]; /* nanoseconds equal to the period of the task defined in the main thread*/

        /* forcing the thread to run on CPU 0 */
        cpu_set_t cset;
        CPU_ZERO( &cset );
        CPU_SET( 0, &cset);

        pthread_setaffinity_np( pthread_self(), sizeof(cpu_set_t), &cset);

        while(1)
        {

                task1_code(); // task do something

                struct timeval ora;
                struct timezone zona;
                gettimeofday(&ora, &zona);

                long int timetowait= 1000*((next_arrival_time[1].tv_sec - ora.tv_sec)*1000000
                                +(next_arrival_time[1].tv_usec-ora.tv_usec)); //  time interval between now and the next arrival time 
                if (timetowait < 0)
                        missed_deadlines[1]++;

                waittime.tv_sec = timetowait/1000000000;
                waittime.tv_nsec = timetowait%1000000000;

                nanosleep(&waittime, NULL);  // sleep till next arrival time

                // computing next arrival time
                long int periods_micro=periods[1]/1000;
                next_arrival_time[1].tv_sec = next_arrival_time[1].tv_sec +
                        periods_micro/1000000;
                next_arrival_time[1].tv_usec = next_arrival_time[1].tv_usec +
                        periods_micro%1000000;

        }
}
void task2_code()
{

        double uno;
        printf("                    2 START\n                    |\n"); fflush(stdout);
        for (int i = 0; i < 10; i++)
        {
                for (int j = 0; j < 1000; j++)
                        uno = rand()%2;
        }
        printf("                    |\n                    2 END\n\n"); fflush(stdout);
}


void *task2( void *ptr )
{
        int i=0;
        struct timespec waittime;
        waittime.tv_sec=0; /* seconds */
        waittime.tv_nsec = periods[2]; /* nanoseconds */

        /* forcing the thread to run on CPU 0 */
        cpu_set_t cset;
        CPU_ZERO( &cset );
        CPU_SET( 0, &cset);

        pthread_setaffinity_np( pthread_self(), sizeof(cpu_set_t), &cset);

        while(1)
        {

                task2_code();
                struct timeval ora;
                struct timezone zona;
                gettimeofday(&ora, &zona);
                long int timetowait= 1000*((next_arrival_time[2].tv_sec -
                                        ora.tv_sec)*1000000 +(next_arrival_time[2].tv_usec-ora.tv_usec));
                if (timetowait <0)
                        missed_deadlines[2]++;
                waittime.tv_sec = timetowait/1000000000;
                waittime.tv_nsec = timetowait%1000000000;
                nanosleep(&waittime, NULL);
                long int periods_micro=periods[2]/1000;
                next_arrival_time[2].tv_sec = next_arrival_time[2].tv_sec + periods_micro/1000000;
                next_arrival_time[2].tv_usec = next_arrival_time[2].tv_usec +
                        periods_micro%1000000;

        }
}

void task3_code()
{

        double uno;
        printf("                            3 START\n                            |\n"); fflush(stdout);
        for (int i = 0; i < 10; i++)
        {
                for (int j = 0; j < 1000; j++)
                        uno = rand()%2;
        }
        printf("                            |\n                            3 END\n\n"); fflush(stdout);
}
void *task3( void *ptr)
{
        int i=0;
        struct timespec waittime;
        waittime.tv_sec=0; /* seconds */
        waittime.tv_nsec = periods[3]; /* nanoseconds */

        /* forcing the thread to run on CPU 0 */
        cpu_set_t cset;
        CPU_ZERO( &cset );
        CPU_SET( 0, &cset);

        pthread_setaffinity_np( pthread_self(), sizeof(cpu_set_t), &cset);


        while(1)
        {

                task3_code();
                struct timeval ora;
                struct timezone zona;
                gettimeofday(&ora, &zona);
                long int timetowait= 1000*((next_arrival_time[3].tv_sec - ora.tv_sec)*1000000
                                +(next_arrival_time[3].tv_usec-ora.tv_usec));
                if (timetowait <0)
                        missed_deadlines[3]++;
                waittime.tv_sec = timetowait/1000000000;
                waittime.tv_nsec = timetowait%1000000000;
                nanosleep(&waittime, NULL);
                long int periods_micro=periods[3]/1000;
                next_arrival_time[3].tv_sec = next_arrival_time[3].tv_sec +
                        periods_micro/1000000;
                next_arrival_time[3].tv_usec = next_arrival_time[3].tv_usec +
                        periods_micro%1000000;

        }
}


void task4_code()
{
        pthread_mutex_lock(&mutex_printf);
        printf("    |--> 4a START\n"); fflush(stdout);
        pthread_mutex_unlock(&mutex_printf);

        for (int i = 0; i < 1000; i++)
        {
                for (int j = 0; j < 1000; j++)
                        double uno = rand()*rand();
        }
        pthread_mutex_lock(&mutex_printf);
        printf("    |--> 4a END\n"); fflush(stdout);
        pthread_mutex_unlock(&mutex_printf);
}

void *task4( void *ptr)
{

        /* forcing the thread to run on CPU 0 */
        cpu_set_t cset;
        CPU_ZERO( &cset );
        CPU_SET( 0, &cset);

        pthread_setaffinity_np( pthread_self(), sizeof(cpu_set_t), &cset);


        while(1)
        {

// wait for the polling server to signal the task 4 to start 
                pthread_mutex_lock(&mutex_task_4);
                pthread_cond_wait(&cond_task_4, &mutex_task_4);
                pthread_mutex_unlock(&mutex_task_4);

                task4_code();
// after the task has finished set the toggle accordingly
                pthread_mutex_lock(&mutex_toggle);
                toggle = COMPLETED;
                pthread_mutex_unlock(&mutex_toggle);

// after the task has finished its job signal the polling server which is waiting 
                pthread_mutex_lock(&mutex_stop_task_4);
                pthread_cond_signal(&cond_stop_task_4);
                pthread_mutex_unlock(&mutex_stop_task_4);
        }
}

