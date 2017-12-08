/*************************************************************************************************
*
*  Four periodc threads scheduled with rate monotonic, two aperiodc tasks (no threads).
*  The polling server is the periodic task with the highest priority among all periodic
*  tasks, it is in charge of the scheduling of the aperiodic tasks in a FIFO fashion.
*  The polling server schedules an aperiodic task as long as it has enough capacity left
*  to run it completly, otherwise it waits till its next arrival time to schedule it.
*  To estimate the worst execution time of each task, the thread main run its code 10 times
*  and take the maximum computational time.
*  A circular queue is implemented in order to manage multiple arrival of aperiodic a tasks.
*
**************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>


#define NTASKS 6
#define NPERIODICTASKS 4
#define NAPERIODICTASKS 0 

#define BUFFER_SIZE 1000 //  over sized in order to never get a segmentation fault - to be optimized
#define TASK_4 1
#define TASK_5 2
#define CAPACITY 800000 // low enough to have still task in the queue when server polling 
                         // has non capacity left

// application specific code
void polling_server_code( );
void task1_code( );
void task2_code( );
void task3_code( );
void task4_code( );
void task5_code( );

// thread functions 
void *polling_server( void *);
void *task1( void *);
void *task2( void *);
void *task3( void *);

// initialization of mutexes and conditions
pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_q = PTHREAD_MUTEX_INITIALIZER;


int q = 0; // polling server queue index
int h = 0; // task 1,2,3 queue index
int queue[BUFFER_SIZE] = {0};  // initialize the buffer to zero
long int C_task_4, C_task_5;   // computational times of aperiodic task 4 and  5
long int Capacity = CAPACITY;  // server polling capacity

long int periods[NTASKS];
struct timeval next_arrival_time[NTASKS];
long int WCET[NTASKS];
long int WorstExTime[NTASKS] = {0};
int missed_deadlines[NTASKS];
pthread_attr_t attributes[NTASKS];
struct sched_param parameters[NTASKS];
pthread_t thread_id[NTASKS];

int main()
{
        periods[0]= 100000000; 
        periods[1]= 110000000; 
        periods[2]= 120000000;
        periods[3]= 130000000;

        struct sched_param priomax;
        priomax.sched_priority=sched_get_priority_max(SCHED_FIFO);
        struct sched_param priomin;
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
                        if (i==5)
                                task5_code();

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
        C_task_4 = WorstExTime[4];  // make the worst computational times of task 4 and task5 available to the server polling
        C_task_5 = WorstExTime[5];

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

        pthread_mutex_lock(&mutex_queue);
        for(int i = 0; i < BUFFER_SIZE; i++)
        {
                queue[i] = 0;
        }
        pthread_mutex_unlock(&mutex_queue);

// wait some time to show worst execution times, U and Ulub berfore starting the execution of tasks
        sleep(5);

// give the thread main the lowest priority so it's never going to preempt the other tasks
        if (getuid() == 0)
        {
                pthread_setschedparam(pthread_self(),SCHED_FIFO,&priomin);
        }

// set the threads attributes and store them in the correspondent struct        
        for (int i =0; i < NPERIODICTASKS; i++)
        {
                pthread_attr_init(&(attributes[i]));
                pthread_attr_setinheritsched(&(attributes[i]), PTHREAD_EXPLICIT_SCHED);

                pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);

                parameters[i].sched_priority = priomax.sched_priority - i;

                pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
        }


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

// join all the created threads so that the thread main waits for the threads to exit before exiting itslef
        pthread_join( thread_id[0], NULL);
        pthread_join( thread_id[1], NULL);
        pthread_join( thread_id[2], NULL);
        pthread_join( thread_id[3], NULL);

        exit(0);
}


void polling_server_code()
{


        printf("    PS START\n    |\n"); fflush(stdout);
// refill the capacity at each execution of the task_polling_server
        int Capacity_left = Capacity;

// As long as there are task in the queue 
        while (queue[h] == TASK_4 || queue [h] == TASK_5)
        {

// if the element in the queue is task 4 and there is enough capcity left to run it
                if (queue[h] == TASK_4 && Capacity_left >=  C_task_4)                {
                        task4_code();

// update the index of the queue
                        h++;
                        
// reduce the capacity left according to task 4 computational time
                        Capacity_left = Capacity_left - C_task_4;    
                }

                else if (queue[h] == TASK_5 && Capacity_left >= C_task_5)
                {
                        task5_code();

                        h++;

                        Capacity_left = Capacity_left - C_task_5; 
                }

// if ther is still a valid element in the queue, but the polling server has not enough capacity left to run it 
// to its completion                 
                else
                {

                        printf("    |\n    |    !!!!! NOT ENOUGH CAPACITY LEFT !!!!!\n"); fflush(stdout);          

// print to stdout the valid elements left in the queue because the polling server had no capacity left to run them
// during this execution 
                        printf("    |        Elements left in the queue -->"); fflush(stdout);

                        int i = h;  // starting from the current location in the queue
                        while(queue[i] == TASK_4 || queue[i] == TASK_5)
                        {
                                if (queue[i] == TASK_4)
                                {
                                        printf(" 4a "); fflush(stdout);
                                        i++;
                                }

                                if (queue[i] == TASK_5)
                                {
                                        printf(" 5a "); fflush(stdout);
                                        i++;
                                }
                        }
                        printf("\n    |\n"); fflush(stdout);
                        break;  // exit the loop 
                }
        }

// If there are no more valid elements in the queue: clear the queue and initialize its indexes 
// otherwise do nothing since, during the next execution, the polling server must strat from where
// it left.
        if (queue[h] != TASK_4 && queue[h] != TASK_5)
        {

// reinitialize the queue (fill it with zeroes up to the last valid element in the queue,
// the other elements are already zeroes)

                pthread_mutex_lock(&mutex_queue);
                for (int k = 0; k < h; k++)
                {
                        queue[k] = 0;
                }
                pthread_mutex_unlock(&mutex_queue);

// initialize the polling server queue pointer
                h = 0;

// initialize the task_1 queue pointer. It must be protected with a mutex since it is a global variable 
// shared between task 1 and task polling server 
                pthread_mutex_lock(&mutex_q);
                q = 0;
                pthread_mutex_unlock(&mutex_q);
        }

        printf("    |\n    PS END\n\n"); fflush(stdout);

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
                        uno = rand()%2;
        }


// when the random variable uno=0, then aperiodic task 4 must be executed
        if (uno == 0)
        {
                printf("            |--> 4a arrival\n");fflush(stdout);
                pthread_mutex_lock(&mutex_queue);     

// update the element of the queue "pointed" by q according to the arrival of a specific aperiodic task (task_4 in this case)
                queue[q] = TASK_4;                  

                pthread_mutex_unlock(&mutex_queue);  
               

                pthread_mutex_lock(&mutex_q);          
                // polling_server_task and must be protected
               
// after a new tak is added to the queue we must update the q index in order to have it stil pointing to the first empty element of the queue
                q++;                                   
                
                
                pthread_mutex_unlock(&mutex_q);

        }

// when the random variable uno equals 1, then aperiodic task 5 must be executed
        if (uno == 1)
        {
                printf("            |--> 5a arrival\n");fflush(stdout);

                pthread_mutex_lock(&mutex_queue);
                queue[q] = TASK_5;
                pthread_mutex_unlock(&mutex_queue);

                pthread_mutex_lock(&mutex_q);
                q++;
                pthread_mutex_unlock(&mutex_q);

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
        for (int i = 0; i < 1000; i++)
        {
                for (int j = 0; j < 1000; j++)
                        uno = rand()%2;
        }

// when the random variable uno=0, then aperiodic task 4 must be executed
        if (uno == 0)
        {
                printf("                    |--> 4a arrival\n");fflush(stdout);
                pthread_mutex_lock(&mutex_queue);     

// update the element of the queue "pointed" by q according to the arrival of a specific aperiodic task (task_4 in this case)
                queue[q] = TASK_4;                  

                pthread_mutex_unlock(&mutex_queue);  
               

                pthread_mutex_lock(&mutex_q);          
                // polling_server_task and must be protected
               
// after a new tak is added to the queue we must update the q index in order to have it stil pointing to the first empty element of the queue
                q++;                                   
                
                
                pthread_mutex_unlock(&mutex_q);

        }

// when the random variable uno equals 1, then aperiodic task 5 must be executed
        if (uno == 1)
        {
                printf("                    |--> 5a arrival\n");fflush(stdout);

                pthread_mutex_lock(&mutex_queue);
                queue[q] = TASK_5;
                pthread_mutex_unlock(&mutex_queue);

                pthread_mutex_lock(&mutex_q);
                q++;
                pthread_mutex_unlock(&mutex_q);

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
        for (int i = 0; i < 1000; i++)
        {
                for (int j = 0; j < 1000; j++)
                        uno = rand()%2;
        }

// when the random variable uno=0, then aperiodic task 4 must be executed
        if (uno == 0)
        {
                printf("                            |--> 4a arrival\n");fflush(stdout);
                pthread_mutex_lock(&mutex_queue);     

// update the element of the queue "pointed" by q according to the arrival of a specific aperiodic task (task_4 in this case)
                queue[q] = TASK_4;                  

                pthread_mutex_unlock(&mutex_queue);  
               

                pthread_mutex_lock(&mutex_q);          
                // polling_server_task and must be protected
               
// after a new tak is added to the queue we must update the q index in order to have it stil pointing to the first empty element of the queue
                q++;                                   
                
                
                pthread_mutex_unlock(&mutex_q);

        }

// when the random variable uno equals 1, then aperiodic task 5 must be executed
        if (uno == 1)
        {
                printf("                            |--> 5a arrival\n");fflush(stdout);

                pthread_mutex_lock(&mutex_queue);
                queue[q] = TASK_5;
                pthread_mutex_unlock(&mutex_queue);

                pthread_mutex_lock(&mutex_q);
                q++;
                pthread_mutex_unlock(&mutex_q);

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
        printf("    |--> 4a START\n"); fflush(stdout);
        for (int i = 0; i < 10; i++)
        {
                for (int j = 0; j < 1000; j++)
                        double uno = rand()*rand();
        }
        printf("    |--> 4a END\n"); fflush(stdout);
}


void task5_code()
{
        printf("    |--> 5a START\n"); fflush(stdout);
        for (int i = 0; i < 10; i++)
        {
                for (int j = 0; j < 1000; j++)	
                        double uno = rand()*rand();
        }	
        printf("    |--> 5a END\n"); fflush(stdout);
}

