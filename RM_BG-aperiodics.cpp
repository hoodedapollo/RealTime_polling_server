// Three periodsc threads with rate monotonic, two aperiodsc threads 
/***********************************************
 * compile with g++ -lpthread RMaperiodsci.cpo
 ************************************************/

/*
 * toggle 0 --> not arrived
 * toggle 1 --> arrived
 * toggle 2 --> stopped
 *
 */

#define COMPLETED    0
#define READY        1
#define STOPPED      2
#define IN_EXECUTION 3

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>


// #define _GNU_SOURCE

// application specific code
void polling_server_code( ); 
void task1_code( );
void task2_code( );
void task3_code( );
void task4_code( );

// thread functions 
void *polling_server( void *);
void *task2( void *);
void *task1( void *);
void *task3( void *);
void *task4( void *);

// initialization of mutexes and conditions
pthread_mutex_t mutex_task_4 = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mutex_task_5 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_toggle = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond_task_4 = PTHREAD_COND_INITIALIZER;
// pthread_cond_t cond_task_5 = PTHREAD_COND_INITIALIZER;

#define NTASKS 4
#define NPERIODICTASKS 3
#define NAPERIODICTASKS 1


long int periods[NTASKS];
struct timeval next_arrival_time[NTASKS];
long int WCET[NTASKS];
int missed_deadlines[NTASKS];
pthread_attr_t attributes[NTASKS];
struct sched_param parameters[NTASKS];
pthread_t thread_id[NTASKS];
long int toggle = COMPLETED;

int main()
{
        periods[0]= 200000000; 
        periods[1]= 100000000; 
        periods[2]= 400000000;

        //for aperiodic tasks we set the period equals to 0
        periods[3]= 0; 
//        periods[4]= 0; 

        struct sched_param priomax;
        priomax.sched_priority=sched_get_priority_max(SCHED_FIFO);
        struct sched_param priomin;
        priomin.sched_priority=sched_get_priority_min(SCHED_FIFO);

        if (getuid() == 0) // verifies if you are superuser
                pthread_setschedparam(pthread_self(),SCHED_FIFO,&priomax); //  set the priority of the current thread
        //  (the main one) as the max possible. This is
        //  done in order to measure the computational time
        //  of the thread code in the case that the thread
        //  has the max priority 

        // execute all tasks in standalone modality in order to measure execution times
        // (use gettimeofday). Use the computed values to update the worst case execution
        // time of each task.

        for (int i =0; i < NTASKS; i++)
        {
                struct timeval timeval1;
                struct timezone timezone1;
                struct timeval timeval2;
                struct timezone timezone2;
                gettimeofday(&timeval1, &timezone1);

                if (i==0)
                        task1_code();
//                if (i==1)
//                        task2_code();
                if (i==2)
                        task3_code();

                //aperiodic tasks
                if (i==3)
                        task4_code();
//                if (i==4)
//                        task5_code();

                gettimeofday(&timeval2, &timezone2);

                WCET[i]= 1000*((timeval2.tv_sec - timeval1.tv_sec)*1000000
                                +(timeval2.tv_usec-timeval1.tv_usec));
                printf("\nWorst Case Execution Time %d=%ld \n", i, WCET[i]); fflush(stdout);
        }

        double Ulub = NPERIODICTASKS*(pow(2.0,(1.0/NPERIODICTASKS)) -1);

        double U = 0;
        for (int i = 0; i < NPERIODICTASKS; i++)
                U+= ((double)WCET[i])/((double)periods[i]);

        if (U > Ulub)
        {
                printf("\n U=%lf Ulub=%lf Not schedulable\n", U, Ulub); fflush(stdout);
                return(-1);
        }
        printf("\n U=%lf Ulub=%lf Schedulable\n", U, Ulub);fflush(stdout);
        fflush(stdout);
        sleep(1);

        if (getuid() == 0)
                pthread_setschedparam(pthread_self(),SCHED_FIFO,&priomin);

        for (int i =0; i < NPERIODICTASKS; i++)
        {
                pthread_attr_init(&(attributes[i]));
                pthread_attr_setinheritsched(&(attributes[i]), PTHREAD_EXPLICIT_SCHED);

                pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);


                pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
        }

                parameters[1].sched_priority = priomax.sched_priority; 
                parameters[0].sched_priority = priomax.sched_priority -2;
                parameters[2].sched_priority = priomax.sched_priority -3;
        // aperiodic tasks
        for (int i =NPERIODICTASKS; i < NTASKS; i++)
        {
                pthread_attr_init(&(attributes[i]));
                pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);

                //set minimum priority (background scheduling)

                parameters[i].sched_priority = priomax.sched_priority - 1;
                pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
        }


        int iret[NTASKS];
        struct timeval ora;
        struct timezone zona;
        gettimeofday(&ora, &zona);

        for (int i = 0; i < NPERIODICTASKS; i++)
        {
                long int periods_micro = periods[i]/1000;
                next_arrival_time[i].tv_sec = ora.tv_sec + periods_micro/1000000;
                next_arrival_time[i].tv_usec = ora.tv_usec + periods_micro%1000000;
                missed_deadlines[i] = 0;
        }

        // thread creation
        iret[0] = pthread_create( &(thread_id[0]), &(attributes[0]), task1, NULL);
        iret[1] = pthread_create( &(thread_id[1]), &(attributes[1]), polling_server, NULL);
        iret[2] = pthread_create( &(thread_id[2]), &(attributes[2]), task3, NULL);
        iret[3] = pthread_create( &(thread_id[3]), &(attributes[3]), task4, NULL);
//        iret[4] = pthread_create( &(thread_id[4]), &(attributes[4]), task5, NULL);

        pthread_join( thread_id[0], NULL);
        pthread_join( thread_id[1], NULL);
        pthread_join( thread_id[2], NULL);


        exit(0);
}

void task1_code()
{

        // the task does something...

        double uno;
        printf("1 START\n"); fflush(stdout);
        for (int i = 0; i < 10; i++)
        {
                for (int j = 0; j < 1000; j++)
                        uno = rand()*rand()%10;
        }


        // when the random variable uno=0, then aperiodic task 4 must
        // be executed

        if (uno == 0)
        {
                pthread_mutex_lock(&mutex_toggle);
                printf(":Execute (4)\n");fflush(stdout);
                toggle = READY;
                printf("Ready\n"); fflush(stdout);
                pthread_mutex_unlock(&mutex_toggle);

        }

        // when the random variable uno=1, then aperiodic task 5 must
        // be executed
//         if (uno == 1)
//         {
//                 printf(":Execute (5)\n");fflush(stdout);
//                 pthread_mutex_lock(&mutex_task_5);
//                 pthread_cond_signal(&cond_task_5);
//                 pthread_mutex_unlock(&mutex_task_5);
//         }
        printf("1 END\n"); fflush(stdout);
}

void *task1( void *ptr)
{
        int i=0;
        struct timespec waittime;
        waittime.tv_sec=0; /* seconds set to zero since task period is less than zero*/
        waittime.tv_nsec = periods[0]; /* nanoseconds equal to the period of the task defined in the main thread*/

        /* forcing the thread to run on CPU 0 */
        cpu_set_t cset;
        CPU_ZERO( &cset );
        CPU_SET( 0, &cset);

        pthread_setaffinity_np( pthread_self(), sizeof(cpu_set_t), &cset);

        for (i=0; i < 100; i++)
        {

                task1_code(); // task do something

                struct timeval ora;
                struct timezone zona;
                gettimeofday(&ora, &zona);

                long int timetowait= 1000*((next_arrival_time[0].tv_sec - ora.tv_sec)*1000000
                                +(next_arrival_time[0].tv_usec-ora.tv_usec)); //  time interval between now and the next arrival time 
                if (timetowait <0)
                        missed_deadlines[0]++;

                waittime.tv_sec = timetowait/1000000000;
                waittime.tv_nsec = timetowait%1000000000;

                nanosleep(&waittime, NULL);  // sleep till next arrival time

                // computing next arrival time
                long int periods_micro=periods[0]/1000;
                next_arrival_time[0].tv_sec = next_arrival_time[0].tv_sec +
                        periods_micro/1000000;
                next_arrival_time[0].tv_usec = next_arrival_time[0].tv_usec +
                        periods_micro%1000000;

        }
}
void polling_server_code()
{
        struct timespec serv_capacity;
        serv_capacity.tv_sec = 1;
        serv_capacity.tv_nsec = 1000;
        printf("PS \n"); fflush(stdout);

// If toggle is equal to STOPPED it means that the aperiodic task was stopped during its
// execution by the signal SIGSTOP. In this case signal the aperiodic task to continue and
// sleep for an interval of time equal to the capacity of the polling server to let the aperiod task 
// take the processor
  
        if(toggle == STOPPED)
        {
                pthread_mutex_lock(&mutex_toggle);
                toggle = IN_EXECUTION;
                pthread_mutex_unlock(&mutex_toggle);
                pthread_kill(thread_id[3], SIGCONT);
                printf("Continue\n"); fflush(stdout);
                nanosleep(&serv_capacity,NULL);
        }

// If the toggle is equal to READY it means that an aperiodic task has arrived and its ready to scheduled
// for the firs time. In this case the polling server signals a condition variable to the aperiodic thread
// to release the mutex and start the execution of the thread. The polling server then sleeps for an interval 
// of time equal to its capacity to let the aperiodic task take the processor
        if(toggle == READY)
        {
                pthread_mutex_lock(&mutex_task_4);
                pthread_cond_signal(&cond_task_4);
                pthread_mutex_unlock(&mutex_task_4);
                nanosleep(&serv_capacity,NULL);
        }

// If the toggle is equal different of COMPLETED means that the aperiodic threads was stopped before
// its completion and then the server polling must send a signal to the aperiodic task to stop it and
// change the toggle to STOPPED 
        if(toggle == IN_EXECUTION)
        {        
                pthread_mutex_lock(&mutex_toggle);
                toggle = STOPPED;
                pthread_kill(thread_id[3],SIGSTOP);
                printf("Stop\n"); fflush(stdout);
                pthread_mutex_unlock(&mutex_toggle);
        printf("PS END\n"); fflush(stdout);
        }
}


void *polling_server( void *ptr )
{
        int i=0;
        struct timespec waittime;
        waittime.tv_sec=0; /* seconds */
        waittime.tv_nsec = periods[1]; /* nanoseconds */

        /* forcing the thread to run on CPU 0 */
        cpu_set_t cset;
        CPU_ZERO( &cset );
        CPU_SET( 0, &cset);

        pthread_setaffinity_np( pthread_self(), sizeof(cpu_set_t), &cset);

        for (i=0; i < 100; i++)
        {

                polling_server_code();
                struct timeval ora;
                struct timezone zona;
                gettimeofday(&ora, &zona);
                long int timetowait= 1000*((next_arrival_time[1].tv_sec -
                                        ora.tv_sec)*1000000 +(next_arrival_time[1].tv_usec-ora.tv_usec));
                if (timetowait <0)
                        missed_deadlines[1]++;
                waittime.tv_sec = timetowait/1000000000;
                waittime.tv_nsec = timetowait%1000000000;
                nanosleep(&waittime, NULL);
                long int periods_micro=periods[1]/1000;
                next_arrival_time[1].tv_sec = next_arrival_time[1].tv_sec + periods_micro/1000000;
                next_arrival_time[1].tv_usec = next_arrival_time[1].tv_usec +
                        periods_micro%1000000;

        }
}

void task3_code()
{
        printf("3 START\n"); fflush(stdout);
        for (int i = 0; i < 10; i++)
        {
                for (int j = 0; j < 1000; j++)
                {		
                        double uno = rand()*rand();
                }
        }
        printf("3 END\n"); fflush(stdout);
}
void *task3( void *ptr)
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


        for (i=0; i < 100; i++)
        {

                task3_code();
                struct timeval ora;
                struct timezone zona;
                gettimeofday(&ora, &zona);
                long int timetowait= 1000*((next_arrival_time[2].tv_sec - ora.tv_sec)*1000000
                                +(next_arrival_time[2].tv_usec-ora.tv_usec));
                if (timetowait <0)
                        missed_deadlines[2]++;
                waittime.tv_sec = timetowait/1000000000;
                waittime.tv_nsec = timetowait%1000000000;
                nanosleep(&waittime, NULL);
                long int periods_micro=periods[2]/1000;
                next_arrival_time[2].tv_sec = next_arrival_time[2].tv_sec +
                        periods_micro/1000000;
                next_arrival_time[2].tv_usec = next_arrival_time[2].tv_usec +
                        periods_micro%1000000;

        }
}

void task4_code()
{
        printf("4a START\n"); fflush(stdout);

        pthread_mutex_lock(&mutex_toggle);
        toggle = IN_EXECUTION;
        pthread_mutex_unlock(&mutex_toggle);
        for (int i = 0; i < 100000; i++)
        {
                for (int j = 0; j < 1000; j++)
                        double uno = rand()*rand();
        }
// Set the toggle status to COMPLETED to let the polling server know that the aperiodic
// task was completed
        pthread_mutex_lock(&mutex_toggle);
        printf("4a END\n"); fflush(stdout);
        toggle = COMPLETED;
        printf("Completed\nPS END\n"); fflush(stdout);
        pthread_mutex_unlock(&mutex_toggle);
}

void *task4( void *ptr)
{

        /* forcing the thread to run on CPU 0 */
        cpu_set_t cset;
        CPU_ZERO( &cset );
        CPU_SET( 0, &cset);

        pthread_setaffinity_np( pthread_self(), sizeof(cpu_set_t), &cset);

        while (1)
        {
                // waiting for task 1 to signal the condition

                pthread_mutex_lock(&mutex_task_4);
                pthread_cond_wait(&cond_task_4, &mutex_task_4);
                pthread_mutex_unlock(&mutex_task_4);
                task4_code();
        }
}

// void task5_code()
// {
//         printf("5a START\n"); fflush(stdout);
//         for (int i = 0; i < 10; i++)
//         {
//                 for (int j = 0; j < 1000; j++)	
//                         double uno = rand()*rand();
//         }	
//         printf("5a END\n"); fflush(stdout);
// }
// 
// void *task5( void *ptr)
// {
// 
//         /* forcing the thread to run on CPU 0 */
//         cpu_set_t cset;
//         CPU_ZERO( &cset );
//         CPU_SET( 0, &cset);
// 
//         pthread_setaffinity_np( pthread_self(), sizeof(cpu_set_t), &cset);
// 
//         while(1)
//         {
//                 // waiting for task 1 to signal the condition
// 
//                 pthread_mutex_lock(&mutex_task_5);
//                 pthread_cond_wait(&cond_task_5, &mutex_task_5);
//                 pthread_mutex_unlock(&mutex_task_5);
