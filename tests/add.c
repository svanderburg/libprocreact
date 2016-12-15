#include "procreact_pid.h"
#include <stdio.h>

static pid_t add_async(int a, int b)
{
    pid_t pid = fork();
    
    if(pid == 0)
    {
        int result = a + b;
        _exit(result);
    }
    
    return pid;
}

static int add_sync(int a, int b, ProcReact_Status *status)
{
    return procreact_wait_for_exit_status(add_async(a, b), status);
}

int main(int argc, char *argv[])
{
    ProcReact_Status status;
    int result = add_sync(2, 2, &status);
    
    if(status == PROCREACT_STATUS_OK)
    {
        fprintf(stderr, "Result is: %d\n", result);
        
        if(result == 4)
        {
            fprintf(stderr, "Result is correct!\n");
            return 0;
        }
        else
        {
            fprintf(stderr, "Result is incorrect!\n");
            return 1;
        }
    }
    else
    {
        fprintf(stderr, "An unexpected error occurred: %d\n", status);
        return 1;
    }
}
