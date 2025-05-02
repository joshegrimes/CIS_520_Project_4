#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <omp.h>

int main(int argc, char *argv[])
{
    // ensures there is only one argument after the executable: the file path
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 0;
    }
    const char *path = argv[1];

    // calls open in read only mode and reports an error if one occurred
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return 0;
    }

    // determines the files size to later be used with mmap and reports an error if one occurred
    struct stat st;
    if (fstat(fd, &st) != 0)
    {
        perror("fstat");
        close(fd);
        return 0;
    }
    size_t filesize = st.st_size;
    if (filesize == 0)
    {
        fprintf(stderr, "Empty file\n");
        close(fd);
        return 0;
    }

    // maps the file to memory, as suggested by https://hpc-tutorials.llnl.gov/openmp/
    // reports an error if one occurred; protection is set to read only, MAP_PRIVATE
    // flag indicates writes made to the mapped memory aren't visible to the underlying
    // file and won't be seen by other processes mapping the same file
    char *buf = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buf == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return 0;
    }
    close(fd);

    // scans the entire buffer one time to count newline characters, if the file doesn't end in
    // \n, it adds one more line for the final, non-terminated line
    size_t nlines = 0;
    for (size_t i = 0; i < filesize; ++i)
    {
        if (buf[i] == '\n') nlines++;
    }
    if (filesize > 0 && buf[filesize-1] != '\n')
        nlines++;

    // allocates helper arrays to hold byte offsets and the maximum value of each line
    // reports a failure if one occurred
    size_t * start = malloc(nlines * sizeof(size_t));
    size_t * end = malloc(nlines * sizeof(size_t));
    int * maxval = malloc(nlines * sizeof(int));
    if (!start || !end || !maxval)
    {
        fprintf(stderr, "Allocation failure\n");
        munmap(buf, filesize);
        return 0;
    }

    // iterates over the buffer again, and whenever it sees \n, sets start[i] to the byte after
    // the previous newline (or 0 for the first line), sets end[i] to the position of the current
    // newline, and updates last to the next character
    size_t idx = 0, last = 0;
    for (size_t i = 0; i < filesize; ++i)
    {
        if (buf[i] == '\n')
        {
            start[idx] = last;
            end[idx] = i;
            last = i + 1;
            idx++;
        }
    }
    // if there's a trailing non-newline line, its start and end are addded
    if (last < filesize)
    {
        start[idx] = last;
        end[idx] = filesize;
    }

    // openMP parallel for loop splits the lines evenly among the threads and retrieves the
    // maximum printable ASCII value per line
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < nlines; ++i)
    {
        int maxValue = 0;
        for (size_t j = start[i]; j < end[i]; ++j)
        {
            unsigned char c = buf[j];
            if (c >= 32 && c <= 126 && c > maxValue)
                maxValue = c;
        }
        maxval[i] = maxValue;
    }

    // prints the results
    for (size_t i = 0; i < nlines; ++i)
    {
        printf("%zu: %d\n", i, maxval[i]);
    }

    // cleanup; frees memory
    free(start);
    free(end);
    free(maxval);
    munmap(buf, filesize);

    // success
    return 1;
}
