#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SIZE 100
#define NUMB_THREADS 2
#define SHARED 1

typedef char buffer_t;
buffer_t buffer[SIZE];
int buffer_index;
FILE *fileptr;
char *filebuf;
FILE *write_ptr;

int done;


pthread_mutex_t buffer_mutex;
sem_t full_sem;  /* when 0, buffer full */
sem_t empty_sem; /* when 0, buffer empty */



/*insert a byte into the buffer */
void insertbuffer(buffer_t value) {
    if (buffer_index < SIZE) {
        buffer[buffer_index++] = value;
    } else {
        printf("Buffer overflow\n");
    }
}

/*go back one index, insertbuffer will replace previous index.*/
buffer_t dequeuebuffer() {
    if (buffer_index > 0) {
        return buffer[--buffer_index];
    } else {
        printf("Buffer underflow\n");
    }
	return 0;
}

void *producer(void *thread_n) {
    int thread_numb = *(int *)thread_n;
    buffer_t value;
	/* open a file */
	fileptr = fopen("input.txt", "rb"); 
	/* check if end of file */
    while ((value = fgetc(fileptr)) != EOF ) {
		/* if end of file, done is set to 1 */
		done = 1;
		/* random sleep*/
		usleep((rand()%3+1)*100000);
		/* if sem = 0, then wait, otherwise if >0 continue and decrement.*/
		sem_wait(&empty_sem); 
		pthread_mutex_lock(&buffer_mutex);
		/* insert read byte value into shared buffer */
		insertbuffer(value);
		pthread_mutex_unlock(&buffer_mutex);
		/* post increment full semaphore */
		sem_post(&full_sem); 
		printf("Producer %d added %c to buffer\n", thread_numb, value);
		
    }
	/* if out of while loop, done */
	done = 0;
	/* close file and exit pthread */
	fclose(fileptr);
    pthread_exit(0);
}

void *consumer(void *thread_n) {
    int thread_numb = *(int *)thread_n;
    buffer_t value;
	/* open file2 */
	write_ptr = fopen("output.txt","wb");
	/* while file 1 has not finished completely writing */
    while (done != 0) {
		/* random sleep */
		usleep((rand()%3+1)*100000);
		sem_wait(&full_sem);
		pthread_mutex_lock(&buffer_mutex);
		/* take byte out of buffer and write to file */
		value = dequeuebuffer();
		fputc(value, write_ptr);
		pthread_mutex_unlock(&buffer_mutex);
		/* post increment empty semaphore */
		sem_post(&empty_sem);
		printf("Consumer %d dequeue %c from buffer\n", thread_numb, value);
		
   }
   /* close file and exit pthread */
	fclose(write_ptr);
    pthread_exit(0);
}

int main(int argc, char **argv) {
    buffer_index = 0;
    pthread_mutex_init(&buffer_mutex, NULL);
    sem_init(&full_sem, SHARED, 0);
    sem_init(&empty_sem, SHARED, 1);
	pthread_t thread[NUMB_THREADS];
    int thread_numb[NUMB_THREADS];
    int i;
    for (i = 0; i < NUMB_THREADS; ) {
        thread_numb[i] = i;
        pthread_create(thread + i, NULL, producer, thread_numb + i);
        i++;
        thread_numb[i] = i;
        pthread_create(&thread[i], NULL, consumer, &thread_numb[i]);
        i++;
    }
    for (i = 0; i < NUMB_THREADS; i++)
        pthread_join(thread[i], NULL);
    pthread_mutex_destroy(&buffer_mutex);
    sem_destroy(&full_sem);
    sem_destroy(&empty_sem);
	pthread_exit(NULL);
    return 0;
}

