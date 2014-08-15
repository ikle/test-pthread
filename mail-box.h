#ifndef _MAIL_BOX_H
#define _MAIL_BOX_H 1

struct mail_box *mail_box_create (void);
int mail_box_destroy (struct mail_box *mb);

int mail_box_put (struct mail_box *mb, void *data);
void *mail_box_get (struct mail_box *mb);

#endif  /* _MAIL_BOX_H */
