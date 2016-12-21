#include <stdio.h>
#include "procreact_signal.h"

#define TRUE 1
#define FALSE 0

static pid_t true_async(void)
{
    pid_t pid = fork();
    
    if(pid == 0)
    {
        char *const args[] = { "true", NULL };
        execvp(args[0], args);
        _exit(1);
    }
    
    return pid;
}

typedef struct
{
    unsigned int index;
    unsigned int length;
    int status;
    unsigned int complete_count;
}
IteratorData;

static int has_next_true_process(void *data)
{
    IteratorData *iterator_data = (IteratorData*)data;
    return iterator_data->index < iterator_data->length;
}

static pid_t next_true_process(void *data)
{
    IteratorData *iterator_data = (IteratorData*)data;
    pid_t pid = true_async();
    iterator_data->index++;
    return pid;
}

static void complete_true_process(void *data, pid_t pid, ProcReact_Status status, int result)
{
    IteratorData *iterator_data = (IteratorData*)data;
    iterator_data->complete_count++;
    fprintf(stderr, "process: %d complete, with status: %u, and result: %d\n", pid, status, result);
}

int main(int argc, char *argv[])
{
    if(procreact_register_signal_handler() == -1)
    {
        fprintf(stderr, "Cannot register signal handler!\n");
        return 1;
    }
    else
    {
        IteratorData data = { 0, 5, TRUE, 0 };
        ProcReact_PidIterator iterator = procreact_initialize_pid_iterator(has_next_true_process, next_true_process, procreact_retrieve_boolean, complete_true_process, &data);

        /* Spawn the processes in parallel */
        while(procreact_spawn_next_pid(&iterator));
        
        /* Main loop that runs until all process have been completed */
        while(data.complete_count < 5)
            procreact_complete_all_finished_processes(&iterator);
        
        return 0;
    }
}
