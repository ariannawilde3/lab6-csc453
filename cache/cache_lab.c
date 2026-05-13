/*
 * cache_lab.c  -  Memory Hierarchy and Cache Effects
 *
 * Four experiments that reveal how caches, cache lines, and data layout
 * affect real-world performance.
 *
 * Compile:  gcc -std=c99 -O2 -Wall -Wextra -pthread -o cache_lab cache_lab.c
 *
 * TODOs are marked below.  Search for "TODO" to find them.
 */
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

/* Typical x86-64 cache line size; verify with: getconf LEVEL1_DCACHE_LINESIZE */
#define CACHE_LINE_SIZE 64

/* Each timed section is repeated REPEAT times and averaged */
#define REPEAT 3

/* ================================================================
 * Timing helper
 * ================================================================ */

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ================================================================
 * Experiment 1 - Memory Hierarchy Discovery  (fully provided)
 *
 * Iterate through arrays of increasing size with both sequential
 * and random access patterns.  Measure nanoseconds per element.
 * ================================================================ */

/* Fisher-Yates shuffle */
static void shuffle(int *a, size_t n)
{
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rand() % (i + 1);
        int tmp  = a[i];
        a[i]     = a[j];
        a[j]     = tmp;
    }
}

static void experiment1(void)
{
    printf("=== Experiment 1: Memory Hierarchy Discovery ===\n\n");

    size_t sizes[] = {
        8u   * 1024,                /*   8 KB  - fits in L1           */
        32u  * 1024,                /*  32 KB  - ~L1 boundary         */
        256u * 1024,                /* 256 KB  - fits in L2           */
        2u   * 1024 * 1024,         /*   2 MB  - L2/L3 boundary      */
        16u  * 1024 * 1024,         /*  16 MB  - fits in L3           */
        64u  * 1024 * 1024          /*  64 MB  - exceeds most L3s    */
    };
    int nsizes = (int)(sizeof(sizes) / sizeof(sizes[0]));

    printf("%-10s  %15s  %15s  %10s\n",
           "Size", "Sequential", "Random", "Ratio");
    printf("%-10s  %15s  %15s  %10s\n",
           "----", "----------", "------", "-----");

    for (int s = 0; s < nsizes; s++) {
        size_t n   = sizes[s] / sizeof(int);
        int   *arr = malloc(sizes[s]);
        int   *idx = malloc(n * sizeof(int));
        if (!arr || !idx) { perror("malloc"); free(arr); free(idx); continue; }

        /* initialise data and shuffled index array */
        for (size_t i = 0; i < n; i++) {
            arr[i] = (int)(i & 0xFF);
            idx[i] = (int)i;
        }
        shuffle(idx, n);

        /* ensure at least 10 M element-touches for stable timing */
        size_t min_touches = 10u * 1024 * 1024;
        size_t passes      = min_touches / n;
        if (passes < 1) passes = 1;
        size_t total = passes * n;

        /*
         * Cast to volatile so the compiler cannot auto-vectorise,
         * collapse repeated passes, or eliminate loads.
         * Each load must actually hit the memory hierarchy.
         */
        volatile int *varr = (volatile int *)arr;

        /* --- sequential --- */
        long   sum = 0;
        double t0  = now_sec();
        for (int r = 0; r < REPEAT; r++)
            for (size_t p = 0; p < passes; p++)
                for (size_t i = 0; i < n; i++)
                    sum += varr[i];
        double seq_ns = ((now_sec() - t0) / REPEAT / total) * 1e9;

        /* --- random --- */
        sum = 0;
        t0  = now_sec();
        for (int r = 0; r < REPEAT; r++)
            for (size_t p = 0; p < passes; p++)
                for (size_t i = 0; i < n; i++)
                    sum += varr[idx[i]];
        double rnd_ns = ((now_sec() - t0) / REPEAT / total) * 1e9;

        /* prevent dead-code elimination */
        if (sum == -999) printf("!");

        char label[32];
        if (sizes[s] >= 1024 * 1024)
            snprintf(label, sizeof(label),"%zu MB", sizes[s] / (1024*1024));
        else
            snprintf(label, sizeof(label),"%zu KB", sizes[s] / 1024);

        printf("%-10s  %12.2f ns  %12.2f ns  %9.1fx\n",
               label, seq_ns, rnd_ns, rnd_ns / seq_ns);

        free(arr);
        free(idx);
    }
    printf("\n");
}

/* ================================================================
 * Experiment 2 - Spatial Locality  (Row-Major vs Column-Major)
 *
 * Row-major is provided.  You implement column-major.
 * ================================================================ */

#define MSIZE 4096          /* 4096 x 4096 ints  =  64 MB */

static void experiment2(void)
{
    printf("=== Experiment 2: Spatial Locality (Row vs Column Major) ===\n\n");

    int *m = malloc((size_t)MSIZE * MSIZE * sizeof(int));
    if (!m) { perror("malloc"); return; }

    for (int i = 0; i < MSIZE; i++)
        for (int j = 0; j < MSIZE; j++)
            m[i * MSIZE + j] = 1;

    /* volatile prevents the compiler from collapsing/vectorising */
    volatile int *mv = (volatile int *)m;

    /* --- row-major (provided) --- */
    long   sum = 0;
    double t0  = now_sec();
    for (int r = 0; r < REPEAT; r++) {
        sum = 0;
        for (int i = 0; i < MSIZE; i++)
            for (int j = 0; j < MSIZE; j++)
                sum += mv[i * MSIZE + j];
    }
    double row_t = (now_sec() - t0) / REPEAT;
    printf("Row-major:    sum = %ld   %.4f sec\n", sum, row_t);

    /* --- column-major --- */
    /*
     * TODO: Traverse the same matrix in column-major order.
     *
     * Column-major means you visit ALL rows of column 0, then ALL rows
     * of column 1, etc.  The outer loop should iterate over columns (j)
     * and the inner loop over rows (i).
     *
     * Use the same timing / repeat structure as the row-major version
     * above.  Read through mv[] (the volatile pointer), not m[].
     *
     * Store the elapsed time in col_t.
     */
    double col_t = 0.0;
    /* ---- YOUR CODE HERE ---- */

    t0 = now_sec();
    for (int r = 0; r < REPEAT; r++) {
        sum = 0;

        for (int j = 0; j < MSIZE; j++) {
            for(int i = 0; i < MSIZE; i++) {
                sum += mv[i * MSIZE + j];
            }
        }
    }

    col_t = (now_sec() - t0) /  REPEAT;

    /* ---- END YOUR CODE ---- */


    printf("Column-major: sum = %ld   %.4f sec\n", sum, col_t);

    printf("Column-major is %.1fx slower\n\n",
           col_t > 0 ? col_t / row_t : 0.0);

    free(m);
}

/* ================================================================
 * Experiment 3 - False Sharing
 * ================================================================ */

#define MAX_THREADS 64

/* --- Unpadded: all counters packed together (provided) --- */
struct counters_unpadded {
    long count[MAX_THREADS];
};

/*
 * --- Padded: each counter on its own cache line ---
 *
 * TODO: Add a padding field so that sizeof(struct padded_slot) ==
 *       CACHE_LINE_SIZE.  The 'count' field is sizeof(long) bytes,
 *       so you need CACHE_LINE_SIZE - sizeof(long) bytes of padding.
 *
 *       Use a char array for the padding (e.g. char _pad[...]).
 *
 * When you are done, the distance printed by exp3_print_layout()
 * between adjacent padded counters should equal CACHE_LINE_SIZE.
 */
struct padded_slot {
    long count;
    char _pad[CACHE_LINE_SIZE - sizeof(long)];
};

struct counters_padded {
    struct padded_slot slot[MAX_THREADS];
};

/* Thread argument block */
struct exp3_args {
    int   id;
    long  iters;
    int   padded;       /* 0 = unpadded, 1 = padded */
    void *counters;
};

static void *exp3_worker(void *arg)
{
    struct exp3_args *a = (struct exp3_args *)arg;

    if (a->padded) {
        /*
         * TODO: Implement the padded counter increment loop.
         *
         * This should look very similar to the unpadded case below,
         * but you access the counter through struct counters_padded
         * and its slot[] array instead.
         *
         * Steps:
         *   1. Cast a->counters to (struct counters_padded *)
         *   2. Get a volatile long * to your slot's count field
         *   3. Increment it a->iters times in a loop
         *
         * The volatile pointer ensures each increment actually
         * touches the cache (rather than being kept in a register).
         */
        /* ---- YOUR CODE HERE ---- */

        struct counters_padded *c = (struct counters_padded *)a -> counters;
        volatile long *ctr = &c -> slot[a -> id].count;
        for (long i = 0; i < a -> iters; i++) {
            (*ctr)++;
        }

        /* ---- END YOUR CODE ---- */
    } else {
        /* Unpadded case (provided) */
        struct counters_unpadded *c = (struct counters_unpadded *)a->counters;
        volatile long *ctr = &c->count[a->id];
        for (long i = 0; i < a->iters; i++)
            (*ctr)++;
    }
    return NULL;
}

/* Print the memory layout of counters so students can see addresses */
static void exp3_print_layout(int nthreads)
{
    struct counters_unpadded u;
    struct counters_padded   p;
    memset(&u, 0, sizeof(u));
    memset(&p, 0, sizeof(p));

    int show = nthreads < 8 ? nthreads : 8;

    printf("Counter memory layout (%d counters shown):\n\n", show);

    printf("  Unpadded (sizeof each slot = %zu bytes):\n", sizeof(long));
    for (int i = 0; i < show; i++)
        printf("    counter[%d] at %p\n", i, (void *)&u.count[i]);
    printf("    -> distance between [0] and [1]: %zu bytes\n\n",
           (size_t)((char *)&u.count[1] - (char *)&u.count[0]));

    printf("  Padded   (sizeof each slot = %zu bytes):\n",
           sizeof(struct padded_slot));
    for (int i = 0; i < show; i++)
        printf("    counter[%d] at %p\n", i, (void *)&p.slot[i].count);
    printf("    -> distance between [0] and [1]: %zu bytes\n\n",
           (size_t)((char *)&p.slot[1].count - (char *)&p.slot[0].count));
}

static void exp3_run(int nthreads, long iters, int padded)
{
    pthread_t        thr[MAX_THREADS];
    struct exp3_args args[MAX_THREADS];

    void *counters;
    if (padded)
        counters = calloc(1, sizeof(struct counters_padded));
    else
        counters = calloc(1, sizeof(struct counters_unpadded));
    if (!counters) { perror("calloc"); return; }

    double t0 = now_sec();
    for (int i = 0; i < nthreads; i++) {
        args[i].id       = i;
        args[i].iters    = iters;
        args[i].padded   = padded;
        args[i].counters = counters;
        pthread_create(&thr[i], NULL, exp3_worker, &args[i]);
    }
    for (int i = 0; i < nthreads; i++)
        pthread_join(thr[i], NULL);
    double elapsed = now_sec() - t0;

    /* verify totals */
    long total = 0;
    for (int i = 0; i < nthreads; i++) {
        if (padded)
            total += ((struct counters_padded *)counters)->slot[i].count;
        else
            total += ((struct counters_unpadded *)counters)->count[i];
    }

    printf("  %2d thread(s)  %-10s  %7.4f s   %8.2f M inc/s   total=%ld\n",
           nthreads, padded ? "padded" : "unpadded",
           elapsed, total / elapsed / 1e6, total);

    free(counters);
}

static void experiment3(void)
{
    printf("=== Experiment 3: False Sharing ===\n\n");

    exp3_print_layout(4);

    long iters   = 10L * 1000 * 1000;          /* 10 M per thread */
    int  counts[] = {1, 2, 4, 8};
    int  nc = (int)(sizeof(counts) / sizeof(counts[0]));

    for (int t = 0; t < nc; t++) {
        exp3_run(counts[t], iters, 0);  /* unpadded */
        exp3_run(counts[t], iters, 1);  /* padded   */
        printf("\n");
    }
}

/* ================================================================
 * Experiment 4 - Array of Structs (AoS)  vs  Struct of Arrays (SoA)
 * ================================================================ */

#define NUM_PARTICLES (4 * 1024 * 1024)         /* 4 M elements */

/* Array of Structs */
struct particle {
    float x, y, z;
    float vx, vy, vz;
    float mass;
    int   id;
    char  _pad[24];     /* inflate to 64 bytes so struct is cache-line sized */
};

/* Struct of Arrays */
struct particles_soa {
    float *x,  *y,  *z;
    float *vx, *vy, *vz;
    float *mass;
    int   *id;
};

static void experiment4(void)
{
    printf("=== Experiment 4: Array of Structs vs Struct of Arrays ===\n\n");

    size_t n = NUM_PARTICLES;

    /* ---- AoS (provided) ---- */
    struct particle *aos = malloc(n * sizeof(*aos));
    if (!aos) { perror("malloc"); return; }

    for (size_t i = 0; i < n; i++) {
        float v     = (float)(i % 1000) * 0.1f;
        aos[i].x    = v;  aos[i].y  = v;  aos[i].z  = v;
        aos[i].vx   = v;  aos[i].vy = v;  aos[i].vz = v;
        aos[i].mass = v + 1.0f;
        aos[i].id   = (int)i;
    }

    /* ---- SoA ---- */
    /*
     * TODO: Allocate all 8 arrays in the particles_soa struct.
     *       Each array should hold n elements of the appropriate type
     *       (float for x/y/z/vx/vy/vz/mass, int for id).
     *
     * TODO: Initialise the SoA arrays with the same values as the
     *       AoS version above (use the same formula).
     */
    struct particles_soa soa;
    memset(&soa, 0, sizeof(soa));
    /* ---- YOUR ALLOCATION CODE HERE ---- */

    soa.x = malloc(n * sizeof(float));
    soa.y = malloc(n * sizeof(float));
    soa.z = malloc(n * sizeof(float));
    soa.vx = malloc(n * sizeof(float));
    soa.vy = malloc(n * sizeof(float));
    soa.vz = malloc(n * sizeof(float));
    soa.mass = malloc(n * sizeof(float));
    soa.id = malloc(n * sizeof(int));
    if (!soa.x) {
        perror("malloc");
        return;
    }

    /* ---- YOUR INITIALISATION CODE HERE ---- */

    for (size_t i = 0; i < n; i++) {
        float v = (float)(i % 1000) * 0.1f;
        soa.x[i] = v;
        soa.y[i] = v;
        soa.z[i] = v;
        soa.vx[i] = v;
        soa.vy[i] = v;
        soa.vz[i] = v;
        soa.mass[i] = v + 1.0f;
        soa.id[i] = (int)i;
    }

    /* ---- END YOUR CODE ---- */

    /* ---- AoS: sum only x (provided) ---- */
    /* stride between consecutive .x fields = sizeof(struct particle) = 64 B */
    volatile double sink = 0.0;
    double sum = 0.0;
    double t0  = now_sec();
    for (int r = 0; r < REPEAT; r++) {
        sum = 0.0;
        for (size_t i = 0; i < n; i++)
            sum += (double)aos[i].x;
        sink += sum;     /* prevent the compiler from collapsing repeats */
    }
    double aos_t = (now_sec() - t0) / REPEAT;
    printf("AoS  sum(x) = %.2f    %.4f sec\n", sum, aos_t);

    /* ---- SoA: sum only x ---- */
    /*
     * TODO: Sum only soa.x[0..n-1] into 'sum', using the same timing /
     *       repeat structure as the AoS version above.
     *       Store the elapsed time in soa_t.
     *
     *       Remember to use  sink += sum;  after each repeat to prevent
     *       the compiler from collapsing the loop.
     */
    double soa_t = 0.0;
    /* ---- YOUR CODE HERE ---- */

    t0 = now_sec();
    for (int r = 0; r < REPEAT; r++) {
        sum = 0.0;
        /* soa.x[] is contiguous, so every cache line holds 16 consecutive
         * floats. one cache line fetch = 16 useful reads, not 1. */
        for (size_t i = 0; i < n; i++) {
            sum += (double)soa.x[i];
        }
            
        sink += sum;
    }
    soa_t = (now_sec() - t0) / REPEAT;

    /* ---- END YOUR CODE ---- */
    printf("SoA  sum(x) = %.2f    %.4f sec\n", sum, soa_t);
    (void)sink;

    printf("AoS is %.1fx slower\n\n",
           soa_t > 0 ? aos_t / soa_t : 0.0);

    free(aos);
    /* TODO: Free all SoA arrays */
    free(soa.x);  free(soa.y);  free(soa.z);
    free(soa.vx); free(soa.vy); free(soa.vz);
    free(soa.mass); free(soa.id);
}

/* ================================================================
 * main
 * ================================================================ */

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s -e <experiment>\n"
            "  -e 0   Run all experiments\n"
            "  -e 1   Memory hierarchy discovery\n"
            "  -e 2   Spatial locality (row vs column major)\n"
            "  -e 3   False sharing\n"
            "  -e 4   Array of Structs vs Struct of Arrays\n",
            prog);
}

int main(int argc, char *argv[])
{
    int exp = -1;
    int opt;

    while ((opt = getopt(argc, argv, "e:h")) != -1) {
        switch (opt) {
        case 'e': exp = atoi(optarg); break;
        default:  usage(argv[0]); return 1;
        }
    }
    if (exp < 0) { usage(argv[0]); return 1; }

    srand(42);  /* fixed seed for reproducibility */

    printf("Cache Lab - Memory Hierarchy Experiments\n");
    printf("=========================================\n\n");

    switch (exp) {
    case 0:
        experiment1();
        experiment2();
        experiment3();
        experiment4();
        break;
    case 1: experiment1(); break;
    case 2: experiment2(); break;
    case 3: experiment3(); break;
    case 4: experiment4(); break;
    default:
        usage(argv[0]);
        return 1;
    }

    return 0;
}
