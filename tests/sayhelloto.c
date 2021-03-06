#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "procreact_future.h"

static ProcReact_Future say_hello_to_async(const char *name)
{
    ProcReact_Future future = procreact_initialize_future(procreact_create_string_type());
    
    if(future.pid == 0)
    {
        dprintf(future.fd, "Hello %s!", name);
        _exit(0);
    }
    
    return future;
}

static char *say_hello_to_sync(const char *name, ProcReact_Status *status)
{
    ProcReact_Future future = say_hello_to_async(name);
    return (char*)procreact_future_get(&future, status);
}

int main(int argc, char *argv[])
{
    int exit_status;
    ProcReact_Status status;
    char *ret = say_hello_to_sync("Sander van der Burg", &status);
    
    if(status != PROCREACT_STATUS_OK)
    {
        fprintf(stderr, "An unexpected error occured: %u\n", status);
        exit_status = 1;
    }
    else if(ret == NULL)
    {
        fprintf(stderr, "The return value should not be NULL!\n");
        exit_status = 1;
    }
    else
    {
        if(strcmp(ret, "Hello Sander van der Burg!") == 0)
        {
            fprintf(stderr, "Outcome is correct: %s\n", ret);
            exit_status = 0;
        }
        else
        {
            fprintf(stderr, "Wrong outcome: %s\n", ret);
            exit_status = 1;
        }
    }
    
    free(ret);
    return exit_status;
}
