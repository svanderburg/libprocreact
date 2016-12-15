#include <stdio.h>
#include "procreact_future.h"

static ProcReact_Future cat_text_file_async(char *filename)
{
    ProcReact_Future future = procreact_initialize_future(procreact_create_string_array_type('\n'));
    
    if(future.pid == 0)
    {
        char *args[] = { "cat", filename, NULL };
        dup2(future.fd, 1);
        execvp(args[0], args);
        _exit(1);
    }
    
    return future;
}

static char **cat_text_file_sync(char *filename, ProcReact_Status *status)
{
    ProcReact_Future future = cat_text_file_async(filename);
    return procreact_future_get(&future, status);
}

int main(int argc, char *argv[])
{
    if(argc > 1)
    {
        int exit_status;
        char *filename = argv[1];
        ProcReact_Status status;
        char **lines = cat_text_file_sync(filename, &status);
        
        if(status == PROCREACT_STATUS_OK)
        {
            if(lines != NULL)
            {
                unsigned int count = 0;
                char *line;
                
                while((line = lines[count]) != NULL)
                {
                    printf("line %u: [%s]\n", count, line);
                    count++;
                }
            }
            
            exit_status = 0;
        }
        else
        {
            fprintf(stderr, "An unexpected error happened: %d\n", status);
            exit_status = 1;
        }
        
        procreact_free_string_array(lines);
        return exit_status;
    }
    else
    {
        fprintf(stderr, "A filename must be provided!\n");
        return 1;
    }
}
