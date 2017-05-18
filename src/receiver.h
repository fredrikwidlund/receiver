#ifndef RECEIVER_H_INCLUDED
#define RECEIVER_H_INCLUDED

typedef struct receiver receiver;
struct receiver
{
  int x;
};

int receiver_construct(receiver *);
int receiver_add(receiver *, char *);

#endif /* RECEIVER_H_INCLUDED */
