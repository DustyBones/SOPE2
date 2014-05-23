
#include <stdio.h> 
#include <stdlib.h> 
#include <math.h>
#include <pthread.h> 
#include <semaphore.h>

#define NOT_SHARED 0
#define N_EMLEM(x) (sizeof(x)/sizeof(x[0]))

 sem_t done;
 pthread_mutex_t mut=PTHREAD_MUTEX_INITIALIZER;
 int *primes, *numPrimes;
 int N=2;
 
 
//------------------------------------------------------------------------------------------ 
// Type of the circular queue elements 
 
typedef unsigned long QueueElem; 
 
//------------------------------------------------------------------------------------------ 
// Struct for representing a "circular queue" 
// Space for the queue elements will be allocated dinamically by queue_init() 
 
typedef struct 
{ 
 QueueElem *v; // pointer to the queue buffer unsigned int capacity;
 unsigned int capacity; // queue capacity
 unsigned int first; // head of the queue 
 unsigned int last; // tail of the queue 
 sem_t empty; // semaphores and mutex for implementing the 
 sem_t full; // producer-consumer paradigm pthread_mutex_t mutex; 
 pthread_mutex_t mutex; 
} CircularQueue; 
 
 
//------------------------------------------------------------------------------------------ 
// Allocates space for circular queue 'q' having 'capacity' number of elements 
// Initializes semaphores & mutex needed to implement the producer-consumer paradigm 
// Initializes indexes of the head and tail of the queue 
// TO DO BY STUDENTS: ADD ERROR TESTS TO THE CALLS & RETURN a value INDICATING (UN)SUCESS 
 
void queue_init(CircularQueue **q, unsigned int capacity) // TO DO: change return value 
{ 
 *q = (CircularQueue *) malloc(sizeof(CircularQueue)); 
 sem_init(&((*q)->empty), 0, capacity); 
 sem_init(&((*q)->full), 0, 0); 
 pthread_mutex_init(&((*q)->mutex), NULL); 
 (*q)->v = (QueueElem *) malloc(capacity * sizeof(QueueElem)); 
 (*q)->capacity = capacity; 
 (*q)->first = 0; 
 (*q)->last = 0; 
} 
 
//------------------------------------------------------------------------------------------ 
// Inserts 'value' at the tail of queue 'q' 
 
void queue_put(CircularQueue *q, QueueElem value) { 
	sem_wait(&(q->empty));
	q->v[q->last]=value;
	(q->last)++;
	(q->last)%=(q->capacity);
	sem_post(&(q->full));
	return;
} 
 
//------------------------------------------------------------------------------------------ 
// Removes element at the head of queue 'q' and returns its 'value' 
 
QueueElem queue_get(CircularQueue *q) {
	QueueElem value;
	sem_wait(&(q->full));
	value=q->v[q->first];
	(q->last)--;
	(q->last)%=(q->capacity);
	sem_post(&(q->empty));
	return value;
} 
 
//------------------------------------------------------------------------------------------ 
// Frees space allocated for the queue elements and auxiliary management data 
// Must be called when the queue is no more needed 
 
void queue_destroy(CircularQueue *q) 
{ 
	free(q);
} 

void *processer (void *q){
	pthread_t cid;
	int temp, prime;
	
	prime=queue_get(q);
	
	if(prime>sqrt(N)){
		while(prime!=0){
			pthread_mutex_lock(&mut);
			primes[(*numPrimes)]=prime;
			(*numPrimes)++;
			pthread_mutex_unlock(&mut);
			prime=queue_get(q);
		}
		queue_destroy(q);
		sem_post(&done);
	}else{
		CircularQueue *newData;
		queue_init(&newData, 10);
		pthread_create(&cid, NULL, processer, &newData);
		temp=1;
		while(temp!=0){
			temp=queue_get(q);
			if(temp%prime!=0){
				queue_put(newData, temp);
			}
		}
		queue_put(newData, 0);
		queue_destroy(q);
		pthread_mutex_lock(&mut);
		primes[*numPrimes]=prime;
		(*numPrimes)++;
		pthread_mutex_unlock(&mut);
		pthread_join(cid, NULL); 
	}
	return NULL;
}

void *starter (void *arg){
	pthread_t cid;
	
	primes[*numPrimes]=2;
	(*numPrimes)++;
	
	if (N>2){
		CircularQueue *data;
		queue_init(&data, 10);
		pthread_create(&cid, NULL, processer, &data); 
		int i;
		for(i=3; i<=N; ++i){
			if(i%2!=0){
				queue_put(data, i);
			}
		}
		queue_put(data, 0);
		pthread_join(cid, NULL); 
	}else sem_post(&done);
	return NULL;
}

int cmpfunc (const void * a, const void * b){
   return ( *(int*)a - *(int*)b );
}
 
 int main(int argc, char *argv[]) { 
	pthread_t pid; 
 
	if (argc != 2 || ( (int) argv[1])<2) { 
		fprintf(stderr,"USAGE: %s numItems [numItems >= 2]\n",argv[0]); 
		exit(1); 
	} 
	N=*argv[1];
	
	//initialize the semaphore
	sem_init(&done, NOT_SHARED, 0);
	
	//allocate shared memory
	primes =  malloc(1.2*(*argv[1])/log(*argv[1]));
	numPrimes = malloc(sizeof(int));
	
	if (primes == 0 || numPrimes==0){
		printf("ERROR: Out of memory\n");
		return 2;
	}
	(*numPrimes)=0;
	
	//launch the threads
	pthread_create(&pid, NULL, starter, argv[1]); 
	sem_wait(&done);
 
	sem_destroy(&done); 
	pthread_join(pid, NULL); 

	//show the results
	qsort(primes, *numPrimes, sizeof(int), cmpfunc);
	int i;
	printf("Primes: %d", primes[0]);
	for(i=1; i<(*numPrimes); ++i){
		printf(", %d", primes[i]);
	}
	printf("\n");
	
	//release shared memory
	free(primes);
	free(numPrimes);
	
	return 0;
}
