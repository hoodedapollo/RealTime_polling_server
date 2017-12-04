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


#define NTASKS 6
#define NPERIODICTASKS 4
#define NAPERIODICTASKS 0 

#define BUFFER_SIZE 1000 // stil to be evaluated
#define TASK_4 1
#define TASK_5 2
#define CAPACITY 10000000 // still to be  evaluated

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



int q = 0;
int j = 0;
int queue[BUFFER_SIZE] = {0};
long int C_task_4, C_task_5;
long int Capacity = CAPACITY;

long int periods[NTASKS];
struct timeval next_arrival_time[NTASKS];
long int WCET[NTASKS];
int missed_deadlines[NTASKS];
pthread_attr_t attributes[NTASKS];
struct sched_param parameters[NTASKS];
pthread_t thread_id[NTASKS];

int main()
{
        periods[0]= 100000000; 
        periods[1]= 200000000; 
        periods[2]= 400000000;
        periods[3]= 800000000;

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
                {
                 printf("Polling server"); fflush(stdout);
                 WCET[i] = CAPACITY;
                }
                else
                {
                if (i==1)
                        task1_code();
                if (i==2)
                        task2_code();
                if (i==3)
                        task3_code();

                //aperiodic tasks
                if (i==4)
                        task4_code();
                if (i==5)
                        task5_code();

                gettimeofday(&timeval2, &timezone2);

                WCET[i]= 1000*((timeval2.tv_sec - timeval1.tv_sec)*1000000
                                +(timeval2.tv_usec-timeval1.tv_usec));
                }
                printf("\nWorst Case Execution Time %d=%ld \n", i, WCET[i]); fflush(stdout);
        }

        C_task_4 = WCET[4];
        C_task_5 = WCET[5];

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

                parameters[i].sched_priority = priomax.sched_priority - i;

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
        iret[0] = pthread_create( &(thread_id[0]), &(attributes[0]), polling_server, NULL);
        iret[1] = pthread_create( &(thread_id[1]), &(attributes[1]), task1, NULL);
        iret[2] = pthread_create( &(thread_id[2]), &(attributes[2]), task2, NULL);
        iret[3] = pthread_create( &(thread_id[3]), &(attributes[3]), task3, NULL);

        pthread_join( thread_id[0], NULL);
        pthread_join( thread_id[1], NULL);
        pthread_join( thread_id[2], NULL);
        pthread_join( thread_id[3], NULL);


        exit(0);
}

void polling_server_code()
{

        printf("PS START\n"); fflush(stdout);
        // refill the capacity at each execution of the task_polling_server
        int Capacity_left = Capacity;
        // As long as there are task in the queue and there is capacity left to run at least the task
        // that requires less capacity run the task
        while (queue[j] == TASK_4 || queue [j] == TASK_5)
        {

                if (queue[j] == TASK_4 && Capacity_left >=  C_task_4) // if the element in the queue is 
                        // and there is enough capcity left to
                        // run task_4 signal task_4 to run
                {
                        task4_code();

                        // increase the index of the queue, it's important to update the index of the qu
                        // before updating the capacity left because the we can exit the loop due to con
                        // related to both queue and capacity, but we want to know in particular if we e
                        // due to the queue in order to initialize it so that task 1 update the queue fr
                        // beginning of the queue. In this way we optimize the use of the queue buffer s


                        j++;                                            // update the index of the queue

                        Capacity_left = Capacity_left - C_task_4;       // reduce the capacity left acco
                        // to task_4 computational time
                }

                else if (queue[j] == TASK_5 && Capacity_left >= C_task_5)
                {
                        task5_code();

                        j++;

                        Capacity_left = Capacity_left - C_task_5;       // reduce the capacity left acco
                        // to task_5 computational time
                }
                else
                {
                        break;  // in the case there is capacity left to run the task that requires the 
                        // of computational time but the task in the queue that must be runned
                        // requires more computational time than it is available, we need to forcefully 
                        // the loop otherwise we will be stuck inside of it: there is a valid element in
                        // queue but we cannot run it so the pointer j is never going to change neither
                        // the capacity left is going to be updated,
                }
        }

        // clear the queue and initialize its indexes only if there are no more valid element in the que
        // if the capacity left in not sufficient to run more aperiodic task, then, during the next exec
        // the polling server periodic task,  you want the queue to start from the exact point it stoppe



        if (queue[j] != TASK_4 && queue [j] != TASK_5) // when there are no more valid element in the qu
        {

                // delete the queue (fill it with zeroes up to the last valid element in the queue)

                pthread_mutex_lock(&mutex_queue);
                for (int k = 0; k < j; k++)
                {
                        queue[k] = 0;
                }
                pthread_mutex_unlock(&mutex_queue);

                // initialize the polling server queue pointer
                j = 0;

                // initialize the task_1 queue pointer

                pthread_mutex_lock(&mutex_q); // variable is shared between task_1 and task_polling_serv
                // it must be protected with a mutex
                q = 0;
                pthread_mutex_unlock(&mutex_q);
        }

        printf("PS END\n"); fflush(stdout);

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
        printf("1 START\n"); fflush(stdout);
        for (int i = 0; i < 10; i++)
        {
                for (int j = 0; j < 1000; j++)
                        uno = rand()%2;
        }


        // when the random variable uno=0, then aperiodic task 4 must
        // be executed

        if (uno == 0)
        {
                printf("--> 4 added to queue\n");fflush(stdout);
                pthread_mutex_lock(&mutex_queue);     // the variable queue is shared among task_1 and
                // polling_server_task so it must be protected with a mut

                queue[q] = TASK_4;                    // element of the queue "pointed" by i updated according
                // to the arrival of a specific aperiodic task (task_4 in
                // this case)

                pthread_mutex_unlock(&mutex_queue);   // after the queue is updated we can relase the mutex in
                // charge of protecting it

                pthread_mutex_lock(&mutex_q);          // i is another variable shared between task_1 and
                // polling_server_task and must be protected

                q++;                                   // after a new tak is added to the queue we must update
                // the i index in order to have it stil pointing to the
                // first empty element of the queue
                pthread_mutex_unlock(&mutex_q);

        }

        // when the random variable uno=1, then aperiodic task 5 must
        // be executed
        if (uno == 1)
        {
                printf("--> 5 added to queue\n");fflush(stdout);

                pthread_mutex_lock(&mutex_queue);
                queue[q] = TASK_5;
                pthread_mutex_unlock(&mutex_queue);

                pthread_mutex_lock(&mutex_q);
                q++;
                pthread_mutex_unlock(&mutex_q);

        }
        printf("1 END\n"); fflush(stdout);
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
                if (timetowait <1)
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
        printf("2 START\n"); fflush(stdout);
        for (int i = 0; i < 10; i++)
        {
                for (int j = 0; j < 1000; j++)
                        uno = rand()%2;
        }


        // when the random variable uno=0, then aperiodic task 4 must
        // be executed

        if (uno == 0)
        {
                printf("--> 4 added to queue\n");fflush(stdout);
                pthread_mutex_lock(&mutex_queue);     // the variable queue is shared among task_1 and
                // polling_server_task so it must be protected with a mut

                queue[q] = TASK_4;                    // element of the queue "pointed" by i updated according
                // to the arrival of a specific aperiodic task (task_4 in
                // this case)

                pthread_mutex_unlock(&mutex_queue);   // after the queue is updated we can relase the mutex in
                // charge of protecting it

                pthread_mutex_lock(&mutex_q);          // i is another variable shared between task_1 and
                // polling_server_task and must be protected

                q++;                                   // after a new tak is added to the queue we must update
                // the i index in order to have it stil pointing to the
                // first empty element of the queue
                pthread_mutex_unlock(&mutex_q);

        }

        // when the random variable uno=1, then aperiodic task 5 must
        // be executed
        if (uno == 1)
        {
                printf("--> 5 added to queue\n");fflush(stdout);

                pthread_mutex_lock(&mutex_queue);
                queue[q] = TASK_5;
                pthread_mutex_unlock(&mutex_queue);

                pthread_mutex_lock(&mutex_q);
                q++;
                pthread_mutex_unlock(&mutex_q);

        }
        printf("2 END\n"); fflush(stdout);
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
        printf("4a START\n"); fflush(stdout);
        for (int i = 0; i < 10; i++)
        {
                for (int j = 0; j < 1000; j++)
                        double uno = rand()*rand();
        }
        printf("4a END\n"); fflush(stdout);
}


void task5_code()
{
        printf("5a START\n"); fflush(stdout);
        for (int i = 0; i < 10; i++)
        {
                for (int j = 0; j < 1000; j++)	
                        double uno = rand()*rand();
        }	
        printf("5a END\n"); fflush(stdout);
}

