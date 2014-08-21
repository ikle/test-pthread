#include "mail-box.h"
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

struct mail_box {
	pthread_mutex_t lock;
	pthread_cond_t cond;
	struct message *list;
};

struct message {
	struct message *next;
	void *data;
};

struct mail_box *mail_box_create (void)
{
	struct mail_box *mb;

	if ((mb = calloc (1, sizeof (*mb))) == NULL)
		goto no_mem;

	if ((errno = pthread_mutex_init (&mb->lock, NULL)) != 0)
		goto no_lock;

	if ((errno = pthread_cond_init (&mb->cond, NULL)) != 0)
		goto no_cond;

	mb->list = NULL;
	return mb;
no_cond:
	pthread_mutex_destroy (&mb->lock);
no_lock:
	free (mb);
no_mem:
	return NULL;
}

int mail_box_destroy (struct mail_box *mb)
{
	struct message *p, *next;

	if ((errno = pthread_mutex_destroy (&mb->lock)) != 0)
		return -1;

	/* condition variable and list "protected" by destoyed lock */
	pthread_cond_destroy  (&mb->cond);

	for (p = mb->list; p != NULL; p = next) {
		next = p->next;
		free (p);
	}

	free (mb);
	return 0;
}

void message_list_add (struct message **p, struct message *m)
{
	for (; *p != NULL; p = &(*p)->next) {}

	*p = m;
}

int mail_box_put (struct mail_box *mb, void *data)
{
	struct message *message;

	if ((message = malloc (sizeof (*message))) == NULL)
		goto no_mem;

	if ((errno = pthread_mutex_lock (&mb->lock)) != 0)
		goto no_lock;

	message->next = NULL;
	message->data = data;
	message_list_add (&mb->list, message);

	pthread_cond_signal (&mb->cond);

	pthread_mutex_unlock (&mb->lock);
	return 0;
no_lock:
	free (message);
no_mem:
	return -1;
}

void *mail_box_get (struct mail_box *mb)
{
	struct message *message;
	void *data;

	if ((errno = pthread_mutex_lock (&mb->lock)) != 0)
		return NULL;

	while (mb->list == NULL)
		pthread_cond_wait (&mb->cond, &mb->lock);

	message = mb->list;
	mb->list = message->next;

	data = message->data;
	free (message);

	pthread_mutex_unlock (&mb->lock);
	return data;
}
