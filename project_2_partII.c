#include "queue.c"
#include "linkedlist.c"
#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

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
time_t start_time;

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

// our function declarations
int getGiftType();
void addGiftToQueues(int giftType, int *giftID, int *taskID);
void logHeaders();
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
    waiting_for_packaging = ConstructList(1000);

    //enter the header logs.
    logHeaders();

    // initialize mutexes
    pthread_mutex_init(&mtxGiftCount, NULL);
    pthread_mutex_init(&mtxTaskCount, NULL);
    pthread_mutex_init(&mtxPainting, NULL);
    pthread_mutex_init(&mtxPackaging, NULL);
    pthread_mutex_init(&mtxAssembly, NULL);
    pthread_mutex_init(&mtxQa, NULL);
    pthread_mutex_init(&mtxDelivery, NULL);
    pthread_mutex_init(&mtxWaiting, NULL);
    srand(seed); // feed the seed

    pthread_t threads[NUM_THREADS];
    start_time = time(NULL);
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
    time_t seconds = time(NULL);
    while (seconds < start_time + simulationTime)
    {
        pthread_mutex_lock(&mtxWaiting);
        int gID = FindReady(waiting_for_packaging);
        // checking if any type 4 or 5 might be waiting in the linked list
        if(gID != -1)
        {
            // if found create the task object to keep log and delete that gift from the list.
            Task t;
            node* current = FindID(waiting_for_packaging, gID);
            Gift g = current->data;
            t.taskType = 'C';
            t.giftID = gID;
            t.giftType = g.type;
            t.taskTime = g.packageTime;
            t.giftTime = g.giftTime;
            Delete(waiting_for_packaging, gID);
            pthread_mutex_unlock(&mtxWaiting);
            pthread_mutex_lock(&mtxTaskCount);
            taskID++;
            t.taskID = taskID;
            pthread_mutex_unlock(&mtxTaskCount);
            t.responsible = 'A';
            pthread_sleep(PACKAGING_TIME);
            t.completionTime = time(NULL) - start_time;
            printTask(&t);
        }
        else
        {
            // if not found look for type 1, 2 or 3 packaging tasks.
            pthread_mutex_unlock(&mtxWaiting);
            pthread_mutex_lock(&mtxPackaging);
            if (!isEmpty(packaging))
            {
                Task t = Dequeue(packaging);
                pthread_mutex_unlock(&mtxPackaging);
                t.responsible = 'A';
                pthread_sleep(PACKAGING_TIME);
                t.completionTime = time(NULL) - start_time;
                printTask(&t);
                // Add the next task to delivery queue
                pthread_mutex_lock(&mtxTaskCount);
                taskID++;
                t.taskID = taskID;
                pthread_mutex_unlock(&mtxTaskCount);
                t.taskType = 'D';
                t.taskTime = time(NULL) - start_time;
                pthread_mutex_lock(&mtxDelivery);
                Enqueue(delivery, t);
                pthread_mutex_unlock(&mtxDelivery);
                
            }
            else
            {
                // if not found look for painting tasks
                pthread_mutex_unlock(&mtxPackaging);
                pthread_mutex_lock(&mtxPainting);
                if (!isEmpty(painting))
                {
                    Task t = Dequeue(painting);
                    pthread_mutex_unlock(&mtxPainting);
                    t.responsible = 'A';
                    pthread_sleep(PAINTING_TIME);
                    t.completionTime = time(NULL) - start_time;
                    printTask(&t);
                    if(t.giftType == 4)
                    {
                        // if type 4 add that gift information to the linked list or update it if it already exists.
                        pthread_mutex_lock(&mtxWaiting);
                        node* gift = FindID(waiting_for_packaging, t.giftID);
                        if(gift == NULL)
                        {
                            Gift g;
                            g.type = t.giftType;
                            g.ID = t.giftID;
                            g.giftTime = t.giftTime;
                            g.painting = 1;
                            Add(waiting_for_packaging, g);
                        }
                        else
                        {
                            gift->data.painting = 1;
                            gift->data.packageTime = time(NULL) - start_time;
                        }
                        pthread_mutex_unlock(&mtxWaiting);
                    }
                    else
                    {
                        // Add the next task to queue of packaging
                        pthread_mutex_lock(&mtxTaskCount);
                        taskID++;
                        t.taskID = taskID;
                        pthread_mutex_unlock(&mtxTaskCount);
                        t.taskType = 'C';
                        t.taskTime = time(NULL) - start_time;
                        pthread_mutex_lock(&mtxPackaging);
                        Enqueue(packaging, t);
                        pthread_mutex_unlock(&mtxPackaging);
                    }                
                }
                else
                {
                    pthread_mutex_unlock(&mtxPainting);
                    pthread_sleep(NORMAL_WAITING_TIME);
                }
            }
        }
        time(&seconds);
    }
    pthread_exit(NULL);
}

void *ElfB()
{ // the one that can assemble
    time_t seconds = time(NULL);
    while (seconds < start_time + simulationTime)
    {
        pthread_mutex_lock(&mtxWaiting);
        int gID = FindReady(waiting_for_packaging);
        // checking if any type 4 or 5 might be waiting in the linked list
        if(gID != -1)
        {
            // if found create the task object to keep log and delete that gift from the list.
            Task t;
            node* current = FindID(waiting_for_packaging, gID);
            Gift g = current->data;
            t.taskType = 'C';
            t.giftID = g.ID;
            t.giftType = g.type;
            t.taskTime = g.packageTime;
            t.giftTime = g.giftTime;
            Delete(waiting_for_packaging, gID);
            pthread_mutex_unlock(&mtxWaiting);
            pthread_mutex_lock(&mtxTaskCount);
            taskID++;
            t.taskID = taskID;
            pthread_mutex_unlock(&mtxTaskCount);
            t.responsible = 'B';
            pthread_sleep(PACKAGING_TIME);
            t.completionTime = time(NULL) - start_time;
            printTask(&t);
        }
        else
        {
            // if not found look for type 1, 2 or 3 packaging tasks.
            pthread_mutex_unlock(&mtxWaiting);
            pthread_mutex_lock(&mtxPackaging);
            if (!isEmpty(packaging))
            {
                Task t = Dequeue(packaging);
                pthread_mutex_unlock(&mtxPackaging);
                t.responsible = 'B';
                pthread_sleep(PACKAGING_TIME);
                t.completionTime = time(NULL) - start_time;
                printTask(&t);
                // Add the next task to delivery queue
                pthread_mutex_lock(&mtxTaskCount);
                taskID++;
                t.taskID = taskID;
                pthread_mutex_unlock(&mtxTaskCount);
                t.taskType = 'D';
                t.taskTime = time(NULL) - start_time;
                pthread_mutex_lock(&mtxDelivery);
                Enqueue(delivery, t);
                pthread_mutex_unlock(&mtxDelivery);
            }
            else
            {
                // if not found look for painting tasks
                pthread_mutex_unlock(&mtxPackaging);
                pthread_mutex_lock(&mtxAssembly);
                if (!isEmpty(assembly))
                {
                    Task t = Dequeue(assembly);
                    pthread_mutex_unlock(&mtxAssembly);
                    t.responsible = 'B';
                    pthread_sleep(ASSEMBLY_TIME);
                    t.completionTime = time(NULL) - start_time;
                    printTask(&t);
                    if(t.giftType == 5)
                    {
                        // if type 5 add that gift information to the linked list or update it if it already exists.
                        pthread_mutex_lock(&mtxWaiting);
                        node* gift = FindID(waiting_for_packaging, t.giftID);
                        if(gift == NULL)
                        {
                            Gift g;
                            g.type = t.giftType;
                            g.ID = t.giftID;
                            g.giftTime = t.giftTime;
                            g.assembly = 1;
                            Add(waiting_for_packaging, g);
                        }
                        else
                        {
                            gift->data.assembly = 1;
                            gift->data.packageTime = time(NULL) - start_time;
                        }
                        pthread_mutex_unlock(&mtxWaiting);
                    }
                    else
                    {
                        // Add the next task to queue of packaging
                        pthread_mutex_lock(&mtxTaskCount);
                        taskID++;
                        t.taskID = taskID;
                        pthread_mutex_unlock(&mtxTaskCount);
                        t.taskType = 'C';
                        t.taskTime = time(NULL) - start_time;
                        pthread_mutex_lock(&mtxPackaging);
                        Enqueue(packaging, t);
                        pthread_mutex_unlock(&mtxPackaging);
                    }
                }
                else
                {
                    pthread_mutex_unlock(&mtxAssembly);
                    pthread_sleep(NORMAL_WAITING_TIME);
                }
            }
        }
        time(&seconds);
    }
    pthread_exit(NULL);
}

// manages Santa's tasks
void *Santa()
{
    time_t seconds = time(NULL);
    while(seconds < start_time + simulationTime)
    {
        //priority for delivery tasks
        pthread_mutex_lock(&mtxWaiting);
        int qa_waiting = WaitingQA(waiting_for_packaging);
        pthread_mutex_unlock(&mtxWaiting);
        pthread_mutex_lock(&mtxDelivery);
        if (!(isEmpty(delivery) || qa_waiting >= 3))
        {
            Task t = Dequeue(delivery);
            pthread_mutex_unlock(&mtxDelivery);
            t.responsible = 'S';
            pthread_sleep(DELIVERY_TIME);
            t.completionTime = time(NULL) - start_time;
            printTask(&t);
        }
        else
        {
            // if not found look for QA tasks
            pthread_mutex_unlock(&mtxDelivery);
            pthread_mutex_lock(&mtxQa);
            if (!isEmpty(qa))
            {
                Task t = Dequeue(qa);
                pthread_mutex_unlock(&mtxQa);
                t.responsible = 'S';
                pthread_sleep(QA_TIME);
                t.completionTime = time(NULL) - start_time;
                printTask(&t);
                if(t.giftType == 5 || t.giftType == 4)
                {
                    // if type 4 or 5 add that gift information to the linked list or update it if it already exists.
                    pthread_mutex_lock(&mtxWaiting);
                    node* gift = FindID(waiting_for_packaging, t.giftID);
                    if(gift == NULL)
                    {
                        Gift g;
                        g.type = t.giftType;
                        g.ID = t.giftID;
                        g.giftTime = t.giftTime;
                        g.qa = 1;
                        Add(waiting_for_packaging, g);
                    }
                    else
                    {
                        gift->data.qa = 1;
                        gift->data.packageTime = time(NULL) - start_time;
                    }
                    pthread_mutex_unlock(&mtxWaiting);
                }
                else
                {
                    // Add the next task to queue of packaging
                    pthread_mutex_unlock(&mtxQa);
                    pthread_mutex_lock(&mtxTaskCount);
                    taskID++;
                    t.taskID = taskID;
                    pthread_mutex_unlock(&mtxTaskCount);
                    t.taskType = 'D';
                    t.taskTime = time(NULL) - start_time;
                    pthread_mutex_lock(&mtxDelivery);
                    Enqueue(delivery, t);
                    pthread_mutex_unlock(&mtxDelivery);
                }
            }
            else
            {
                pthread_mutex_unlock(&mtxQa);
                pthread_sleep(NORMAL_WAITING_TIME);
            }
        }
        time(&seconds); 
    }
    pthread_exit(NULL);
}

// the function that controls queues and output
void *ControlThread()
{ // handles printing and queues (up to you)

    time_t seconds = time(NULL);
    while(seconds < start_time + simulationTime)
    {
        // lock all of the queues and print them:
        int now  = time(NULL) - start_time;
        pthread_mutex_lock(&mtxPainting);
        printQueue(painting,"painting",now);
        pthread_mutex_unlock(&mtxPainting);

        pthread_mutex_lock(&mtxPackaging);
        printQueue(packaging,"packaging",now);
        pthread_mutex_unlock(&mtxPackaging);

        pthread_mutex_lock(&mtxAssembly);
        printQueue(assembly,"assembly",now);
        pthread_mutex_unlock(&mtxAssembly);

        pthread_mutex_lock(&mtxQa);
        printQueue(qa,"qa",now);
        pthread_mutex_unlock(&mtxQa);

        pthread_mutex_lock(&mtxDelivery);
        printQueue(delivery,"delivery",now);
        printf("---------------------------\n");
        pthread_mutex_unlock(&mtxDelivery);


        int giftType = getGiftType();
        if (giftType != -1)
        {
            Task *t = (Task *)malloc(sizeof(Task));
            t->giftTime = time(NULL) - start_time;
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
                t->taskType = 'C';
                pthread_mutex_lock(&mtxPackaging);
                t->taskTime = time(NULL) - start_time;
                Enqueue(packaging, *t);
                pthread_mutex_unlock(&mtxPackaging);
                break;
            case 2:
                t->taskType = 'P';
                pthread_mutex_lock(&mtxPainting);
                t->taskTime = time(NULL) - start_time;
                Enqueue(painting, *t);
                pthread_mutex_unlock(&mtxPainting);
                break;
            case 3:
                t->taskType = 'A';
                pthread_mutex_lock(&mtxAssembly);
                t->taskTime = time(NULL) - start_time;
                Enqueue(assembly, *t);
                pthread_mutex_unlock(&mtxAssembly);
                break;
            case 4:
                t->taskType = 'P';
                pthread_mutex_lock(&mtxPainting);
                t->taskTime = time(NULL) - start_time;
                Enqueue(painting, *t);
                pthread_mutex_unlock(&mtxPainting);
                pthread_mutex_lock(&mtxTaskCount);
                taskID++;
                t->taskID = taskID;
                pthread_mutex_unlock(&mtxTaskCount);
                //queueing the second task
                t->taskType = 'Q';
                pthread_mutex_lock(&mtxQa);
                t->taskTime = time(NULL) - start_time;
                Enqueue(qa, *t);
                pthread_mutex_unlock(&mtxQa);
                break;
            case 5:
                t->taskType = 'A';
                pthread_mutex_lock(&mtxAssembly);
                t->taskTime = time(NULL) - start_time;
                Enqueue(assembly, *t);
                pthread_mutex_unlock(&mtxAssembly);
                pthread_mutex_lock(&mtxTaskCount);
                taskID++;
                t->taskID = taskID;
                pthread_mutex_unlock(&mtxTaskCount);
                //queueing the second task
                t->taskType = 'Q';
                pthread_mutex_lock(&mtxQa);
                t->taskTime = time(NULL) - start_time;
                Enqueue(qa, *t);
                pthread_mutex_unlock(&mtxQa);
                break;
            default:
                break;
            }
            free(t);
        }
        pthread_sleep(NORMAL_WAITING_TIME);
        time(&seconds);   
    }
    pthread_exit(NULL);
}

int getGiftType()
{
    int probability = rand() % 100;
    if (probability <= 50)
    {
        return 1;
    }
    else if (probability > 50 && probability <= 70)
    {
        return 2;
    }
    else if (probability > 70 && probability <= 90)
    {
        return 3;
    }
    else if (probability > 90 && probability <= 95)
    {
        return 4;
    }
    else if (probability > 95 && probability <= 100)
    {
        return 5;
    }
    else
    {
        return -1;
    }
}
void logHeaders()
{
    const char* log_string = "Task ID    Gift ID      Gift Type   Task Type  Request Time     Task Arrival    TT     Responsible";
    FILE* log_file = fopen("events.log", "w"); // open the log file in write mode
    fprintf(log_file, "%s\n", log_string);  
    fclose(log_file);  
}

void printTask(Task *t)
{
    //printf("Task ID: %d, Gift ID: %d, Gift Type: %d, Task Type: %c, Request Time: %d, Task Arrival: %d, TT: %d, Responsible: %c\n", t->taskID, t->giftID, t->giftType, t->taskType, t->giftTime, t->taskTime, t->completionTime - t->taskTime,t->responsible);
    char log_string[100];  // allocate a character array to hold the formatted string
    snprintf(log_string, sizeof(log_string), "%d             %d             %d           %c           %d                %d           %d          %c", t->taskID, t->giftID, t->giftType, t->taskType, t->giftTime, t->taskTime, t->completionTime - t->taskTime, t->responsible);  // format the string using snprintf
    FILE* log_file = fopen("events.log", "a");  // open the log file in append mode
    fprintf(log_file, "%s\n", log_string);  // print the log string to the file
    fclose(log_file);  // close the file
}


