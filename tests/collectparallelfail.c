#include "procreact_future_iterator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

typedef struct
{
    unsigned int index;
    unsigned int amount;
    int success;
    char **results;
    unsigned int results_length;
}
IteratorData;

static ProcReact_Future return_count_async(unsigned int count)
{
    ProcReact_Future future = procreact_initialize_future(procreact_create_string_type());
    
    if(future.pid == 0)
    {
        dprintf(future.fd, "%u", count);
        _exit(1); /* Deliberately fail the process */
    }
    
    return future;
}

static int has_next_count(void *data)
{
    IteratorData *iterator_data = (IteratorData*)data;
    return iterator_data->index < iterator_data->amount;
}

static ProcReact_Future next_count_process(void *data)
{
    IteratorData *iterator_data = (IteratorData*)data;
    ProcReact_Future future = return_count_async(iterator_data->index + 1);
    iterator_data->index++;
    return future;
}

static void complete_count_process(void *data, ProcReact_Future *future, ProcReact_Status status)
{
    IteratorData *iterator_data = (IteratorData*)data;
    
    if(status == PROCREACT_STATUS_OK && future->result != NULL)
    {
        iterator_data->results = (char**)realloc(iterator_data->results, (iterator_data->results_length + 1) * sizeof(char*));
        iterator_data->results[iterator_data->results_length] = future->result;
        iterator_data->results_length++;
    }
    else
        iterator_data->success = FALSE;
}

static void free_string_array(char **arr, unsigned int arr_length)
{
    if(arr != NULL)
    {
        unsigned int i;
        
        for(i = 0; i < arr_length; i++)
            free(arr[i]);
            
        free(arr);
    }
}

int main(int argc, char *argv[])
{
    int exit_status;
    IteratorData data = { 0, 5, TRUE, NULL, 0 };
    ProcReact_FutureIterator iterator = procreact_initialize_future_iterator(has_next_count, next_count_process, complete_count_process, &data);
    
    procreact_fork_in_parallel_buffer_and_wait(&iterator);
    
    if(data.success)
    {
        fprintf(stderr, "It seems to have unexpectedly succeeded!\n");
        exit_status = 1;
    }
    else
    {
        fprintf(stderr, "A failure has occured, as we expect it to happen!\n");
        exit_status = 0;
    }
    
    free_string_array(data.results, data.results_length);
    procreact_destroy_future_iterator(&iterator);
    
    return exit_status;
}
