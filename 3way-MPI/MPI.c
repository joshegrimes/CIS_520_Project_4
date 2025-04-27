#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, nprocs; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (argc < 2) {
        if (!rank) fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        MPI_Finalize(); return 1;
    }
    const char *fname = argv[1];

    /* ---------- Step 1: Determine file size (root) then broadcast -------- */
    MPI_Offset fsize = 0;
    if (!rank) {
        FILE *fs = fopen(fname, "rb");
        if (!fs) { perror("fopen"); MPI_Abort(MPI_COMM_WORLD, 2); }
        fseek(fs, 0, SEEK_END);
        fsize = ftell(fs);
        fclose(fs);
    }
    MPI_Bcast(&fsize, 1, MPI_OFFSET, 0, MPI_COMM_WORLD);

    /* ---------- Step 2: Establish byte-ranges for each rank -------------- */
    MPI_Offset chunk = (fsize + nprocs - 1) / nprocs;         // ceil-div
    MPI_Offset begin = rank * chunk;
    MPI_Offset end   = (begin + chunk > fsize) ? fsize : begin + chunk;
    MPI_Offset myBytes = (end > begin) ? end - begin : 0;

    /* ---------- Step 3: Read my slice collectively ---------------------- */
    MPI_File fh;
    MPI_File_open(MPI_COMM_WORLD, fname, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);

    char *buf = malloc(myBytes + 1);          // +1 for sentinel
    MPI_File_read_at_all(fh, begin, buf, myBytes, MPI_CHAR, MPI_STATUS_IGNORE);
    buf[myBytes] = '\0';                      // safety

    MPI_File_close(&fh);

    /* ---------- Step 4: Split slice into lines, compute maxima ----------- */
    // Ensure line boundaries: if not rank 0, skip until after first '\n'
    size_t offset = 0;
    if (rank) {
        while (offset < myBytes && buf[offset] != '\n') ++offset;
        if (offset < myBytes) ++offset;       // past newline
    }

    // Build vector of <index, maxASCII>
    typedef struct { long idx; unsigned char val; } pair_t;
    size_t cap = 1024, n = 0; pair_t *vec = malloc(cap * sizeof *vec);

    long globalLine = 0;                      // will be fixed later
    MPI_Exscan(&globalLine, &globalLine, 1, MPI_LONG, MPI_SUM, MPI_COMM_WORLD);

    unsigned char max = 0;
    for (MPI_Offset i = offset; i < myBytes; ++i) {
        unsigned char c = (unsigned char)buf[i];
        if (c != '\n') {
            if (c > max) max = c;
        } else {                              // end of line
            if (n == cap) { cap *= 2; vec = realloc(vec, cap * sizeof *vec); }
            vec[n++] = (pair_t){ globalLine++, max };
            max = 0;
        }
    }
    free(buf);

    /* ---------- Step 5: Gather counts then all results to rank 0 --------- */
    int *counts = NULL, *displs = NULL;
    int myPairs = (int)n;
    if (!rank) { counts = malloc(nprocs * sizeof *counts); }
    MPI_Gather(&myPairs, 1, MPI_INT, counts, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (!rank) {
        displs = malloc(nprocs * sizeof *displs);
        displs[0] = 0;
        for (int p = 1; p < nprocs; ++p) displs[p] = displs[p-1] + counts[p-1];
    }
    pair_t *all = NULL;
    int totalPairs = 0;
    if (!rank) {
        for (int p = 0; p < nprocs; ++p) totalPairs += counts[p];
        all = malloc(totalPairs * sizeof *all);
    }
    MPI_Gatherv(vec, myPairs, MPI_BYTE,
                all, counts, displs, MPI_BYTE,
                0, MPI_COMM_WORLD);

    /* ---------- Step 6: Rank 0 prints in order -------------------------- */
    if (!rank) {
        for (int i = 0; i < totalPairs; ++i)
            printf("%ld: %u\n", all[i].idx, all[i].val);
        free(all); free(counts); free(displs);
    }
    free(vec);
    MPI_Finalize();
    return 0;
}
