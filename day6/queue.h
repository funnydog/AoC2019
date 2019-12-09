#ifndef QUEUE_H
#define QUEUE_H

#define QUEUE_SIZE 32

struct queue
{
	struct vertex *data[QUEUE_SIZE];
	unsigned r, w;
};

static inline void queue_init(struct queue *q)
{
	q->r = q->w = 0;
}

static inline int queue_full(struct queue *q)
{
	return q->r + QUEUE_SIZE == q->w;
}

static inline int queue_empty(struct queue *q)
{
	return q->r == q->w;
}

static inline void queue_add(struct queue *q, struct vertex *v)
{
	q->data[q->w & (QUEUE_SIZE-1)] = v;
	q->w++;
}

static inline struct vertex *queue_pop(struct queue *q)
{
	struct vertex *v = q->data[q->r & (QUEUE_SIZE-1)];
	q->r++;
	return v;
}

#endif
