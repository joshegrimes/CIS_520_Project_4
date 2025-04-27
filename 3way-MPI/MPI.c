#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, nprocs;  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (argc < 2) {
        if (!rank) fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        MPI_Finalize();  return 1;
    }
    const char *fname = argv[1];

    MPI_Offset fsize = 0;
    if (!rank) {
        FILE *fs = fopen(fname, "rb");
        if (!fs) { perror("fopen"); MPI_Abort(MPI_COMM_WORLD, 2); }
        fseek(fs, 0, SEEK_END);
        fsize = ftell(fs);
        fclose(fs);
    }
    MPI_Bcast(&fsize, 1, MPI_OFFSET, 0, MPI_COMM_WORLD);

    MPI_Offset chunk  = (fsize + nprocs - 1) / nprocs;  /* ceil-div */
    MPI_Offset begin  = rank * chunk;
    MPI_Offset end    = (begin + chunk > fsize) ? fsize : begin + chunk;
    MPI_Offset bytes  = (end > begin) ? end - begin : 0;

    MPI_File fh;  MPI_File_open(MPI_COMM_WORLD, fname,
                                MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);

    char *buf = (bytes ? malloc(bytes) : NULL);
    MPI_File_read_at_all(fh, begin, buf, bytes, MPI_CHAR, MPI_STATUS_IGNORE);
    MPI_File_close(&fh);

    MPI_Offset off = 0;
    if (rank) {                      /* move to first newline */
        while (off < bytes && buf[off] != '\n') ++off;
        if (off < bytes) ++off;
    }

    unsigned char *vals = NULL; size_t cap = 1024, n = 0;
    if (bytes) { vals = malloc(cap); }

    unsigned char cur = 0;
    for (MPI_Offset i = off; i < bytes; ++i) {
        unsigned char c = (unsigned char)buf[i];
        if (c != '\n') {
            if (c > cur) cur = c;
        } else {                         /* end of a line */
            if (n == cap) { cap <<= 1; vals = realloc(vals, cap); }
            vals[n++] = cur;
            cur = 0;
        }
    }
    free(buf);

    int myCount = (int)n;
    int *counts = NULL, *displs = NULL;
    if (!rank) { counts = malloc(nprocs * sizeof *counts); }

    MPI_Gather(&myCount, 1, MPI_INT,
               counts,   1, MPI_INT, 0, MPI_COMM_WORLD);

    unsigned char *allvals = NULL;
    if (!rank) {
        displs = malloc(nprocs * sizeof *displs);
        displs[0] = 0;
        for (int p = 1; p < nprocs; ++p)
            displs[p] = displs[p-1] + counts[p-1];

        allvals = malloc(displs[nprocs-1] + counts[nprocs-1]);
    }

    MPI_Gatherv(vals, myCount, MPI_UNSIGNED_CHAR,
                allvals, counts, displs, MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);

    if (!rank) {
        long idx = 0;
        long total = displs[nprocs-1] + counts[nprocs-1];
        for (; idx < total; ++idx)
            printf("%ld: %u\n", idx, allvals[idx]);

        free(allvals); free(counts); free(displs);
    }
    free(vals);
    MPI_Finalize();
    return 0;
}

