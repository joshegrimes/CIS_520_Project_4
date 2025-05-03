#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>

#define MAX_LINE_LENGTH 4096

int numThreads;
char **lines = NULL;      // Array of pointers to all lines
int total_lines = 0;      // Number of lines read
int capacity = 10000;     // initial capacity
int *results = NULL;      // Global results: max ASCII value for each line

///
/// Reads in all lines in the file to the lines pointer
/// \param filename a string representing the name of the file being read from
///
void read_file(const char *filename)
{
    // opening the file and checking for param issue
    FILE *fp = fopen(filename, "r");
    if(!fp)
    {
        perror("Unable to open file");
        return;
    }

    // allocate memory for the lines
    lines = malloc(capacity * sizeof(char *));
    if(!lines)
    {
        perror("malloc failure");
        return;
    }

    // places the lines from the file into the lines array
    char buffer[MAX_LINE_LENGTH];
    while(fgets(buffer, MAX_LINE_LENGTH, fp))
    {
        if(total_lines == capacity)
        {
            capacity *= 2;
            lines = realloc(lines, capacity * sizeof(char *));
        }
        lines[total_lines] = strdup(buffer);
        total_lines++;
    }

    fclose(fp);
}

///
/// Retrieves the max ASCII value from each line and populates the results array
/// \param arg a generic pointer to pass the thread's ID number into a thread function
///
void *process_lines(void *arg)
{
    // getting the thread's id, setting the start and end
    int threadID = (int)(intptr_t)arg;
    int start = threadID * (total_lines / numThreads);
    int end = (threadID == numThreads - 1) ? total_lines : start + (total_lines / numThreads);

    // algorithm to find the max value in each line and store it in the results array once it's found
    for(int i = start; i < end; i++)
    {
        int max_value = 0;
        char *line = lines[i];
        for(int j = 0; line[j] != '\0'; j++)
        {
            if((int)line[j] > max_value)
            {
                max_value = line[j];
            }
        }
        results[i] = max_value;
    }
    pthread_exit(NULL);
}

///
/// Main function, sets up pthreads and prints out results
/// \param argc number of arguments passed when pthread.c was executed
/// \param argv the arguments passed in text form, in this case it will be a file name
///
int main(int argc, char *argv[])
{
    // param check, informs user correct format to run the executable with
    if(argc < 3)
    {
        fprintf(stderr, "Usage: %s <input_file> <num_threads>\n", argv[0]);
        return 0;
    }

    // parse thread count from argv[2]
    if (sscanf(argv[2], "%d", &numThreads) != 1 || numThreads < 1)
    {
        fprintf(stderr, "Invalid thread count: %s\n", argv[2]);
        return 0;
    }

    // Read the file into memory
    read_file(argv[1]);

    // Allocate the results array
    results = malloc(total_lines * sizeof(int));
    if(!results)
    {
        perror("malloc failure for results");
        return 0;
    }

    pthread_t *threads = malloc(numThreads * sizeof(pthread_t));
    pthread_attr_t attr;
    int rc;

    // initializing with default attributes and making the threads joinable
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // creating new threads
    for(int i = 0; i < numThreads; i++)
    {
        rc = pthread_create(&threads[i], &attr, process_lines, (void *)(intptr_t)i);
        if(rc)
        {
            fprintf(stderr, "Error: return code from pthread_create() is %d\n", rc);
            return 0;
        }
    }

    // we don't need the attr object anymore
    pthread_attr_destroy(&attr);

    for(int i = 0; i < numThreads; i++)
    {
        // blocks the main thread until i finishes, don't care about the return value
        rc = pthread_join(threads[i], NULL);
        if(rc)
        {
            fprintf(stderr, "Error: return code from pthread_join() is %d\n", rc);
            return 0;
        }
    }

    // Print the results for each line in order
    for(int i = 0; i < total_lines; i++)
    {
        printf("%d: %d\n", i, results[i]);
    }

    // free the allocated memory
    for(int i = 0; i < total_lines; i++)
    {
        free(lines[i]);
    }
    free(lines);
    free(results);

    // program completed successfully
    printf("Main: program completed. Exiting.\n");
    return 0;
}
