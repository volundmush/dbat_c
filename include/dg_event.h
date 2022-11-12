/* ************************************************************************
*  File: dg_event.h                                                       *
*                                                                         *
*  Usage: structures and prototypes for events                            *
*                                                                         *
*  Written by Eric Green (ejg3@cornell.edu)                               *
*                                                                         *
*  Changes:                                                               *
*      3/6/98 ejg:  Changed return type of EVENTFUNC from void to long.   *
*                   Moved struct event definition to events.c.            *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
************************************************************************ */
#ifndef __DG_EVENT_H__
#define __DG_EVENT_H__

#include "structs.h"


/*
** how often will heartbeat() call the 'wait' event function?
*/
#define PULSE_DG_EVENT 1


/********** Event related section *********/

#define EVENTFUNC(name) long (name)(void *event_obj)


/*
** define event related structures
*/
struct event {
  EVENTFUNC(*func);
  void *event_obj;
  struct q_element *q_el;
};

/****** End of Event related info ********/

/***** Queue related info ******/

/* number of queues to use (reduces enqueue cost) */
#define NUM_EVENT_QUEUES    10

struct queue {
  struct q_element *head[NUM_EVENT_QUEUES], *tail[NUM_EVENT_QUEUES];
};

struct q_element {
  void *data;
  long key;
  struct q_element *prev, *next;
};
/****** End of Queue related info ********/

/* - events - function protos need by other modules */
extern void event_init(void);
extern struct event *event_create(EVENTFUNC(*func), void *event_obj, long when);
extern void event_cancel(struct event *event);
extern void event_process(void);
extern long event_time(struct event *event);
extern void event_free_all(void);

/* - queues - function protos need by other modules */
extern struct queue *queue_init(void);
extern struct q_element *queue_enq(struct queue *q, void *data, long key);
extern void queue_deq(struct queue *q, struct q_element *qe);
extern void *queue_head(struct queue *q);
extern long queue_key(struct queue *q);
extern long queue_elmt_key(struct q_element *qe);
extern void queue_free(struct queue *q);
extern int  event_is_queued(struct event *event);

#endif