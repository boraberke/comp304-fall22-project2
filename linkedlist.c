// implementation of linked list
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE  1
#define FALSE 0

typedef struct {
    int giftID;
    int giftType;
    int painting;
    int assembly;
    int qa;
} Gift;

typedef struct node {
    Gift data;
    struct node *next;
    struct node *prev;
} node;

typedef struct List {
    node *head;
    node *tail;
    int size;
    int limit;
} List;

List *ConstructList(int limit);
void DestructList(List *pList);
int Add(List *pList, Gift g);
void Delete(List *pList, int giftID);
void DeleteFirst(List *pList);
int isEmpty(List *pList);
node* FindID(List *pList, int giftID);
int FindReady(List *pList);

List *ConstructList(int limit) {
    List *list = (List*) malloc(sizeof (List));
    if (list == NULL) {
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
void DestructList(List *pList) {
    node* current = pList->head;
    node* next;
    while (!isEmpty(pList)) {
        DeleteFirst(pList);
    }
    free(pList);
}
//add an item to the list
int Add(List *pList, Gift g) {
    /* Bad parameter */
    node* item = (node*) malloc(sizeof (node));
    item->data = g;

    if ((pList == NULL) || (item == NULL)) {
        return FALSE;
    }
    
    if (pList->size >= pList->limit) {
        return FALSE;
    }
    /*the list is empty*/
    item->next = NULL;
    if (pList->size == 0) {
        pList->head = item;
        pList->tail = item;

    } else {
        /*adding item to the end of the list*/
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
        return;
    }
    //navigate through list
    node* current = FindID(pList, giftID);
    //found a match, update the link
    if(current->data.giftID == pList->head->data.giftID) {
        //change first to point to next link
        pList->head = pList->head->next;
        if(pList->head == NULL)
        {
            pList->tail = NULL;
        }
        else
        {
            pList->head->prev = NULL;

        }
    }
    else if(current->data.giftID == pList->tail->data.giftID) 
    {
        pList->tail = pList->tail->prev;
        pList->tail->next = NULL;
    }
    else 
    {
        //bypass the current link
        current->prev->next = current->next;
        current->next->prev = current->prev;
    }
    pList->size--;
    free(current);
}  

//delete head of the list
void DeleteFirst(List *pList) {

    if(isEmpty(pList)) {
        return;
    }
    //navigate through list
    node* head = pList->head;

    if(head->next == NULL)
    {
        pList->head = NULL;
        pList->tail = NULL;
    }
    else
    {
        pList->head = pList->head->next;
        pList->head->prev = NULL;

    }
    pList->size--;
    free(head);
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

//find the gift with the given ID
node* FindID(List *pList, int giftID) {

    //start from the first link
    node* current = pList->head;

    //if list is empty
    if(current == NULL) {
        return NULL;
    }
    int current_ID = current->data.giftID;
    //navigate through list
    while(current_ID != giftID) {

        //if it is last node
        if(current->next == NULL) {
            return NULL;
        } else {
            //go to next link
            current_ID = current->next->data.giftID;
            current = current->next;
        }
    }
    //if data found, return the current Link
    return current;
}

 
int FindReady(List *pList) {
    //start from the first link
    node* current = pList->head;
    //if list is empty
    if(current == NULL) {
        return -1;
    }
    Gift g;
    int giftType, current_ID;
    //navigate through list
    while(current != NULL) {
        g = current->data;
        giftType = g.giftType;
        current_ID = g.giftID;
        if(giftType == 4){
            if(g.painting == 1 && g.qa == 1){
                return current_ID;
            }
        }
        else if(giftType == 5){
            if(g.assembly == 1 && g.qa == 1){
                return current_ID;
            }
        }
        current = current->next;
    }
    //if it is not found
    return -1;
}

void printList(List* pList){
    node *current = pList->head;
    while(current!=NULL){
        printf("Gift ID: %d\n",current->data.giftID);
        current = current->next;
    }
}
