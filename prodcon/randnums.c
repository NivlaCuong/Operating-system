#include "prodcon.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * this is a simple producer consumer shared library that will generate random
 * numbers in the producer printing the sum of the numbers they each have
 * generated at the end. the consumers will sum the numbers they consume, printing
 * the sum at the end.
 */
void run_producer(int num, int producer_count, produce_f produce, int argc, char
**argv)
{
    unsigned int seed = num;
    long long total = 0;
    for (
            int i = 0; i < 1000; i++) {
        int r = rand_r(&seed) % 1000;
        char buffer[21];
        sprintf(buffer, "%d", r);
        produce(buffer);
        total += r;
    }
    printf("producer %d produced: %lld\n", num, total);
}

void run_consumer(int num, consume_f consume, int argc, char **argv)
{
    long long total = 0;
    char *str;
    while ((str = consume()) != NULL) {
        // printf("consumer %d -> String is %s\n", num, str);
        total += atol(str);
	       free(str);
    }
    printf("consumer %d consumed: %lld\n", num, total);
}

int assign_consumer(int consumer_count, const char *buffer)
{
    return labs(atol(buffer)) % consumer_count;
}
