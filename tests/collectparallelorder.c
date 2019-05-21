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
        _exit(0);
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
        iterator_data->results[iterator_data->results_length] = (char*)future->result;
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
    
    procreact_fork_buffer_and_wait_in_parallel_limit(&iterator, 1);
    
    if(data.success)
    {
        if(data.results_length != 5)
        {
            fprintf(stderr, "Incorrect number of elements: %u\n", data.results_length);
            exit_status = 1;
        }
        else
        {
            /* Check if the result matches what we expect */
            if(strcmp(data.results[0], "1") != 0 ||
              strcmp(data.results[1], "2") != 0 ||
              strcmp(data.results[2], "3") != 0 ||
              strcmp(data.results[3], "4") != 0 ||
              strcmp(data.results[4], "5") != 0)
            {
                fprintf(stderr, "One of the result elements is incorrect!\n");
                exit_status = 1;
            }
            else
                exit_status = 0;
        }
    }
    else
    {
        fprintf(stderr, "A failure has occured!\n");
        exit_status = 1;
    }
    
    free_string_array(data.results, data.results_length);
    procreact_destroy_future_iterator(&iterator);
    
    return exit_status;
}
