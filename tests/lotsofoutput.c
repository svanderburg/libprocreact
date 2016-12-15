#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "procreact_future.h"

#define NUM_OF_STRINGS 100

char *alphabet = "abcdefghijklmnopqrstuvxyz";

static ProcReact_Future return_alphabet_async(void)
{
    ProcReact_Future future = procreact_initialize_future(procreact_create_string_type());
    
    if(future.pid == 0)
    {
        unsigned int i;
        
        for(i = 0; i < NUM_OF_STRINGS; i++)
            dprintf(future.fd, "%s", alphabet);
        
        _exit(0);
    }
    
    return future;
}

static char *return_alphabet_sync(ProcReact_Status *status)
{
    ProcReact_Future future = return_alphabet_async();
    return procreact_future_get(&future, status);
}

int main(int argc, char *argv[])
{
    int exit_status;
    ProcReact_Status status;
    char *ret = return_alphabet_sync(&status);
    
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
        fprintf(stderr, "%s", ret);
        
        if(strlen(ret) == NUM_OF_STRINGS * strlen(alphabet))
        {
            fprintf(stderr, "\nThe amount of characters is correct!\n");
            exit_status = 0;
        }
        else
        {
            fprintf(stderr, "\nThe amount of characters is incorrect!\n");
            exit_status = 1;
        }
    }
    
    free(ret);
    return exit_status;
}
