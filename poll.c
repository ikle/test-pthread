#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <poll.h>
#include <pthread.h>
#include <signal.h>

#define SIGNAL  SIGRTMIN

/* Shared state */
static pthread_mutex_t lock;
//static pthread_cond_t cond;

static void signal_handler (int no) {}

static void *worker (void *ctx)
{
	pthread_t to = ctx - NULL;
	int i;

	pthread_mutex_lock (&lock);
	fprintf (stderr, "worker: enter\n");
	pthread_mutex_unlock (&lock);

	for (i = 0; i < 10; ++i) {
		sleep (1);  /* Do some fake work */

		pthread_mutex_lock (&lock);
		fprintf (stderr, "worker: send signal\n");
		pthread_mutex_unlock (&lock);

//		pthread_cond_signal (&cond);
		pthread_kill (to, SIGNAL);
	}

	pthread_mutex_lock (&lock);
	fprintf (stderr, "worker: exit\n");
	pthread_mutex_unlock (&lock);
	return NULL;
}

static void *poller (void *ctx)
{
	int i;

	pthread_mutex_lock (&lock);
	fprintf (stderr, "poller: enter\n");
	pthread_mutex_unlock (&lock);

	for (i = 0; i < 10; ++i) {
		poll (NULL, 0, -1);

		pthread_mutex_lock (&lock);
		fprintf (stderr, "poller: interrupted\n");
		pthread_mutex_unlock (&lock);
	}

	pthread_mutex_lock (&lock);
	fprintf (stderr, "poller: exit\n");
	pthread_mutex_unlock (&lock);
	return NULL;
}

int main (int argc, char *argv[])
{
	struct sigaction sa = {};
	pthread_attr_t attr;
	pthread_t w, p;

	sa.sa_handler = signal_handler;

	if (sigaction (SIGNAL, &sa, NULL) < 0)
		perror ("sigaction");

	pthread_mutex_init (&lock, NULL);
//	pthread_cond_init (&cond, NULL);

	/* For portability, explicitly create threads in a joinable state */
	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);

	pthread_create (&p, &attr, poller, NULL);
	pthread_create (&w, &attr, worker, p + NULL);

	pthread_join (w, NULL);
	pthread_join (p, NULL);

	pthread_attr_destroy (&attr);
//	pthread_cond_destroy (&cond);
	pthread_mutex_destroy (&lock);
	return 0;
}
