#include "threadpool.h"
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

void free_all(threadpool * pool);
void freeLinkedList(work_t* headList);
void popOutWork(threadpool * pool);

threadpool *create_threadpool(int num_threads_in_pool) {

    if( num_threads_in_pool > MAXT_IN_POOL || num_threads_in_pool <= 0)
        return NULL;

    threadpool* pool = (threadpool*) malloc(sizeof(threadpool));
    if (pool == NULL){
        return NULL;
    }
    pool->num_threads = num_threads_in_pool;   // initialization
    pool->qsize = 0 ;
    pool->qhead = NULL;
    pool->qtail = NULL;
    pool->shutdown = 0 ;
    pool->dont_accept = 0;

    pthread_mutex_init (&pool->qlock,NULL);    // mutex initialization
    pthread_cond_init (&(pool->q_not_empty),NULL);
    pthread_cond_init (&pool->q_empty,NULL);

    pool->threads = (pthread_t*)malloc( sizeof(pthread_t) * num_threads_in_pool);
    if(pool->threads == NULL)
    {
        free_all(pool);
        return NULL;
    }
    for (int i = 0; i < num_threads_in_pool ; i++)
    {
       int rc = pthread_create(&pool->threads[i], NULL, do_work, (void *)pool);
        if (rc)
        {
            destroy_threadpool(pool);   // destroy all the threads
            return NULL;
        }
    }
    return pool;
}
void *do_work(void *p)
{
    if(p == NULL)
        return NULL;

    threadpool* pool = (threadpool*)p;

    while (1)
    {
      pthread_mutex_lock(&pool->qlock);  // take the first job alone

      if (pool->shutdown == 1)  // if there is shutdown we need to exit the threads
      {
          pthread_mutex_unlock(&(pool->qlock));  // took it we can release the mutex
          return NULL;
      }
      if (pool->qsize == 0 )
          pthread_cond_wait(&(pool->q_not_empty),&(pool->qlock));

      if (pool->shutdown == 1)  // if there is shutdown we need to exit the threads
      {
          pthread_mutex_unlock(&(pool->qlock));  // took it we can release the mutex
          return NULL;
      }
      work_t* work = pool->qhead;
      popOutWork(pool);
      //If the queue becomes empty and destruction process waits to begin (dont_accept=1), signal destruction process.

      if(pool->qsize == 0 && pool->dont_accept == 1)
      {
          //pthread_mutex_unlock(&(pool->qlock));  // took it we can release the mutex
          pthread_cond_signal(&(pool->q_empty));  //we are ready to destroy. wake it up
      }
      pthread_mutex_unlock(&(pool->qlock));  // took it we can release the mutex
      if(work)
      {
          work->routine(work->arg);  // start function of the work.
          free(work); // finish with this work so we can free it
      }
    }
}
/**
 * dispatch enter a "job" of type work_t into the queue.
 * when an available thread takes a job from the queue, it will
 * call the function "dispatch_to_here" with argument "arg".
 * this function should:
 * 1. create and init work_t element
 * 2. lock the mutex
 * 3. add the work_t element to the queue
 * 4. unlock mutex
 * */

void dispatch(threadpool *from_me, dispatch_fn dispatch_to_here, void *arg)
{
    if(from_me == NULL || dispatch_to_here == NULL )
        return;

    pthread_mutex_lock(&(from_me->qlock));  // we touch the queue so we need the mutex
    if (from_me->dont_accept == 1)// don't accept new item to the queue
    {
        pthread_mutex_unlock(&(from_me->qlock));   // unlock before finish
        return;
    }

    work_t* work = (work_t*) malloc(sizeof(work_t));
    if (work == NULL) {
        free_all(from_me);
        return;
    }
        work->routine = dispatch_to_here;
        work->arg = arg;
        work->next = NULL;

    if(from_me->qsize == 0) // we start new linked list
    {
        from_me->qtail = work;
        from_me->qhead = work;
    } else{
        from_me->qtail->next = work;
        from_me->qtail = work;  // we have new tail now
    }

    from_me->qsize++;  // we added new job so ++
    pthread_cond_signal(&(from_me->q_not_empty));
    pthread_mutex_unlock(&(from_me->qlock));
}

void destroy_threadpool(threadpool *destroyme)
{
    if(destroyme == NULL)
        return;

    pthread_mutex_lock(&(destroyme->qlock));  // we touch the queue so we need the mutex
    destroyme->dont_accept = 1 ;  //// dont accept new works now
    if(destroyme->qsize > 0) {// i have more jobs to be done so wait
        pthread_cond_wait(&(destroyme->q_empty), &(destroyme->qlock));  // Wait for queue to become empty
    }
    destroyme->shutdown = 1;
    pthread_cond_broadcast(&(destroyme->q_not_empty));
    pthread_mutex_unlock(&(destroyme->qlock));

    for (int i = 0; i < destroyme->num_threads ; i++)
        pthread_join(destroyme->threads[i],NULL);

    free_all(destroyme);
}

void free_all(threadpool * pool)  // when there is an error i call this function to clear my things
{
    if (pool == NULL)
        return;
    if (pool->threads)
        free(pool->threads);

    pthread_mutex_destroy (&pool->qlock);
    pthread_cond_destroy(&pool->q_not_empty);
    pthread_cond_destroy(&pool->q_empty);

    if(pool->qsize > 0)
        freeLinkedList(pool->qhead);

    free(pool);
}

void freeLinkedList(work_t* headList)        // free all grade list.
{
    if (headList == NULL)
        return;
    freeLinkedList(headList->next);         // first go to the last node and then start to free the list
    free(headList);
}

void popOutWork(threadpool* pool)
{
    if (pool->qhead == NULL)
        return;

        pool->qhead = pool->qhead->next;   // we go to the new job in the list

        if (pool->qhead == NULL)
            pool->qtail = NULL;
        pool->qsize--;
}
