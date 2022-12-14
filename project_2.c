#include "queue.c"
#include <sys/time.h>
#include <string.h>
#include <pthread.h>

#define NUM_THREADS 4
#define PAINTING_TIME 3
#define ASSEMBLY_TIME 2
#define QA_TIME 1
#define DELIVERY_TIME 1
#define PACKAGING_TIME 1
#define NORMAL_WAITING_TIME 1
int simulationTime = 120;    // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 30; // frequency of emergency gift requests from New Zealand

void *ElfA(); // the one that can paint
void *ElfB(); // the one that can assemble
void *Santa();
void *ControlThread(); // handles printing and queues (up to you)

// queue declarations
Queue *painting;
Queue *packaging;
Queue *assembly;
Queue *qa;
Queue *delivery;
Queue *waiting_for_packaging;

// mutex declarations
pthread_mutex_t mtxPainting;
pthread_mutex_t mtxPackaging;
pthread_mutex_t mtxAssembly;
pthread_mutex_t mtxQa;
pthread_mutex_t mtxDelivery;
pthread_mutex_t mtxWaiting;

// our function declarations
int getGiftType();
void addGiftToQueues(int giftType, int *giftID, int *taskID);
void printTask(Task *t);

// pthread sleeper function
int pthread_sleep(int seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if (pthread_mutex_init(&mutex, NULL))
    {
        return -1;
    }
    if (pthread_cond_init(&conditionvar, NULL))
    {
        return -1;
    }
    struct timeval tp;
    // When to expire is an absolute time, so get the current time and add it to our delay time
    gettimeofday(&tp, NULL);
    timetoexpire.tv_sec = tp.tv_sec + seconds;
    timetoexpire.tv_nsec = tp.tv_usec * 1000;

    pthread_mutex_lock(&mutex);
    int res = pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);

    // Upon successful completion, a value of zero shall be returned
    return res;
}

int main(int argc, char **argv)
{
    // -t (int) => simulation time in seconds
    // -s (int) => change the random seed
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-t"))
        {
            simulationTime = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-s"))
        {
            seed = atoi(argv[++i]);
        }
    }

    // initialize Queues.
    painting = ConstructQueue(1000);
    packaging = ConstructQueue(1000);
    assembly = ConstructQueue(1000);
    qa = ConstructQueue(1000);
    delivery = ConstructQueue(1000);
    waiting_for_packaging = ConstructQueue(1000);

    // initialize mutexes
    pthread_mutex_init(&mtxPainting, NULL);
    pthread_mutex_init(&mtxPackaging, NULL);
    pthread_mutex_init(&mtxAssembly, NULL);
    pthread_mutex_init(&mtxQa, NULL);
    pthread_mutex_init(&mtxDelivery, NULL);
    pthread_mutex_init(&mtxWaiting, NULL);

    srand(seed); // feed the seed

    pthread_t threads[NUM_THREADS];

    pthread_create(&threads[0], NULL, ControlThread, NULL);
    pthread_create(&threads[1], NULL, ElfA, NULL);
    pthread_create(&threads[2], NULL, ElfB, NULL);
    pthread_create(&threads[3], NULL, Santa, NULL);

    /* Last thing that main() should do */
    pthread_exit(NULL);

    return 0;
}

void *ElfA()
{ // the one that can paint

    for (int i = 0; i < simulationTime; i++)
    {

        pthread_mutex_lock(&mtxPackaging);
        if (!isEmpty(packaging))
        {
            Task t = Dequeue(packaging);
            printTask(&t);
            pthread_mutex_unlock(&mtxPackaging);
            pthread_sleep(PACKAGING_TIME);
            // Add the next task to delivery queue
            pthread_mutex_lock(&mtxDelivery);
            // update the task id and maybe also add the responsible elf
            t.taskID = t.taskID + 1;
            t.taskType = 5;
            Enqueue(delivery, t);
            pthread_mutex_unlock(&mtxDelivery);
        }
        else
        {
            pthread_mutex_unlock(&mtxPackaging);
            pthread_mutex_lock(&mtxPainting);
            if (!isEmpty(painting))
            {
                Task t = Dequeue(painting);
                printTask(&t);
                pthread_mutex_unlock(&mtxPainting);
                pthread_sleep(PAINTING_TIME);
                // if type 2 continue like below
                // Add the same task to queue of packaging
                pthread_mutex_lock(&mtxPackaging);
                // todo: Add further information into t ?
                Enqueue(packaging, t);
                pthread_mutex_unlock(&mtxPackaging);
            }
            else
            {
                pthread_mutex_unlock(&mtxPainting);
                pthread_sleep(NORMAL_WAITING_TIME);
            }
        }
    }
    pthread_exit(NULL);
}

void *ElfB()
{ // the one that can assemble

    for (int i = 0; i < simulationTime; i++)
    {

        pthread_mutex_lock(&mtxPackaging);
        if (!isEmpty(packaging))
        {
            Task t = Dequeue(packaging);
            printTask(&t);
            pthread_mutex_unlock(&mtxPackaging);
            pthread_sleep(PACKAGING_TIME);
            // Add the same task to delivery queue
            pthread_mutex_lock(&mtxDelivery);
            t.taskID++;
            Enqueue(delivery, t);
            pthread_mutex_unlock(&mtxDelivery);
        }
        else
        {
            pthread_mutex_unlock(&mtxPackaging);
            pthread_mutex_lock(&mtxAssembly);
            if (!isEmpty(assembly))
            {
                Task t = Dequeue(assembly);
                printTask(&t);
                pthread_mutex_unlock(&mtxAssembly);
                pthread_sleep(ASSEMBLY_TIME);
                // Add the same task to queue of packaging
                pthread_mutex_lock(&mtxPackaging);
                // todo: Add further information into t ?
                Enqueue(packaging, t);
                pthread_mutex_unlock(&mtxPackaging);
            }
            else
            {
                pthread_mutex_unlock(&mtxAssembly);
                pthread_sleep(NORMAL_WAITING_TIME);
            }
        }
    }
    pthread_exit(NULL);
}

// manages Santa's tasks
void *Santa()
{
    for (int i = 0; i < simulationTime; i++)
    {

        pthread_mutex_lock(&mtxDelivery);
        if (!isEmpty(delivery))
        {
            Task t = Dequeue(delivery);
            printTask(&t);
            pthread_mutex_unlock(&mtxDelivery);
            pthread_sleep(DELIVERY_TIME);
        }
        else
        {
            pthread_mutex_unlock(&mtxDelivery);
            pthread_mutex_lock(&mtxQa);
            if (!isEmpty(qa))
            {
                Task t = Dequeue(qa);
                printTask(&t);
                pthread_mutex_unlock(&mtxQa);
                pthread_sleep(QA_TIME);
            }
            else
            {
                pthread_mutex_unlock(&mtxQa);
                pthread_sleep(NORMAL_WAITING_TIME);
            }
        }
    }
    pthread_exit(NULL);
}

// the function that controls queues and output
void *ControlThread()
{ // handles printing and queues (up to you)
    int giftID = 1;
    int taskID = 1;

    for (int i = 0; i < simulationTime; i++)
    {
        int giftType = getGiftType();
        if (giftType != -1)
        {
            Task *t = (Task *)malloc(sizeof(Task));
            t->giftID = giftID;
            t->taskID = taskID;
            GiftType *g = (GiftType *)malloc(sizeof(GiftType));
            g->giftType = giftType;
            g->painting = 0;
            g->assembly = 0;
            g->qa = 0;
            t->giftType = g;
            //Task type: C = packaging, P = painting, A = assembly, Q = QA, D = delivery
            switch (giftType)
            {
            case 1:
                pthread_mutex_lock(&mtxPackaging);
                t->taskType = 'C';
                Enqueue(packaging, *t);
                pthread_mutex_unlock(&mtxPackaging);
                break;
            case 2:
                pthread_mutex_lock(&mtxPainting);
                t->taskType = 'P';
                Enqueue(painting, *t);
                pthread_mutex_unlock(&mtxPainting);
                break;
            case 3:
                pthread_mutex_lock(&mtxAssembly);
                t->taskType = 'A';
                Enqueue(assembly, *t);
                pthread_mutex_unlock(&mtxAssembly);
                break;
            case 4:
                pthread_mutex_lock(&mtxPainting);
                t->taskType = 'P';
                Enqueue(painting, *t);
                pthread_mutex_unlock(&mtxPainting);
                taskID++;
                //queueing the second task
                t->taskID = taskID;
                t->taskType = 'Q';
                pthread_mutex_lock(&mtxQa);
                Enqueue(qa, *t);
                pthread_mutex_unlock(&mtxQa);
                break;
            case 5:
                pthread_mutex_lock(&mtxAssembly);
                t->taskType = 'A';
                Enqueue(assembly, *t);
                pthread_mutex_unlock(&mtxAssembly);
                taskID++;
                //queueing the second task
                t->taskID = taskID;
                t->taskType = 'Q';
                pthread_mutex_lock(&mtxQa);
                Enqueue(qa, *t);
                pthread_mutex_unlock(&mtxQa);
                break;
            default:
                break;
            }
            giftID++;
            taskID++;
            free(t);
            //DONT FORGET TO FREE GIFTTYPES
        }
        pthread_sleep(NORMAL_WAITING_TIME);
        
    }
    pthread_exit(NULL);
}

int getGiftType()
{
    int probability = rand() % 100;
    if (probability <= 40)
    {
        return 1;
    }
    else if (probability > 40 && probability <= 60)
    {
        return 2;
    }
    else if (probability > 60 && probability <= 80)
    {
        return 3;
    }
    else if (probability > 80 && probability <= 85)
    {
        return 4;
    }
    else if (probability > 85 && probability <= 90)
    {
        return 5;
    }
    else
    {
        return -1;
    }
}

void printTask(Task *t)
{
    printf("Task ID: %d, Gift ID: %d, Gift Type: %d, Task Type: %c\n", t->taskID, t->giftID, t->giftType->giftType, t->taskType);
}
