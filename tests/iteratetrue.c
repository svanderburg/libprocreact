#include "procreact_pid_iterator.h"

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
    ProcReact_bool success;
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

    if(status != PROCREACT_STATUS_OK || !result)
        iterator_data->success = FALSE;
}

int main(int argc, char *argv[])
{
    IteratorData data = { 0, 5, TRUE };
    ProcReact_PidIterator iterator =  procreact_initialize_pid_iterator(has_next_true_process, next_true_process, procreact_retrieve_boolean, complete_true_process, &data);

    procreact_fork_in_parallel_and_wait(&iterator);

    return (!data.success);
}
