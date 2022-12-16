#include "queue.c"
#include "linkedlist.c"
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
int giftID = 0;
int taskID = 0;
int currentTime = 0;         // keeping track of time

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
//A list to keep track of both QA and Assembly for type 4, QA and Painting for type 5.
List *waiting_for_packaging;

// mutex declarations
pthread_mutex_t mtxGiftCount;
pthread_mutex_t mtxTaskCount;
pthread_mutex_t mtxPainting;
pthread_mutex_t mtxPackaging;
pthread_mutex_t mtxAssembly;
pthread_mutex_t mtxQa;
pthread_mutex_t mtxDelivery;
pthread_mutex_t mtxWaiting;
pthread_mutex_t mtxTime;

// our function declarations
int getGiftType();
void addGiftToQueues(int giftType, int *giftID, int *taskID);
void printTask(Task *t);
void updateTime(int sec);
int checkTime();

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
    waiting_for_packaging = ConstructList(1000);

    // initialize mutexes
    pthread_mutex_init(&mtxGiftCount, NULL);
    pthread_mutex_init(&mtxTaskCount, NULL);
    pthread_mutex_init(&mtxPainting, NULL);
    pthread_mutex_init(&mtxPackaging, NULL);
    pthread_mutex_init(&mtxAssembly, NULL);
    pthread_mutex_init(&mtxQa, NULL);
    pthread_mutex_init(&mtxDelivery, NULL);
    pthread_mutex_init(&mtxWaiting, NULL);
    pthread_mutex_init(&mtxTime, NULL);
    srand(seed); // feed the seed

    pthread_t threads[NUM_THREADS];

    pthread_create(&threads[0], NULL, ControlThread, NULL);
    pthread_create(&threads[1], NULL, ElfA, NULL);
    pthread_create(&threads[2], NULL, ElfB, NULL);
    pthread_create(&threads[3], NULL, Santa, NULL);

    /* Last thing that main() should do */
    pthread_exit(NULL);
    DestructQueue(painting);
    DestructQueue(packaging);
    DestructQueue(assembly);
    DestructQueue(qa);
    DestructQueue(delivery);
    DestructList(waiting_for_packaging);

    return 0;
}

void *ElfA()
{ // the one that can paint
    //int now = checkTime();
    //while (now <= simulationTime)
    for (int i = 0; i < simulationTime; i++)
    {
        pthread_mutex_lock(&mtxWaiting);
        int gID = FindReady(waiting_for_packaging);
        if(gID != -1)
        {
            Task t;
            node* current = FindID(waiting_for_packaging, gID);
            Gift g = current->data;
            t.taskType = 'C';
            t.giftID = gID;
            t.giftType = g.type;
            Delete(waiting_for_packaging, gID);
            pthread_mutex_unlock(&mtxWaiting);
            pthread_mutex_lock(&mtxTaskCount);
            taskID++;
            t.taskID = taskID;
            pthread_mutex_unlock(&mtxTaskCount);
            t.responsible = 'A';
            printTask(&t);
            pthread_sleep(PACKAGING_TIME);
            updateTime(PACKAGING_TIME);
        }
        else
        {
            pthread_mutex_unlock(&mtxWaiting);
            pthread_mutex_lock(&mtxPackaging);
            if (!isEmpty(packaging))
            {
                Task t = Dequeue(packaging);
                t.responsible = 'A';
                printTask(&t);
                pthread_mutex_unlock(&mtxPackaging);
                pthread_sleep(PACKAGING_TIME);
                updateTime(PACKAGING_TIME);
                // Add the next task to delivery queue
                pthread_mutex_lock(&mtxTaskCount);
                taskID++;
                t.taskID = taskID;
                pthread_mutex_unlock(&mtxTaskCount);
                t.taskType = 'D';
                pthread_mutex_lock(&mtxDelivery);
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
                    t.responsible = 'A';
                    printTask(&t);
                    pthread_mutex_unlock(&mtxPainting);
                    pthread_sleep(PAINTING_TIME);
                    updateTime(PAINTING_TIME);
                    if(t.giftType == 4)
                    {
                        pthread_mutex_lock(&mtxWaiting);
                        node* gift = FindID(waiting_for_packaging, t.giftID);
                        if(gift == NULL)
                        {
                            Gift g;
                            g.type = t.giftType;
                            g.ID = t.giftID;
                            g.painting = 1;
                            Add(waiting_for_packaging, g);
                        }
                        else
                        {
                            gift->data.painting = 1;
                        }
                        pthread_mutex_unlock(&mtxWaiting);
                    }
                    else
                    {
                        // Add the same task to queue of packaging
                        pthread_mutex_lock(&mtxTaskCount);
                        taskID++;
                        t.taskID = taskID;
                        pthread_mutex_unlock(&mtxTaskCount);
                        t.taskType = 'C';
                        pthread_mutex_lock(&mtxPackaging);
                        Enqueue(packaging, t);
                        pthread_mutex_unlock(&mtxPackaging);
                    }                
            }
            else
            {
                pthread_mutex_unlock(&mtxPainting);
                pthread_sleep(NORMAL_WAITING_TIME);
                updateTime(NORMAL_WAITING_TIME);
            }
        }
            
        }
    }
    pthread_exit(NULL);
}

void *ElfB()
{ // the one that can assemble

    //int now = checkTime();
    //while (now <= simulationTime)
    for (int i = 0; i < simulationTime; i++)
    {
        pthread_mutex_lock(&mtxWaiting);
        int gID = FindReady(waiting_for_packaging);
        if(gID != -1)
        {
            Task t;
            node* current = FindID(waiting_for_packaging, gID);
            Gift g = current->data;
            t.taskType = 'C';
            t.giftID = g.ID;
            t.giftType = g.type;
            Delete(waiting_for_packaging, gID);
            pthread_mutex_unlock(&mtxWaiting);
            pthread_mutex_lock(&mtxTaskCount);
            taskID++;
            t.taskID = taskID;
            pthread_mutex_unlock(&mtxTaskCount);
            t.responsible = 'B';
            printTask(&t);
            pthread_sleep(PACKAGING_TIME);
            updateTime(PACKAGING_TIME);
        }
        else
        {
            pthread_mutex_unlock(&mtxWaiting);
            pthread_mutex_lock(&mtxPackaging);
            if (!isEmpty(packaging))
            {
                Task t = Dequeue(packaging);
                t.responsible = 'B';
                printTask(&t);
                pthread_mutex_unlock(&mtxPackaging);
                pthread_sleep(PACKAGING_TIME);
                updateTime(PACKAGING_TIME);
                // Add the same task to delivery queue
                pthread_mutex_lock(&mtxTaskCount);
                taskID++;
                t.taskID = taskID;
                pthread_mutex_unlock(&mtxTaskCount);
                t.taskType = 'D';
                pthread_mutex_lock(&mtxDelivery);
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
                    t.responsible = 'B';
                    printTask(&t);
                    pthread_mutex_unlock(&mtxAssembly);
                    pthread_sleep(ASSEMBLY_TIME);
                    updateTime(ASSEMBLY_TIME);
                    if(t.giftType == 5)
                    {
                        pthread_mutex_lock(&mtxWaiting);
                        node* gift = FindID(waiting_for_packaging, t.giftID);
                        if(gift == NULL)
                        {
                            Gift g;
                            g.type = t.giftType;
                            g.ID = t.giftID;
                            g.assembly = 1;
                            Add(waiting_for_packaging, g);
                        }
                        else
                        {
                            gift->data.assembly = 1;
                        }
                        pthread_mutex_unlock(&mtxWaiting);
                    }
                    else
                    {
                        // Add the same task to queue of packaging
                        pthread_mutex_lock(&mtxTaskCount);
                        taskID++;
                        t.taskID = taskID;
                        pthread_mutex_unlock(&mtxTaskCount);
                        t.taskType = 'C';
                        pthread_mutex_lock(&mtxPackaging);
                        Enqueue(packaging, t);
                        pthread_mutex_unlock(&mtxPackaging);
                    }
                }
                else
                {
                    pthread_mutex_unlock(&mtxAssembly);
                    pthread_sleep(NORMAL_WAITING_TIME);
                    updateTime(NORMAL_WAITING_TIME);
                }
            }
        }
        
    }
    pthread_exit(NULL);
}

// manages Santa's tasks
void *Santa()
{
    //int now = checkTime();
    //while(now <= simulationTime)
    for (int i = 0; i < simulationTime; i++)
    {

        pthread_mutex_lock(&mtxDelivery);
        if (!isEmpty(delivery))
        {
            Task t = Dequeue(delivery);
            t.responsible = 'S';
            printTask(&t);
            pthread_mutex_unlock(&mtxDelivery);
            pthread_sleep(DELIVERY_TIME);
            updateTime(DELIVERY_TIME);
        }
        else
        {
            pthread_mutex_unlock(&mtxDelivery);
            pthread_mutex_lock(&mtxQa);
            if (!isEmpty(qa))
            {
                Task t = Dequeue(qa);
                t.responsible = 'S';
                printTask(&t);
                pthread_mutex_unlock(&mtxQa);
                pthread_sleep(QA_TIME);
                updateTime(QA_TIME);
                if(t.giftType == 5 || t.giftType == 4)
                {
                    pthread_mutex_lock(&mtxWaiting);
                    node* gift = FindID(waiting_for_packaging, t.giftID);
                    if(gift == NULL)
                    {
                        Gift g;
                        g.type = t.giftType;
                        g.ID = t.giftID;
                        g.qa = 1;
                        Add(waiting_for_packaging, g);
                    }
                    else
                    {
                        gift->data.qa = 1;
                    }
                    pthread_mutex_unlock(&mtxWaiting);
                }
                else
                {
                    pthread_mutex_unlock(&mtxQa);
                    pthread_mutex_lock(&mtxTaskCount);
                    taskID++;
                    t.taskID = taskID;
                    pthread_mutex_unlock(&mtxTaskCount);
                    t.taskType = 'D';
                    pthread_mutex_lock(&mtxDelivery);
                    Enqueue(delivery, t);
                    pthread_mutex_unlock(&mtxDelivery);
                }
            }
            else
            {
                pthread_mutex_unlock(&mtxQa);
                pthread_sleep(NORMAL_WAITING_TIME);
                updateTime(NORMAL_WAITING_TIME);
            }
        }
    }
    pthread_exit(NULL);
}

// the function that controls queues and output
void *ControlThread()
{ // handles printing and queues (up to you)

    for (int i = 0; i < simulationTime; i++)
    {
        int giftType = getGiftType();
        if (giftType != -1)
        {
            Task *t = (Task *)malloc(sizeof(Task));
            pthread_mutex_lock(&mtxGiftCount);
            giftID++;
            t->giftID = giftID;
            pthread_mutex_unlock(&mtxGiftCount);
            pthread_mutex_lock(&mtxTaskCount);
            taskID++;
            t->taskID = taskID;
            pthread_mutex_unlock(&mtxTaskCount);
            t->giftType = giftType;
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
                pthread_mutex_lock(&mtxTaskCount);
                taskID++;
                t->taskID = taskID;
                pthread_mutex_unlock(&mtxTaskCount);
                //queueing the second task
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
                pthread_mutex_lock(&mtxTaskCount);
                taskID++;
                t->taskID = taskID;
                pthread_mutex_unlock(&mtxTaskCount);
                //queueing the second task
                t->taskType = 'Q';
                pthread_mutex_lock(&mtxQa);
                Enqueue(qa, *t);
                pthread_mutex_unlock(&mtxQa);
                break;
            default:
                break;
            }
            free(t);
        }
        pthread_sleep(NORMAL_WAITING_TIME);
        updateTime(NORMAL_WAITING_TIME);        
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
    printf("Task ID: %d, Gift ID: %d, Gift Type: %d, Task Type: %c, Responsible: %c\n", t->taskID, t->giftID, t->giftType, t->taskType, t->responsible);
}

void updateTime(int sec)
{
    pthread_mutex_lock(&mtxTime);
    currentTime += sec;
    pthread_mutex_unlock(&mtxTime);
}

int checkTime()
{
    int ret;
    pthread_mutex_lock(&mtxTime);
    ret = currentTime;
    pthread_mutex_unlock(&mtxTime);
    return ret;
}
