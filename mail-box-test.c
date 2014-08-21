#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "mail-box.h"

#define ARRAY_SIZE(a)  (sizeof(a)/sizeof((a)[0]))

/* Thread context */
typedef ptrdiff_t tid;

#define ctx(id)  ((id) + NULL)
#define tid_of(ctx)  ((ctx) - NULL)

struct message {
	tid src;
	int id;
};

static struct mail_box *mb;

static void do_fake_work (double base)
{
	base *= 1e6;
	usleep (random () * (base / RAND_MAX) + base);
}

static void *worker (void *ctx)
{
	int i;
	struct message *m;

	for (errno = 0, i = 0; i < 10; ++i) {
		if ((m = malloc (sizeof (*m))) == NULL)
			break;

		m->src = tid_of (ctx);
		m->id = i;
		if (mail_box_put (mb, m) != 0)
			break;

		do_fake_work (0.03);
	}

	if (errno != 0)
		fprintf (stderr, "worker %td: error: %s\n", tid_of (ctx),
			 strerror (errno));
	return NULL;
}

static void *watcher (void *ctx)
{
	struct message *m;

	for (errno = 0;;) {
		if ((m = mail_box_get (mb)) == NULL)
			break;

		if (m->id < 0) {
			free (m);
			break;
		}

		printf ("message from %td, id = %d\n", m->src, m->id);
		free (m);

		do_fake_work (0.07);
	}

	if (errno != 0)
		fprintf (stderr, "watcher %td: error: %s\n", tid_of (ctx),
			 strerror (errno));
	return NULL;
}

int main (int argc, char *argv[])
{
	pthread_attr_t attr;
	pthread_t threads[4];
	int i;
	struct message *m;

	srandom (time (NULL));

	if ((mb = mail_box_create ()) == NULL) {
		perror ("mail_box");
		return 1;
	}

	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);

	pthread_create (&threads[0], &attr, watcher, ctx (1));
	pthread_create (&threads[1], &attr, worker,  ctx (2));
	pthread_create (&threads[2], &attr, worker,  ctx (3));
	pthread_create (&threads[3], &attr, worker,  ctx (4));

	pthread_attr_destroy (&attr);

	/* Wait for all worker threads to complete */
	for (i = 1; i < ARRAY_SIZE (threads); ++i)
		pthread_join (threads[i], NULL);

	if ((m = malloc (sizeof (*m))) == NULL) {
		perror ("main");
		exit (1);
	}

	m->src = 0;
	m->id = -1;
	mail_box_put (mb, m);
	pthread_join (threads[0], NULL);

	mail_box_destroy (mb);
	return 0;
}
