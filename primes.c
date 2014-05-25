#include <stdio.h> 
#include <stdlib.h> 
#include <math.h>
#include <pthread.h> 
#include <semaphore.h>

 pthread_mutex_t threadMutex=PTHREAD_MUTEX_INITIALIZER;	//thread related mutex
 pthread_t* threads;									//list of threads
 int numThreads;										//number of threads
 pthread_mutex_t primeMutex=PTHREAD_MUTEX_INITIALIZER;	//prime related mutex
 int* primes;											//list of primes
 int numPrimes;											//number of primes
 
 int N; 												//list size
 sem_t done;											//semaphore, done
 										
 typedef unsigned long QueueElem;  						// Type of the circular queue elements 
 
 typedef struct { 										// Struct for representing a "circular queue" 
	QueueElem *v;										// pointer to the queue buffer unsigned int capacity;
	unsigned int capacity;				 				// queue capacity
	unsigned int first; 								// head of the queue 
	unsigned int last; 									// tail of the queue 
	sem_t empty;										// semaphores and mutex for implementing the 
	sem_t full; 										// producer-consumer paradigm pthread_mutex_t mutex; 
	pthread_mutex_t mutex; 
} CircularQueue; 
 
//------------------------------------------------------------------------------------------ 
// Allocates space for circular queue 'q' having 'capacity' number of elements 
// Initializes semaphores & mutex needed to implement the producer-consumer paradigm 
// Initializes indexes of the head and tail of the queue 
// TO DO BY STUDENTS: ADD ERROR TESTS TO THE CALLS & RETURN a value INDICATING (UN)SUCESS 
 
int queue_init(CircularQueue **q, unsigned int capacity){ // TO DO: change return value { 
	int i;
	*q = (CircularQueue *) malloc(sizeof(CircularQueue)); 
	if((i=sem_init(&((*q)->empty), 0, capacity))!=0){
		free(q);
		exit(i);
	} 
	if((i=sem_init(&((*q)->full), 0, 0))!=0){
		free(q);
		exit(i);
	} 
	if((i=pthread_mutex_init(&((*q)->mutex), NULL))!=0){
		free(q);
		exit(i);
	}
	(*q)->v = (QueueElem *) malloc(capacity * sizeof(QueueElem)); 
	(*q)->capacity = capacity; 
	(*q)->first = 0; 
	(*q)->last = 0; 
	return 0;
} 
 
//------------------------------------------------------------------------------------------ 
// Inserts 'value' at the tail of queue 'q' 
 
void queue_put(CircularQueue *q, QueueElem value) { 
	sem_wait(&q->empty);
	pthread_mutex_lock(&q->mutex);
	q->v[q->last]=value;
	q->last=(q->last+1)%(q->capacity);
	pthread_mutex_unlock(&q->mutex);
	sem_post(&q->full);
	return;
} 
 
//------------------------------------------------------------------------------------------ 
// Removes element at the head of queue 'q' and returns its 'value' 
 
QueueElem queue_get(CircularQueue *q) {
	QueueElem value;
	sem_wait(&q->full);
	pthread_mutex_lock(&(q->mutex));
	value=q->v[q->first];
	q->first=(q->first+1)%(q->capacity);
	pthread_mutex_unlock(&q->mutex);
	sem_post(&q->empty);
	return value;
} 
 
//------------------------------------------------------------------------------------------ 
// Frees space allocated for the queue elements and auxiliary management data 
// Must be called when the queue is no more needed 
 
void queue_destroy(CircularQueue *q) { 
	//TODO
	free(q->v);
	free(q);
} 

void *processer (void *q){
	
	QueueElem prime, remainder;
	prime=queue_get(q);
	remainder=queue_get(q);
	if(prime>sqrt(N)){
		while(remainder!=0){
			pthread_mutex_lock(&primeMutex);
			primes[numPrimes]=remainder;
			(numPrimes)++;
			pthread_mutex_unlock(&primeMutex);
			remainder=queue_get(q);
		}
		queue_destroy(q);
		sem_post(&done);
	}else{
		CircularQueue* newData;
		queue_init(&newData, 10);
		pthread_mutex_lock(&threadMutex);
		pthread_create(&threads[numThreads], NULL, processer, newData);
		numThreads++;
		pthread_mutex_unlock(&threadMutex);
		while(remainder!=0){
			if(remainder!=0){
				if(remainder%prime!=0){
					queue_put(newData, remainder);
				}
			}
			remainder=queue_get(q);
		}
		queue_put(newData, 0);
		queue_destroy(q);
	}
	pthread_mutex_lock(&primeMutex);
	primes[numPrimes]=prime;
	numPrimes++;
	pthread_mutex_unlock(&primeMutex);
	
	return NULL;
}

void *starter (void *arg){
	
	primes[numPrimes]=2;
	numPrimes++;
	
	if (2<N){
		CircularQueue* data;
		queue_init(&data, 10);
		pthread_mutex_lock(&threadMutex);
		pthread_create(&threads[numThreads], NULL, processer, data);
		numThreads++;
		pthread_mutex_unlock(&threadMutex);
		int i;
		for(i=3; i<=N; i+=2){
			queue_put(data, i);
		}
		queue_put(data, 0);
	}else sem_post(&done);
	return NULL;
}

int cmpfunc (const void * a, const void * b){
   return ( *(int*)a - *(int*)b );
}
 
 int main(int argc, char *argv[]) { 
	
	if (argc != 2 || atoi(argv[1])<2) { 
		fprintf(stderr,"USAGE: %s numItems [numItems >= 2]\n",argv[0]); 
		exit(1); 
	} 
	
	//allocate shared memory
	N=atoi(argv[1]);
	primes = (int *)malloc(ceil(1.2*(N)/log(N))*sizeof(int));
	numPrimes=0;
	threads = (pthread_t *)malloc(ceil(sqrt(N))*sizeof(pthread_t));
	numThreads=0;
	
	
	//initialize the semaphore
	sem_init(&done, 0, 0);
	
	//launch the threads
	pthread_mutex_lock(&threadMutex);
	pthread_create(&threads[numThreads], NULL, starter, &N);
	numThreads++;
	pthread_mutex_unlock(&threadMutex);
	
	//wait untill everything is finished
	sem_wait(&done);
	sem_destroy(&done);
	int i;
	for(i=0; i<numThreads;++i){
		pthread_join(threads[i], NULL);
	}

	//show the results
	qsort(primes, numPrimes, sizeof(int), cmpfunc);
	int j;
	printf("Primes between 2 and %d: %d\n", N, numPrimes);
	for(j=0; j<numPrimes; ++j){
		printf("%d. %d\n",j+1, primes[j]);
	}
	
	return 0;
}
