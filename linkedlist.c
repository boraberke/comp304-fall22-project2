// implementation of linked list
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE  1
#define FALSE 0

typedef struct {
    int ID;
    int type;
    int painting;
    int assembly;
    int qa;
    int packageTime;
    int giftTime;
    int newZealand;
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
void Delete(List *pList, int ID);
void DeleteFirst(List *pList);
int isListEmpty(List *pList);
node* FindID(List *pList, int ID);
int FindReady(List *pList);
int WaitingQA(List *pList);

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

void DestructList(List *pList) {
    node* current = pList->head;
    node* next;
    while (!isListEmpty(pList)) {
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
void Delete(List *pList, int ID) {

    if(isListEmpty(pList)) {
        return;
    }
    //navigate through list
    node* current = FindID(pList, ID);
    //found a match, update the link
    if(current->data.ID == pList->head->data.ID) {
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
    else if(current->data.ID == pList->tail->data.ID) 
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

    if(isListEmpty(pList)) {
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

int isListEmpty(List* pList) {
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
node* FindID(List *pList, int ID) {

    //start from the first link
    node* current = pList->head;

    //if list is empty
    if(current == NULL) {
        return NULL;
    }
    int current_ID = current->data.ID;
    //navigate through list
    while(current_ID != ID) {

        //if it is last node
        if(current->next == NULL) {
            return NULL;
        } else {
            //go to next link
            current_ID = current->next->data.ID;
            current = current->next;
        }
    }
    //if data found, return the current Link
    return current;
}

//gives priority to newZealand gifts
int FindReady(List *pList) {
    //start from the first link
    node* current = pList->head;
    Gift g;
    //if list is empty
    if(current == NULL) {
        return -1;
    }
    int giftType, current_ID, newZealand;
    int smallest_nonZealand = pList->limit;
    //navigate through list
    while(current != NULL) {
        g = current->data;
        giftType = g.type;
        current_ID = g.ID;
        newZealand = g.newZealand;
        if(giftType == 4){
            if(g.painting == 1 && g.qa == 1){
                // if a ready New Zealand gift is found return the ID
                if(newZealand)
                {
                    return current_ID;
                }
                else
                {
                    // return the ready gift that has been put the earliest
                    if (current_ID <= smallest_nonZealand)
                    {
                        smallest_nonZealand = current_ID;
                    }
                }
            }
        }
        else if(giftType == 5){
            if(g.assembly == 1 && g.qa == 1){
                // if a ready New Zealand gift is found return the ID
                if(newZealand)
                {
                    return current_ID;
                }
                else
                {
                    // return the ready gift that has been put the earliest
                    if (current_ID <= smallest_nonZealand)
                    {
                        smallest_nonZealand = current_ID;
                    }
                }
            }
        }               
        current = current->next;
    }
    //if no gift is found return -1
    if (smallest_nonZealand == pList->limit)
    {
        return -1;
    }
    else
    //if newZealand gift is not found but another ready gift is found return a ready non-New Zealand gift
    {
        return smallest_nonZealand;
    }
}

int WaitingQA(List *pList)
{
    int ret = 0;
    //start from the first link
    node* current = pList->head;
    Gift g;
    //if list is empty
    if(current == NULL) {
        return ret;
    }
    int giftType, current_ID;
    //navigate through list
    while(current != NULL) {
        g = current->data;
        giftType = g.type;
        current_ID = g.ID;
        if(giftType == 4){
            if(g.qa == 0){
                ret++;
            }
        }
        else if(giftType == 5){
            if(g.qa == 0){
                ret++;
            }
        }
        current = current->next;
    }
    return ret;
}

void printList(List* pList){
    node *current = pList->head;
    while(current!=NULL){
        printf("Gift ID: %d\n",current->data.ID);
        current = current->next;
    }
}
