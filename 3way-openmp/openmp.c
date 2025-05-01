#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <omp.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *path = argv[1];

    // Open file
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    // Get file size
    struct stat st;
    if (fstat(fd, &st) != 0) {
        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }
    size_t filesize = st.st_size;
    if (filesize == 0) {
        fprintf(stderr, "Empty file\n");
        close(fd);
        return EXIT_SUCCESS;
    }

    // Memory-map the file
    char *buf = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buf == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }
    close(fd);

    double t0 = omp_get_wtime();

    // Count lines (include final line if not newline-terminated)
    size_t nlines = 0;
    for (size_t i = 0; i < filesize; ++i) {
        if (buf[i] == '\n') nlines++;
    }
    if (filesize > 0 && buf[filesize-1] != '\n') nlines++;

    // Allocate arrays for line boundaries and results
    size_t *start = malloc(nlines * sizeof(size_t));
    size_t *end   = malloc(nlines * sizeof(size_t));
    int    *maxval = malloc(nlines * sizeof(int));
    if (!start || !end || !maxval) {
        fprintf(stderr, "Allocation failure\n");
        munmap(buf, filesize);
        return EXIT_FAILURE;
    }

    // Record line start/end offsets
    size_t idx = 0, last = 0;
    for (size_t i = 0; i < filesize; ++i) {
        if (buf[i] == '\n') {
            start[idx] = last;
            end[idx]   = i;
            last = i + 1;
            idx++;
        }
    }
    if (last < filesize) {
        start[idx] = last;
        end[idx]   = filesize;
    }

    // Parallel computation: max printable ASCII per line
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < nlines; ++i) {
        int mx = 0;
        for (size_t j = start[i]; j < end[i]; ++j) {
            unsigned char c = buf[j];
            if (c >= 32 && c <= 126 && c > mx) mx = c;
        }
        maxval[i] = mx;
    }

    double t1 = omp_get_wtime();

    // Output results
    for (size_t i = 0; i < nlines; ++i) {
        printf("%zu: %d\n", i, maxval[i]);
    }

    fprintf(stderr, "Processed %zu lines in %.3f seconds with %d threads.\n",
            nlines, t1 - t0, omp_get_max_threads());

    // Cleanup
    free(start);
    free(end);
    free(maxval);
    munmap(buf, filesize);
    return EXIT_SUCCESS;
}
