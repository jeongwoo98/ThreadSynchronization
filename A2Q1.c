#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <unistd.h>
//Jeongwoo Moon, 260804750

#define MAX(x, y) (x > y ? x : y)	//Used to update minimum, maximum waiting times 
#define MIN(x, y) (x < y ? x : y)	//for reader and writer threads 

int NB_WRITES, NB_READS;			//Number iterations a reader or writer thread would 
									//attempt to access the shared variable
const int NB_WTHREADS = 10;			//Will later create 10 writer threads
const int NB_RTHREADS = 500;		//Will later create 500 reader threads

int shared_var, read_count;			//Shared variable in this assignment is an integer
									//read_count keeps track of how many threads are 
									//currently reading the object(shared variable)
double min_wtime, max_wtime, tot_wtime;	//times for writers
double min_rtime, max_rtime, tot_rtime;	//times for readers
static sem_t mutex, rw_mutex;
//mutex: semaphore to ensure mutual exclusion when read_count is updated
//rw_mutex: semaphore to ensure mutual exclusion for the writer threads



/* PSEUDO Code for the writer: 
do{
	wait(mutex);
	read_count++
	if(read_count==1) wait(rw_mutex);
	signal(mutex);
	
	perform read (...)
	
	wait(mutex);
	read_count--;
	if(read_count==0) signal(rw_mutex);
	signal (mutex);
}while(true);
*/
void *reader(void *arg) 
{
  int temp, sleep_time;		//temp: holder for updating the shared variable
  double timeCount;
  int i = 0;
  while (i++ < NB_READS) 	//loop for the number of times the reader should attempt to access 
  { 	
    clock_t tstart = clock();
    sem_wait(&mutex);		//wait(mutex);
    read_count++;			//read_count++;
    if (read_count == 1)
    {
      sem_wait(&rw_mutex);	//wait(rw_mutex);
    }
    sem_post(&mutex);		//signal(mutex);
    clock_t tend = clock();
    timeCount = (double)(tend - tstart) / CLOCKS_PER_SEC * 1000;

    min_rtime = MIN(min_rtime, timeCount);	//update minimum time if necessary 
    max_rtime = MAX(max_rtime, timeCount);	//update maximum time if necessary 
    tot_rtime += timeCount;					//increment total time, which we will use to get avg

    sleep_time = rand() % 101 * 1000;	//go to sleep for a random amount of time, between
    usleep(sleep_time);					//0 to 100 milliseconds
    temp = shared_var;					//temp holds the current value of shared_int

    sem_wait(&mutex);		//wait(mutex);
    read_count--;			//read_count--;
    if (read_count == 0) 
    {
      sem_post(&rw_mutex);	//signal(rw_mutex);
    }
    sem_post(&mutex);		//signal(mutex);
  }
}

/*PSEUDO Code for the writer: 
do{
	wait(rw_mutex);
	
		(...)
	write performed
		(...)
	signal=(rw_mutex);
	
	}while(true);
*/
void *writer(void *arg) 
{
  int temp, sleep_time;		//temp = holder to update the shared variable later on 
  double timeCount;			//to update min,max and total times that a thread takes 
  int i = 0;
  
  while (i++ < NB_WRITES)	//loop for the number of times the writer should attempt to access 
  {	
    clock_t tstart = clock(); 
    sem_wait(&rw_mutex);	//wait(rw_mutex);
    clock_t tend = clock();
    timeCount = (double)(tend-tstart)/CLOCKS_PER_SEC * 1000; //get time elapsed in milliseconds
    
    min_wtime = MIN(min_wtime, timeCount); //update minimum time if necessary 
    max_wtime = MAX(max_wtime, timeCount); //update maximum time if necessary 
    tot_wtime += timeCount;				//increment total time, which we will use to get avg 

    sleep_time = rand() % 101 * 1000;	//go to sleep for a random amount of time, between 
    usleep(sleep_time);					//0 to 100 milliseconds
    temp += shared_var;					//temp holds the current value of shared_int
    temp += 10;							//temp holds the updated value, ie. writing process
    shared_var = temp;					//shared variable is updated

    sem_post(&rw_mutex);	//signal(rw_mutex);
  }
}

int main(int argc, char *argv[])
{
  NB_WRITES = atoi(argv[1]);	//cmd- line argument: ./a.out NUMBER_OF_WRITES NUMBER_OF_READS
  NB_READS = atoi(argv[2]);
  read_count = 0;		//initialize read_count to 0

  if (sem_init(&mutex, 0, 1) == -1 || sem_init(&rw_mutex, 0, 1) == -1)	//initialize semaphores
  {			//sem_init returns 0 upon success, -1 on failure 
    printf("There was an error in initializing the sempahores\n");
    exit(1);
  }

  pthread_t rthreads[NB_RTHREADS], wthreads[NB_WTHREADS];	//initialize arrays of reader and writer threads
  
  max_wtime = 0;		//numerical limit for the maximum writing time 
  min_wtime = DBL_MAX;	//numerical limit for the minimum writing time
  tot_wtime = 0;		//total waiting time for writing
  
  max_rtime = 0;
  min_rtime = DBL_MAX;
  tot_rtime = 0;		//total waiting time for reading 
  
  srand(time(NULL));	// to set a different seed of random through rand, in each execution 
  
  for (int i = 0; i < NB_RTHREADS; i++) //create 500 reader threads
  {	
    pthread_create(&rthreads[i], NULL, reader, &shared_var);
  }	
  for (int j = 0; j < NB_WTHREADS; j++) //create 10 writer threads
  {	
    pthread_create(&wthreads[j], NULL, writer, &shared_var);
  }
  for (int i = 0; i < NB_RTHREADS; i++) 
  {	
    pthread_join(rthreads[i], NULL);
  }
  for (int j = 0; j < NB_WTHREADS; j++) 
  {
    pthread_join(wthreads[j], NULL);
  }

  long totalRcount, totalWcount;
  
  totalRcount = NB_READS*NB_RTHREADS;
  totalWcount = NB_WRITES*NB_WTHREADS;
  
  printf("Minimum reader wait time: %f milliseconds\n", min_rtime);
  printf("Maximum reader wait time: %f milliseconds\n", max_rtime);
  printf("Average reader wait time: %f milliseconds\n\n", tot_rtime/totalRcount);
  printf("Minimum writer wait time: %f milliseconds\n", min_wtime);
  printf("Maximum writer wait time %f milliseconds\n", max_wtime);
  printf("Average writer wait time: %f milliseconds\n", tot_wtime/totalWcount);
  return 0;
}



