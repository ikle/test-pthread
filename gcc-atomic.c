#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

#define COUNT  10000000

#ifndef __GNUC__
#error GNU C compiler required for this test program
#else

#define GCC_VERSION	(__GNUC__	* 10000	+	\
			 __GNUC_MINOR__	* 100	+	\
			 __GNUC_PATCHLEVEL__)

#if GCC_VERSION < 40100
#error At least GNU C compiler version 4.1 required
#elif GCC_VERSION >= 40700
/* Atomic increment need not be ordered */
#define atomic_inc(c)	__atomic_add_fetch (&(c), 1, __ATOMIC_RELAXED)
#else
#define atomic_inc(c)	__sync_add_and_fetch (&(c), 1);
#endif

#endif  /* __GNUC__ */

#define ARRAY_SIZE(a)  (sizeof(a)/sizeof((a)[0]))

/* Shared state */
static volatile int counter = 0;

static void *worker (void *ctx)
{
	int i;

	for (i = 0; i < COUNT; ++i)
		atomic_inc (counter);

	return NULL;
}

int main (int argc, char *argv[])
{
	pthread_attr_t attr;
	pthread_t threads[2];
	struct timeval start, stop;
	int i;

	/* For portability, explicitly create threads in a joinable state */
	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);

	if (gettimeofday (&start, NULL) != 0)
		goto fatal;

	pthread_create (&threads[0], &attr, worker, NULL);
	pthread_create (&threads[1], &attr, worker, NULL);

	/* Wait for all worker threads to complete */
	for (i = 0; i < ARRAY_SIZE (threads); ++i)
		pthread_join (threads[i], NULL);

	/* NOTE: full memory barrier implied by pthread_join */

	if (gettimeofday (&stop, NULL) != 0)
		goto fatal;

	stop.tv_sec -= start.tv_sec;
	if (stop.tv_usec < start.tv_usec)
		--stop.tv_sec, stop.tv_usec += 1000000L;
	stop.tv_usec -= start.tv_usec;

	printf ("result %s, time elapsed: %d.%06d\n",
		(COUNT * ARRAY_SIZE (threads) == counter) ?
			"correct" : "incorrect",
		stop.tv_sec, stop.tv_usec);

	pthread_attr_destroy (&attr);
	return 0;
fatal:
	perror ("gcc-atomic");
	return 1;
}
