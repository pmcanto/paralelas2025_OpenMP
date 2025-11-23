CC = gcc
CFLAGS = -Wall -O3 -fopenmp
SRCS_COMMON = hash_table.c

all: seq critical atomic

seq: analyzer_seq.c $(SRCS_COMMON)
	$(CC) $(CFLAGS) analyzer_seq.c $(SRCS_COMMON) -o analyzer_seq

critical: analyzer_par_critical.c $(SRCS_COMMON)
	$(CC) $(CFLAGS) analyzer_par_critical.c $(SRCS_COMMON) -o analyzer_par_critical

atomic: analyzer_par_atomic.c $(SRCS_COMMON)
	$(CC) $(CFLAGS) analyzer_par_atomic.c $(SRCS_COMMON) -o analyzer_par_atomic

clean:
	rm -f analyzer_seq analyzer_par_critical analyzer_par_atomic results.csv