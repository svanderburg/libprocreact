libprocreact
============
This library provides a number of abstractions to make implementing certain
multi-process programming patterns in POSIX applications more convenient. Its
primary use-case is to facilitate process orchestration in
[Disnix](http://nixos.org/disnix).

Disnix is a tool that automatically deploys service-oriented systems from
declarative specifications. Each deployment activity is carried out by a
different (child)process, for the following reasons:

* Some of these processes may have to run in parallel to improve the tool's
  performance
* External tools must be invoked that carry out certain deployment activities

This library encapsulates the patterns Disnix uses to accomplish the above
properties. The resulting function interfaces are inspired by reactive
programming patterns and look somewhat similar.

Installation
============
Installation of `libprocreact` is very straight forward by running the standard
Autotools build procedure:

    $ ./configure
    $ make
    $ make install

Usage
=====
This library offers abstractions that can be used in a variety of scenarios.

Writing a primitive function
----------------------------
Take the following trivial function as an example that simply displays the text
`Hello!` on the standard output:

```C
#include <stdio.h>

void print_hello(void)
{
    printf("Hello!\n");
}
```

we can also split and extend the above function into an asynchronous variant:

```C
#include <unistd.h>

pid_t print_hello_async(void)
{
    pid_t pid = fork();
    
    if(pid == 0)
    {
        printf("Hello!\n");
        _exit(0);
    }
    
    return pid;
}
```

The asynchronous function (shown above) does the following:
* It forks a child process executing the print statement
* It returns the pid of the child process to the parent (or -1 if the child
  process cannot be forked)

In addition to running the function asynchronously (by means of a process), the
parent process must also know when the process has finished and whether it has
succeeded. This library contains a number of wait abstractions making this job
more convenient than manually invoking `wait()` and the corresponding wait
macros:

```C
#include <procreact_pid.h>

ProcReact_Status status;

/* Invoke the asynchronous function */
pid_t pid = print_hello_async();

/* Wait and retrieve its exit status */
int exit_status = procreact_wait_for_exit_status(pid, &status);

/* Check the result */
if(status != PROCREACT_STATUS_OK)
    fprintf(stderr, "The process terminated abnormally!\n");
else if(exit_status != 0)
    fprintf(stderr, "The process failed!\n");
```

In the above code fragment, the `procreact_wait_for_exit_status()` invocation
waits for the process to complete and returns the corresponding exit status.
Futhermore, it sets the `status` variable to a status code that we can use to
determine whether the process has been terminated abnormally or not.

In many cases, when composing a function that runs something in a process
asynchronously, you may also want to provide a function abstraction that runs
the same operation synchronously. Writing such a function is straight forward:

```C
int print_hello_sync(ProcReact_Status *status)
{
    return procreact_wait_for_exit_status(print_hello_async(), status);
}
```

By simply wrapping the function invocation in a wait abstraction, we can provide
a synchronous variant of an asynchronous function.

In the above example, we use `procreact_wait_for_exit_status()` as a function to
retrieve the result. It is also possible to use different return types or create
a custom retrieval function. See section: 'Intrerpreting exit statuses' for more
information.

Writing a function providing arbitrary output (complex functions)
-----------------------------------------------------------------
The previously shown abstractions work well for functions returning a byte,
boolean, or void-functions. However, it may also be desirable to implement
asynchronous functions returning more complex data, such as strings or arrays of
strings. For example:

```C
#include <stdlib.h>
#include <string.h>

char *say_hello_to(const char *name)
{
    char *result = (char*)malloc(strlen(name) + 7 + 1);
    sprintf(result, "Hello %s!", name);
    return result;
}
```

The above function composes a string that greets a person with a given name.
Implementing an asynchronous variant of the above function requires
extra facilities to propagate the result back to the parent process.

By constructing a `ProcReact_Future` struct, we can automatically fork a
process, set up a pipe, and let the child process construct the greeting:

```C
#include <procreact_future.h>

ProcReact_Future say_hello_to_async(const char *name)
{
    ProcReact_Future future = procreact_initialize_future(procreact_create_string_type());
    
    if(future.pid == 0)
    {
        dprintf(future.fd, "Hello %s!", name);
        _exit(0);
    }
    
    return future;
}
```

The `procreact_initialize_future()` function takes one parameter: a type, that is
responsible for reading the output from the pipe and converting it into a
representation of choice -- in this case a NUL-terminated string.

We can collect the return value of the function by invoking the
`procreact_future_get()` function:

```C
ProcReact_Status status;
ProcReact_Future future = say_hello_to_async(name);
char *result = procreact_future_get(&future, &status);

if(status == PROCREACT_STATUS_OK && result != NULL)
    printf("%s\n", result);
else
    fprintf(stderr, "Some error occured!\n");
```

When the return value is no longer needed, we must free it:

```C
free(result);
```

As with primitive functions, it may also be desirable to implement a synchronous
variant of an asynchronous complex function. This can be done by simply wrapping
the invocation into a `ProcReact_Future` struct:

```C
char *say_hello_to_sync(const char *name, ProcReact_Status *status)
{
    ProcReact_Future future = say_hello_to_async(name);
    return procreact_future_get(&future, status);
}
```

In the above examples, I have described that a future requires a type parameter.
In addition to strings, also byte arrays and string arrays are supported.
Furthermore, you can also compose your own types. See section: 'Composing custom
types' for more information.

Synchronously executing a collection of primitive functions
-----------------------------------------------------------
In addition to asynchronous functions returning single values, we may also want
to work with collections of asynchronous function invocations. For example, we
may want to invoke the following function multiple times, preferably in parallel
with other function invocations, to provide improved performance:

```C
pid_t true_async(void)
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
```

This library provides an iterator abstraction that works with collections of
processes. For example, we can construct an iterator for primitive functions as
follows:

```C
#include <procreact_pid_iterator.h>

ProcReact_PidIterator iterator = procreact_initialize_pid_iterator(has_next_true_process, next_true_process, procreact_retrieve_boolean, complete_true_process, &data);
```

The above function invocation: `procreact_initialize_pid_iterator()` configures
an `ProcReact_PidIterator` struct. It takes the following parameters:

* A pointer to a function that indicates whether there is a next element in the
  collection
* A pointer to a function that spawns the next process in the collection
* A pointer to a function that retrieves the result (from the exit status) of
  each process
* A pointer to a function that gets invoked when a process completes
* A void-pointer referring to an arbitrary data structure that gets passed to
  all functions above, except the retrieve function

To most of the functions an arbitrary data structure is passed as a void-pointer.
This data structure's purpose is to compose the end result of the iteration.
It is up to the implementer to decide what information it should capture and how
it is stored.

For example, to simply iterate over a fixed number of processes and capture the
overall result as a boolean (that indicates its success), we could use the
following struct:

```C
typedef struct
{
    unsigned int index;
    unsigned int length;
    int success;
}
IteratorData;

IteratorData data = { 0, 5, TRUE };
```

We can use the following functions to make the iteration possible over a
collection of invocations to the asynchronous function invoking the `true`
process:

```C
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
```

As may be noticed, the above functions use our custom `IteratorData` struct to
track the state of the iteration.

We can use the following function to retrieve the result of each process
invocation:

```C
static void complete_true_process(void *data, pid_t pid, ProcReact_Status status, int result)
{
    IteratorData *iterator_data = (IteratorData*)data;
    
    if(status != PROCREACT_STATUS_OK || !result)
        iterator_data->success = FALSE;
}
```

The above function takes each boolean result and sets the overall success status
to `FALSE` if it encounters any failure.

We can use the iterator struct to execute all processes in parallel, by
invoking:

```C
procreact_fork_in_parallel_and_wait(&iterator);
```

We can also limit the amount of processes that are allowed to run concurrently
to a specific value, e.g. `2`:

```C
procreact_fork_and_wait_in_parallel_limit(&iterator, 2);
```

Synchronously executing a collection of complex functions
---------------------------------------------------------
As with functions returning single values, we may also want to execute a
collection of complex asynchronous functions, that for example, return strings:

```C
ProcReact_Future return_count_async(unsigned int count)
{
    ProcReact_Future future = procreact_initialize_future(procreact_create_string_type());
    
    if(future.pid == 0)
    {
        dprintf(future.fd, "%u", count);
        _exit(0);
    }
    
    return future;
}
```

The above example function simply returns the provided `count` integer value as
a string.

By constructing an `ProcReact_FutureIterator` struct we can iterate over a
collection of complex function invocations:

```C
ProcReact_FutureIterator iterator = procreact_initialize_future_iterator(has_next_count, next_count_process, complete_count_process, &data);
```

The above function invocation: `procreact_initialize_future_iterator()`
configures an `ProcReact_FutureIterator` struct. It takes the following
parameters:

* A pointer to a function that indicates whether there is a next element in the
  collection
* A pointer to a function that invokes the next process in the collection
* A pointer to a function that gets invoked when a process completes
* A void-pointer referring to an arbitrary data structure that gets passed to
  all functions above

We can use the following data structure to iterate over a predefined number of
processes whose results get collected in an array of strings:

```C
typedef struct
{
    unsigned int index;
    unsigned int amount;
    int success;
    char **results;
    unsigned int results_length;
}
IteratorData;

IteratorData data = { 0, 5, TRUE, NULL, 0 };
```

We can use the following functions to make the iteration possible over a
collection of invocations to `return_acount_async()`:

```C
static int has_next_count_process(void *data)
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
```

We can use the following function to retrieve the result of each process
invocation. In this function, we append the process' result to the array of
results:

```C
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
```

We can use the iterator struct to execute all processes in parallel, by
invoking:

```C
procreact_fork_in_parallel_buffer_and_wait(&iterator);
```

We can also limit the amount of processes that are allowed to run concurrently
to a specific value, e.g. `2`:

```C
procreact_fork_buffer_and_wait_in_parallel_limit(&iterator, 2);
```

When the iteration process is done, you must deallocate the future iterator's
resources:

```C
procreact_destroy_future_iterator(&iterator);
```

Asynchronously executing a collection of primitive functions
------------------------------------------------------------
The previous two collection examples are executed *synchronously*. This means
that while the execution of each function that retrieves an element is done
asynchronously, the overall iteration task blocks the parent process until it
completes, which is not always desirable.

A possible solution to make iterations asynchronous is to fork another
process and iterate over the collection in the child process, but this
introduces another challenge when the collected data needs to be returned to the
parent.

This library also provides short-running/non-blocking functions that can be
integrated into a program's main loop.

The following code fragment configures a signal handler for the `SIGCHLD` signal
notifying the system that there are child process that have been terminated
whose exit statuses must be evaluated:

```C
#include <procreact_signal.h>

if(procreact_register_signal_handler() == -1)
{
    fprintf(stderr, "Cannot register signal handler!\n");
    return 1;
}
```

The `procreact_spawn_next_pid()` function can be used to individually spawn
processes invoking primitive functions provided by an iterator.

```C
if(procreact_spawn_next_pid(&iterator))
    printf("Spawned a process and we have more of them!\n");
else
    printf("All processes have been spawned\n");
```

We can integrate invocations to `procreact_complete_all_finished_processes()`
to retrieve the results of completed processes in a main loop:

```C
while(TRUE)
{
    /* Do other stuff in the main loop */
    
    procreact_complete_all_finished_processes(&iterator);
}
```

Asynchronously executing a collection of complex functions
----------------------------------------------------------
Similar to primitive functions, we can also asynchronously process future
iterators:

```C
if(procreact_spawn_next_future(&iterator))
    printf("Spawned a process and we have more of them!\n");
else
    printf("All process have been spawned\n");
```

We can integrate invocations to `procreact_buffer()` into a main loop to process
chunks of the read-end of the pipe:

```C
while(TRUE)
{
    unsigned int running_processes = procreact_buffer(&iterator);
    
    if(running_processes == 0)
    {
        /* This indicates that there are no running processes anymore */
        /* You could do something with the end result here */
    }
    
    /* Do other stuff in the main loop */
}
```

As with synchronous execution of a future iterator loop, we must deallocate its
resources when the work is done:

```C
procreact_destroy_future_iterator(&iterator);
```

Advanced features
=================
In addition to the scenarios described in the previous sections, this library
also offers a number of advanced features.

Interpreting exit statuses
--------------------------
As described earlier, there are various retrieval functions that can be used to
determine the outcome of a function depending on its exit status. Currently this
library supports the following functions:

* `procreact_wait_for_exit_status()`. Simply propagates the exit status as a
  result.
* `procreact_wait_for_boolean()`. Returns a boolean value based on the exit
  status. By convention a 0 exit status is `TRUE` whereas any non-zero exit
  status is considered `FALSE`.

You can also implement your own custom wait function by defining a retrieval
function first, e.g.

```C
int procreact_retrieve_boolean(pid_t pid, int wstatus, ProcReact_Status *status)
{
    return (procreact_retrieve_exit_status(pid, wstatus, status) == 0);
}
```

and combining it by invoking `procreact_wait_and_retrieve()`:

```C
int procreact_wait_for_boolean(pid_t pid, ProcReact_Status *status)
{
    return procreact_wait_and_retrieve(pid, procreact_retrieve_boolean, status);
}
```

Composing custom types
----------------------
As explained earlier, when retrieving the results of complex functions, a pipe
gets constructed from which data is being read. The data is converted into a
data structure of choice, such as:

* A byte array, that can be constructed with: `procreact_create_bytes_type()`
* String, that can be constructed with: `procreact_create_string_type()`
* String array, that can be constructed with:
  `procreact_create_string_array_type()`

In addition to the above types, it is also possible to construct your own by
creating your own `ProcReact_Type` struct instance:

```C
struct ProcReact_Type
{
    void *(*initialize) (void);
    ssize_t (*append) (ProcReact_Type *type, void *state, int fd);
    void *(*finalize) (void *state, pid_t pid, ProcReact_Status *status);
};
```

The above struct has three function pointers as its members:

* The `initialize` function is executed before the reading from the pipe starts.
  This function is responsible for allocating a data structure in which the
  state can be tracked.
* The `append` function is repeatedly executed (typically) in a loop to read
  from the pipe and update the recorded state with the read data.
* The `finalize` function performs finalization steps (such as adding a NULL
  termination), error checking (based on the provided status) and cleaning all
  obsolete resources. It returns the data that is returned to the caller of a
  wait function.

License
=======
This library is [MIT licensed](https://opensource.org/licenses/MIT).
