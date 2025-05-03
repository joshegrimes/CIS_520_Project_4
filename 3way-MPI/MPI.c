#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    // starts MPI runtime
    MPI_Init(&argc, &argv);
    int rank, nprocs;

    // retrieve's this process's rank (i.e. its unique ID from 0 to nprocs -1)
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // gets the total number of processes in MPI_COMM_WORLD
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    // Ensures there is only one argument other than the executable: the file path
    if(argc != 2)
    {
        // only rank 0 prints this message, to avoid duplicates
        if(!rank)
            fprintf(stderr, "Usage: %s <file>\n", argv[0]);

        // cleanly shuts down and returns
        MPI_Finalize();
        return 1;
    }

    // retrieves file name from the given arguments
    const char * fname = argv[1];

    // inits file size to 0
    MPI_Offset fsize = 0;

    // only do this with rank 0
    if(!rank)
    {
	// opens the file in binary mode, seeks to the end, and retrieves the file size in bytes -
        // calls MPI_Abort if there's an error opening the file so that all ranks exit
        FILE *fs = fopen(fname, "rb");
        if(!fs)
        {
            perror("fopen");
            MPI_Abort(MPI_COMM_WORLD, 2);
        }
        fseek(fs, 0, SEEK_END);
        fsize = ftell(fs);
        fclose(fs);
    }
    // broadcasts the file size to all ranks
    MPI_Bcast(&fsize, 1, MPI_OFFSET, 0, MPI_COMM_WORLD);

    // divides the file into roughly equal chunks and each rank computes its own begin and end byte
    // offsets into the file - and the bytes variable is the number of bytes this rank will actually
    // read, which could be zero for small remainders
    MPI_Offset chunk = (fsize + nprocs - 1) / nprocs;
    MPI_Offset begin = rank * chunk;
    MPI_Offset end = (begin + chunk > fsize) ? fsize : begin + chunk;
    MPI_Offset bytes = (end > begin) ? end - begin : 0;

    // opens file fh on all ranks in read-only mode
    MPI_File fh;
    MPI_File_open(MPI_COMM_WORLD, fname, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);

    // allocates a local buffer of size bytes and each rank reads its chunk simultaneously at the given
    // offset 'begin'. MPI_CHAR tells MPI to treat each of the byte elements in the buffer as one char.
    char *buf = (bytes ? malloc(bytes) : NULL);
    MPI_File_read_at_all(fh, begin, buf, bytes, MPI_CHAR, MPI_STATUS_IGNORE);
    // closes the MPI file handle
    MPI_File_close(&fh);

    // because we split by byte-offsets, non-zero ranks may start in the middle of a line; each non-zero
    // line continues until it sees its first newline character, then starts line processing just after
    // that newline; rank 0 starts at an offset of 0 (i.e. the beginning)
    MPI_Offset off = 0;
    if(rank)
    {
        while(off < bytes && buf[off] != '\n')
            ++off;
        if(off < bytes)
            ++off;
    }

    // vals is a dynamically growing array (starting at 1024) that will store each line's maximum printable
    // ASCII code
    unsigned char *vals = NULL;
    size_t cap = 1024, n = 0;
    if(bytes)
    {
        vals = malloc(cap);
    }

    // loop from 'off' to bytes'; if the character isn't newline, update cur if it's a printable ASCII (32-116)
    // and larger than the current max, on a newline append 'cur' to the 'vals' array, reset cur to 0, and continue;
    // after the loop check for a final line without trailing newline and append it if needed; lastly free the read buffer
    unsigned char cur = 0;
    for(MPI_Offset i = off; i < bytes; ++i)
    {
        unsigned char c = (unsigned char)buf[i];
        if(c != '\n')
        {
            if(c >= 32 && c <= 126 && c > cur)
                cur = c;
        }
        else
        {
            if(n == cap)
            {
                cap <<= 1;
                vals = realloc(vals, cap);
            }
            vals[n++] = cur;
            cur = 0;
        }
    }
    // final check for leftover line
    if(cur != 0 || (bytes > 0 && buf[bytes-1] != '\n'))
    {
         if(n == cap)
         {
             cap <<= 1;
             vals = realloc(vals, cap);
         }
         vals[n++] = cur;
    }
    free(buf);

    // each rank knows how many lines it extracted (myCount = n); rank 0 allocates an array
    // counts[p] to receive all those counts; MPI_Gather collects each rank's line count into
    // counts[] on rank 0
    int myCount = (int)n;
    int *counts = NULL, *displs = NULL;
    if(!rank)
    {
        counts = malloc(nprocs * sizeof *counts);
    }

    MPI_Gather(&myCount, 1, MPI_INT, counts, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // on rank 0 we build a displs[] array of byte-displacements so each rank's set of myCount values
    // lands in the right place in the output array; we allocate allvals large enough to hold the sum
    // of all counts; MPI_Gatherv then collects each rank's vals[] into allvals[] on rank 0
    unsigned char *allvals = NULL;
    if(!rank)
    {
        displs = malloc(nprocs * sizeof *displs);
        displs[0] = 0;
        for (int p = 1; p < nprocs; ++p)
            displs[p] = displs[p-1] + counts[p-1];

        allvals = malloc(displs[nprocs-1] + counts[nprocs-1]);
    }

    MPI_Gatherv(vals, myCount, MPI_UNSIGNED_CHAR, allvals, counts, displs, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // on rank 0, printing the final output and cleaning up by freeing memory
    if(!rank)
    {
        long idx = 0;
        long total = displs[nprocs-1] + counts[nprocs-1];
        for (; idx < total; ++idx)
            printf("%ld: %u\n", idx, allvals[idx]);

        free(allvals);
        free(counts);
        free(displs);
    }
    free(vals);

    // shuts down the MPI environment cleanly
    MPI_Finalize();
    return 0;
}

