// implementation of linked list
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int giftID;
    int giftType;
    int painting;
    int assembly;
    int qa;
} Gift;

struct node {
    Gift *data;
    struct node *next;
    struct node *prev;
} Node;

typedef struct List {
    node *head;
    node *tail;
    int size;
    int limit;
} List;

List *ConstructList(int limit);
void DestructList(List *pList);
int Add(List *pList, Task t);
void Delete(List *pList);
int isEmpty(List *pList);
struct node* find_ID(List *pList, int giftID);

List *ConstructList(int limit) {
    List *list = (List*) malloc(sizeof (List));
    if (queue == NULL) {
        return NULL;
    }
    if (limit <= 0) {
        limit = 65535;
    }
    list->limit = limit;
    list->size = 0;
    list->head = NULL;
    list->tail = NULL;

    return list;
}

//NEEDS TO BE UPDATED
void DestructList(List *list) {
    node *pN;
    while (!isEmpty(list)) {
        delete(list);
    }
    free(list);
}

int Add(List *pList, Gift g) {
    /* Bad parameter */
    node* item = (node*) malloc(sizeof (node));
    item->data = g;

    if ((pList == NULL) || (item == NULL)) {
        return FALSE;
    }
    // if(pQueue->limit != 0)
    if (pList->size >= pList->limit) {
        return FALSE;
    }
    /*the queue is empty*/
    item->next = NULL;
    if (pList->size == 0) {
        pList->head = item;
        pList->tail = item;

    } else {
        /*adding item to the end of the queue*/
        item->prev = pList->tail;
        pList->tail->next = item;
        pList->tail = item;
    }
    pList->size++;
    return TRUE;
}

//delete a link with given key
void Delete(List *pList, int giftID) {

    if(isEmpty(pList)) {
        return NULL;
    }
    //navigate through list
    struct node* current = find(pList, giftID);

    //found a match, update the link
    if(current->data->giftID == pList->head->data->giftID) {
        //change first to point to next link
        pList->head = head->next;
        pList->head->prev = NULL;
    else if(current->data->giftID == pList->tail->data->giftID) {
        pList->tail = tail->prev;
        pList->tail->next = NULL;
    }
    else {
        //bypass the current link
        current->prev->next = current->next;
        current->next->prev = current->prev;
    }
    pList->size--;
    free(current);
}  

int isEmpty(List* pList) {
    if (pList == NULL) {
        return FALSE;
    }
    if (pList->size == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

struct node* find_ID(List *pList, int giftID) {

    //start from the first link
    struct node* current = pList->head;

    //if list is empty
    if(head == NULL) {
        return NULL;
    }
    int current_ID = current->data->giftID;
    //navigate through list
    while(current_ID != giftID) {

        //if it is last node
        if(current->next == NULL) {
            return NULL;
        } else {
            //go to next link
            current_ID = current->next->data->giftID;
        }
    }
    //if data found, return the current Link
    return current;
}

 
int find_ready(List *pList) {

    //start from the first link
    struct node* current = pList->head;

    //if list is empty
    if(head == NULL) {
        return NULL;
    }
    Gift* g;
    int giftType, current_ID;
    //navigate through list
    while(current != NULL) {
        g = current->data;
        giftType = g->giftType;
        current_ID = g->giftID;
        if(giftType == 4){
            if(g->painting == 1 && g->qa == 1){
                break;
            }
            current = current->next;
        }
        else if(giftType == 5){
            if(g->assembly == 1 && g->qa == 1){
                break;
            }
            current = current->next;
        }
    }
    //if it is last node
    if(current->next == NULL) {
        return -1;
    }
    //if data found, return the current Link
    return current_ID;
}
