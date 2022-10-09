/**************************************************************
* Class:  CSC-415-01 fall 2022
* Name:Leo Powers
* Student ID:921661426
* GitHub ID:powersleo
* Project: Assignment 4 – Word Blast
*
* File: Powers_Leo_HW4_main.c
*
* Description:This program uses multithreading to process the text from a given file.
*
**************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

// You may find this Useful
char *delim = "\"\'.“”‘’?:;-,—*($%)! \t\n\x0A\r";

struct node
{
    char *wordPtr;
    int count;
    struct node* nextNode;
};

struct node *head;
struct node *currNode;
int nodeCount, file, init;
pthread_mutex_t mutex;

//deallocate the values of linked list
void deallocateList(){
    struct node * currNode = head;
    struct node * prevNode = NULL;
    while(currNode->nextNode != NULL){
        free(currNode->wordPtr);
        prevNode = currNode;
        currNode = currNode->nextNode;
        free(prevNode);
    }
}


//bubble sort function from GeeksforGeeks
void sort(){
    struct node* topTen[10];
    struct node * headPtr = head;
    struct node * temp;

    int wordInArray = 0;
    //initialize the top ten array
    for(int i = 0; i < 10; i++){
        topTen[i] = NULL;
    }
    //iterate through the next node
    while(headPtr->nextNode != NULL){
        
        wordInArray = 0;
        for(int i = 0; i < 10; i++){
            // printf("%d\n", i);
            if(topTen[i] == NULL){
                topTen[i] = headPtr;
            } else if(headPtr->count > topTen[i]->count){
                for(int j = 0; j < 10; j++){
                    if(strcasecmp(topTen[j]->wordPtr, headPtr->wordPtr) == 0){
                        wordInArray = 1;
                    }
                }
                //if word is not in array already
                //push values to right
                if(wordInArray == 0){
                    temp = topTen[i];
                    for(int j = 9; j -1> i; j--){
                        topTen[j] = topTen[j -1];
                    }
                    topTen[i+1] = temp;
                    topTen[i] =headPtr;
                }
            }
        }
        headPtr = headPtr->nextNode;
    }
    //print out top ten words
    for(int i = 0; i < 10; i++){
        printf("Number %d is %s with a count of \t%d\n", i+1,topTen[i]->wordPtr,topTen[i]->count);
    }
}

//used to pass arguments into the thread
struct args{
    int bytePerThread;
    int threadNum;
};


//used to link linked list nodes
void linkNewNode(char word[]){
    int compareFlag = 1;
    //used to create a new head
    //should be safe for threads
    if(init == 0 && head == NULL){
        //set status if init occurs
        init = 1;
        struct node *newNode;
        newNode = malloc(sizeof(struct node));
        newNode->count = 1;
        newNode->wordPtr = malloc(strlen(word));
        strcpy(newNode->wordPtr, word);
        //critical section
        pthread_mutex_lock(&mutex);
        head = newNode;
        currNode = newNode;  
        nodeCount = 1;
        pthread_mutex_unlock(&mutex);
        //critical section
    } else {
        struct node * searchNode = head;
        while(searchNode->nextNode != NULL){
            compareFlag = strcasecmp(word, searchNode->wordPtr);
            //if a comparison is found update the count
            if(compareFlag == 0 ){
                //critical section
                pthread_mutex_lock(&mutex);
                searchNode->count += 1; 
                pthread_mutex_unlock(&mutex);
                //critical section
                break;       
            }else{
                searchNode = searchNode->nextNode;
            }
        }
        //if complare flag finds no match add new node to the linked list
        if(compareFlag != 0){
            struct node *newNode;
            newNode = malloc(sizeof(struct node));
            //if malloc was successful
            if(newNode != NULL){
                newNode -> count = 1;
                newNode -> wordPtr = malloc(strlen(word));
                    if(newNode -> wordPtr){
                        strcpy(newNode->wordPtr, word);
                        //critical section
                        pthread_mutex_lock(&mutex);
                        currNode->nextNode = newNode;
                        currNode = newNode;
                        nodeCount++;
                        pthread_mutex_unlock(&mutex);
                        //critical section
                    }
                }
        }
    }
}

void *readFile(void * arguments)
{
    //cast the argument structure to read in.
    struct args* argumentStruct = ((struct args*)arguments);
    char * buffer;
    //malloc the buffer with a slight extra space.
    buffer = malloc(argumentStruct->bytePerThread+20);

    //calculate offset of the file
    int offsetPosition = argumentStruct->bytePerThread * argumentStruct->threadNum;
    //removes the overlap(contains some overlap but insignifigant loss of chars)
    int count = argumentStruct->bytePerThread - argumentStruct->threadNum;
    //using read because pread didnt want to work
    int status = read(file, buffer, count);
    char *words;
    while (words = strtok_r(buffer, delim, &buffer))
    {
        //if word is greater than 5
        if(strlen(words) > 5 ){
            linkNewNode(words);
        }
    }
}

int main(int argc, char *argv[])
{
    //***TO DO***  Look at arguments, open file, divide by threads
    //             Allocate and Initialize and storage structures
    int threadCount;
    //check argument count
    if (argc >= 3)
    {
        threadCount = atoi(argv[2]);
        file = open(argv[1], O_RDONLY);
    }else{
        printf("invalid arguments");
        return -1;
    }
    //get length of file.
    int byteLength = lseek(file, (size_t)0, SEEK_END);
    //reset the seek position back to top;
    lseek(file, (size_t)0, SEEK_SET);
    init = 0;
    //threads array
    pthread_t thread[threadCount];
    pthread_mutex_init(&mutex, NULL);

    struct args* threadStruct;
    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    //Time stamp start
    struct timespec startTime;
    struct timespec endTime;

    clock_gettime(CLOCK_REALTIME, &startTime);
    //**************************************************************
    // *** TO DO ***  start your thread processing
    //                wait for the threads to finish
    
    //loop to create threads
    for (int i = 0; i < threadCount; i++){
        //create new struct to pass important variables
        threadStruct = malloc(sizeof(struct args));
        int bytesPerThread = byteLength/threadCount;
        threadStruct->bytePerThread = bytesPerThread;
        threadStruct->threadNum = i;

        if (pthread_create(thread + i, NULL, &readFile, (void*)threadStruct) != 0)
        {
            perror("failed to create thread\n");
            return 1;
        }
    }
    //loop to join threads
    for (int i = 0; i < threadCount; i++)
    {
        if (pthread_join(thread[i], NULL) != 0)
        {
            return 1;
        }
    }
    // printf("%s","finished reading");
    // ***TO DO *** Process TOP 10 and display
    sort();
    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    //Clock output
    clock_gettime(CLOCK_REALTIME, &endTime);
    time_t sec = endTime.tv_sec - startTime.tv_sec;
    long n_sec = endTime.tv_nsec - startTime.tv_nsec;
    if (endTime.tv_nsec < startTime.tv_nsec)
    {
        --sec;
        n_sec = n_sec + 1000000000L;
    }
    printf("Total Time was %ld.%09ld seconds\n", sec, n_sec);
    //**************************************************************
    deallocateList();
    //clean up linked list
    //destroy mutex
    pthread_mutex_destroy(&mutex);

    return 0;
    // ***TO DO *** cleanup
}
