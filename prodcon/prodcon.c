#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <memory.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>

#include "prodcon.h"

pthread_mutex_t *locks;
pthread_cond_t *conds;

static char *sentinel = NULL;

struct llist_node {
    struct llist_node *next;
    char *str;
};

struct info {
    size_t id;
};

/**
 * pop a node off the start of the list.
 *
 * @param phead the head of the list. this will be modified by the call unless the list is empty
 * (*phead == NULL).
 * @return NULL if list is empty or a pointer to the string at the top of the list. the caller is
 * incharge of calling free() on the pointer when finished with the string.
 */
char *pop(int index, struct llist_node **phead) {
    pthread_mutex_lock(&locks[index]);
    while (*phead == NULL) {
        pthread_cond_wait(&conds[index], &locks[index]);    // we are waiting for cond to change => when call wait, have to have a lock.
    }
    char *s = (*phead)->str;
    struct llist_node *next = (*phead)->next;
    free(*phead);
    *phead = next;
    pthread_mutex_unlock(&locks[index]);
    return s;
}

/**
 * push a node onto the start of the list. a copy of the string will be made.
 * @param phead the head of the list. this will be modified by this call to point to the new node
 * being added for the string.
 * @param s the string to add. a copy of the string will be made and placed at the beginning of
 * the list.
 */
void push(int addSentinel, int index, struct llist_node **phead, const char *s) {
    pthread_mutex_lock(&locks[index]);
    struct llist_node *new_node = malloc(sizeof(*new_node));
    new_node->str = (addSentinel == 1) ? sentinel : strdup(s);

    if (addSentinel == 1) {
      if (*phead == NULL) {
          *phead = new_node;
      } else {
        struct llist_node *currNode = *phead;
        while (currNode->next != NULL) {
            currNode = currNode->next;
        }
        currNode->next = new_node;
      }
    } else {
      new_node->next  = *phead;
      *phead          = new_node;
    }

    pthread_cond_signal(&conds[index]);
    pthread_mutex_unlock(&locks[index]);
}

// the array of list heads. the size should be equal to the number of consumers
static struct llist_node **heads;

static assign_consumer_f assign_consumer;
static int producer_count;
static int consumer_count;

// Accredited to Elana on Canvas Discussion and in-class discussion
_Thread_local int my_consumer_number;

// Make global in order to call in start_producer_thread and start_consermer_thread
static run_producer_f run_producer;
static run_consumer_f run_consumer;

static char **new_argv;
static int new_argc;

void queue(int consumer, const char *str) {
    push(0, consumer, &heads[consumer], str);
}

// What if you have 3 producers and 9 consumers
// 5 producer 1 consumer
void produce(const char *buffer) {
    int hash = assign_consumer(consumer_count, buffer);
    queue(hash, buffer);
}

char *consume() {
    return pop(my_consumer_number, &heads[my_consumer_number]);;
}

void *start_producer_thread(void *i) {
    run_producer((size_t) i, producer_count, produce, new_argc, new_argv);
    return i;
}

void *start_consumer_thread(void *i) {
    // Acredited to Ben (in-class discussion)
    struct info *x = i;
    my_consumer_number = (size_t) x;
    run_consumer((size_t) i, consume, new_argc, new_argv);
    return i;
}

void do_usage(char *prog) {
    printf("USAGE: %s shared_lib consumer_count producer_count ....\n", prog);
    exit(1);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        do_usage(argv[0]);
    }

    char *shared_lib = argv[1];
    producer_count = strtol(argv[2], NULL, 10);
    consumer_count = strtol(argv[3], NULL, 10);

    pthread_t producerThreads[producer_count];
    pthread_t consumerThreads[consumer_count];

    new_argv = &argv[4];
    new_argc = argc - 4;
    setlinebuf(stdout);

    void *dh = dlopen(shared_lib, RTLD_LAZY);

    // load the producer, consumer, and assignment functions from the library
    run_producer = dlsym(dh, "run_producer");
    run_consumer = dlsym(dh, "run_consumer");
    assign_consumer = dlsym(dh, "assign_consumer");
    if (run_producer == NULL || run_consumer == NULL || assign_consumer == NULL) {
        printf("Error loading functions: prod %p cons %p assign %p\n", run_producer,
                run_consumer, assign_consumer);
        exit(2);
    }

    if (consumer_count <= 0 || producer_count <= 0) {
        do_usage(argv[0]);
    }

    heads = calloc(consumer_count, sizeof(*heads));
    locks = calloc(consumer_count, sizeof(*locks));
    conds = calloc(consumer_count, sizeof(*conds));

    for (int i = 0; i < consumer_count; i++) {
        pthread_mutex_init(&locks[i], NULL);
        pthread_cond_init(&conds[i], NULL);
    }

    for (int i = 0; i < producer_count; i++) {
        // There is a warning when I do (void  *) i => went to stackoverflow and add (size_t) to get rid of warning
        pthread_create(&producerThreads[i], NULL, start_producer_thread, (void *)(size_t)i);
    }

    for (int i = 0; i < consumer_count; i++) {
        pthread_create(&consumerThreads[i], NULL, start_consumer_thread, (void *)(size_t)i);
    }

    // Know that all the producer is done assigning
    for(int i = 0; i < producer_count; i++) {
        void *v;
        pthread_join(producerThreads[i], &v);
    }

    // Add sentinel for each consumer
    for(int i = 0; i < consumer_count; i++) {
        push(1, i, &heads[i], sentinel);
    }

    // Join all the consumer
    for(int i = 0; i < consumer_count; i++) {
        void *v;
        pthread_join(consumerThreads[i], &v);
    }
    return 0;
}
