#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define ARRAY_SIZE(a)  (sizeof(a)/sizeof((a)[0]))

/* Thread context */
typedef ptrdiff_t tid;

#define ctx(id)  ((id) + NULL)
#define tid_of(ctx)  ((ctx) - NULL)

/* Shared state */
static int count = 0, stop = 0;
static pthread_mutex_t lock;
static pthread_cond_t cond;

static void print_state (const char *who, tid id, const char *comment)
{
	printf ("thread %td (%s), count = %d", id, who, count);
	if (comment != NULL)
		printf (": %s", comment);
	putchar ('\n');
}

#define notify(comment)  (print_state (__func__, tid_of (ctx), (comment)))

static void *worker (void *ctx)
{
	int i;

	pthread_mutex_lock (&lock);
	notify ("starting");

	for (i = 0; i < 10; ++i) {  /* Perform several cycles */

		notify ("update count");
		++count;

		/* Send signal on some condition */
		if ((count % 7) == 0) {
			notify ("threshold reached, sending signal");
			pthread_cond_signal (&cond);
		}

		pthread_mutex_unlock (&lock);
		sleep (1);  /* Do some fake work */
		pthread_mutex_lock (&lock);
	}

	notify ("exiting");
	pthread_mutex_unlock (&lock);
	return NULL;
}

static void *watcher (void *ctx)
{
	pthread_mutex_lock (&lock);
	notify ("starting");

	for (;; count += 100) {
		notify ("wait for signal");
		pthread_cond_wait (&cond, &lock);
		notify ("signal received");
		if (stop)
			break;
	}

	notify ("exiting");
	pthread_mutex_unlock (&lock);
	return NULL;
}

int main (int argc, char *argv[])
{
	pthread_attr_t attr;
	pthread_t threads[3];
	int i;

	pthread_mutex_init (&lock, NULL);
	pthread_cond_init (&cond, NULL);

	/* For portability, explicitly create threads in a joinable state */
	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);

	pthread_create (&threads[0], &attr, watcher, ctx (1));
	pthread_create (&threads[1], &attr, worker,  ctx (2));
	pthread_create (&threads[2], &attr, worker,  ctx (3));

	/* Wait for all worker threads to complete */
	for (i = 1; i < ARRAY_SIZE (threads); ++i)
		pthread_join (threads[i], NULL);

	/* Wait watcher to complete */
	stop = 1;
	pthread_cond_signal (&cond);
	pthread_join (threads[0], NULL);

	pthread_attr_destroy (&attr);
	pthread_cond_destroy (&cond);
	pthread_mutex_destroy (&lock);
	return 0;
}
