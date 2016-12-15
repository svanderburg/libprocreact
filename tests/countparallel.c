#include <stdio.h>
#include <stdlib.h>
#include "procreact_pid_iterator.h"

static pid_t print_number(unsigned int num)
{
    pid_t pid = fork();
    
    if(pid == 0)
    {
        printf("%u ", num);
        exit(0);
    }
    
    return pid;
}

typedef struct
{
    unsigned int index;
    unsigned int length;
}
IteratorData;

static int has_next_print_number_process(void *data)
{
    IteratorData *iterator_data = (IteratorData*)data;
    return iterator_data->index < iterator_data->length;
}

static pid_t next_print_number_process(void *data)
{
    IteratorData *iterator_data = (IteratorData*)data;
    pid_t pid = print_number(iterator_data->index + 1);
    iterator_data->index++;
    return pid;
}

static void complete_print_number_process(void *data, pid_t pid, ProcReact_Status status, int result)
{
    if(status != PROCREACT_STATUS_OK)
        printf("An invocation seems to fail!\n");
}

int main(int argc, char *argv[])
{
    IteratorData data = { 0, 5 };
    ProcReact_PidIterator iterator = procreact_initialize_pid_iterator(has_next_print_number_process, next_print_number_process, procreact_retrieve_boolean, complete_print_number_process, &data);
    
    procreact_fork_and_wait_in_parallel_limit(&iterator, 1);
    
    return 0;
}
