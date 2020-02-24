#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define READER_NUM 5
#define WRITER_NUM 5
#define READ_NUM 5
#define WRITE_NUM 5

unsigned int sharedValue = 0;
int waitingReadersNum = 0;
int readers = 0;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c_read = PTHREAD_COND_INITIALIZER;
pthread_cond_t c_write = PTHREAD_COND_INITIALIZER;

void *readMain (void *threadArgument);
void *writeMain (void *threadArgument);

int main(int argc, char *argv[]) {
    int i;

    int readerNum[READER_NUM];
	int writerNum[WRITER_NUM];

    pthread_t readerTIDs[READER_NUM];
    pthread_t writerTIDs[WRITER_NUM];

    srandom((unsigned int)time(NULL));

    for (i = 0; i < READER_NUM; i++) {
        readerNum[i] = i;
        pthread_create(&readerTIDs[i], NULL, readMain, &readerNum[i]);
    }

    for (i = 0; i < WRITER_NUM; i++) {
        writerNum[i] = i;
        pthread_create(&writerTIDs[i], NULL, writeMain, &writerNum[i]);
    }

    for (i = 0; i < READER_NUM; i++) {
        pthread_join(readerTIDs[i], NULL);
    }

    for (i = 0; i < WRITER_NUM; i++) {
        pthread_join(writerTIDs[i], NULL);
    }
    
    return 0;
}

void *readMain(void *threadArgument) {
    int id = *((int*)threadArgument);
    int i = 0, numReaders = 0;

    for (i = 0; i < READ_NUM; i++) {
        // wait so that read and write do not all happen at once
        usleep(1000 * (random() % READER_NUM + WRITER_NUM));
        
        pthread_mutex_lock(&m);
            waitingReadersNum += 1;
            while (readers == -1) {
                pthread_cond_wait(&c_read, &m);
            }
            waitingReadersNum -= 1;
            readers += 1;
            numReaders = readers;
        pthread_mutex_unlock(&m);

        // Read data
        fprintf(stdout, "[r%d] reading %u  [readers: %2d]\n", id, sharedValue, numReaders);

        // Exit critical section
        pthread_mutex_lock(&m);
            readers -= 1;
            if (readers == 0) {
                pthread_cond_signal(&c_write);
            }
        pthread_mutex_unlock(&m);
    }

    pthread_exit(0);
}

void *writeMain(void *threadArgument) {
    int id = *((int*)threadArgument);
    int i = 0, numReaders = 0;

    for (int i = 0; i < WRITE_NUM; i++) {
        usleep(1000 * (random() % READER_NUM + WRITER_NUM));
        pthread_mutex_lock(&m);
            while (readers != 0) {
                pthread_cond_wait(&c_write, &m);
            }
            readers = -1;
            numReaders = readers;
        pthread_mutex_unlock(&m);

        fprintf(stdout, "[w%d] writing %u* [readers: %2d]\n", id, ++sharedValue, numReaders);

        pthread_mutex_lock(&m);
            readers = 0;
            if (waitingReadersNum > 0) {
                pthread_cond_broadcast(&c_read);
            } else {
                pthread_cond_signal(&c_write);
            }
        pthread_mutex_unlock(&m);
    }

    pthread_exit(0);
}