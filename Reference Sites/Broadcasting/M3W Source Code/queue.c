/* queue.c $Revision: 1.1 $ $Date: 2009/05/11 09:02:51 $
 * This File is part of m3w. 
 *
 *	M3W a mp3 streamer for the www
 *
 *	Copyright (c) 2001, 2002 Martin Ruckert (mailto:ruckertm@acm.org)
 * 
 * m3w is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * m3w is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with m3w; if not, write to the
 * Free Software Foundation, Inc., 
 * 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <windows.h>
#include <stdlib.h>
#include "queue.h"
#include "main.h"

static CRITICAL_SECTION queue_code;

static void critical_error(char *msg)
{  LeaveCriticalSection(&queue_code);
   fatal_error(msg);
}

static q_node *node_pool=NULL;
static struct q_node dummy_node;

static q_node *new_node(void)
{ q_node *n;
  if (node_pool == NULL)
  { n = malloc(sizeof(q_node));
    if (n == NULL)
    {  fatal_error("Out of Memory (q_node)");
       n = &dummy_node; }
  }
  else
  { n = node_pool;
    node_pool = n->next;
  }
  return n;
}

void create_queue(queue *q)
{ EnterCriticalSection(&queue_code);
  q->first = q->last = NULL;
  q->count = 0;
  q->nonempty = CreateEvent(NULL,TRUE,FALSE,NULL);
  if (q->nonempty==NULL)
  {	critical_error("Unable to create queue");
    return;
  }
  LeaveCriticalSection(&queue_code);
}



void exit_queue(void)
{ q_node *n;
  EnterCriticalSection(&queue_code);
  while(node_pool != NULL)
  { n= node_pool;
    node_pool = n->next;
    free(n);
  }
  LeaveCriticalSection(&queue_code);
}
    
void init_queue(void)
{ InitializeCriticalSection(&queue_code);
}

static void old_node(q_node *n)
{ n->next = node_pool;
  node_pool = n;
}

void enqueue(queue *q, void *p)
{ q_node *n;
  EnterCriticalSection(&queue_code);
  n = new_node();
  n->p = p;
  n->next = NULL;
  if (q->last == NULL)
  {  q->first = n;
     SetEvent(q->nonempty);
  }
  else
    q->last->next = n;
  q->last = n;
  q->count++;
  LeaveCriticalSection(&queue_code);
}


void *dequeue(queue *q)
{ void *p;
  q_node *n;
  EnterCriticalSection(&queue_code);
  n = q->first;
  if (n == NULL)
  { LeaveCriticalSection(&queue_code);
    errormsg("Dequeue from empty queue",0);
    return NULL;
  }
  p = n->p;
  q->first = n->next;
  if (q->first == NULL) 
  { q->last = NULL;
    ResetEvent(q->nonempty);
  }
  old_node(n);
  q->count--;
  LeaveCriticalSection(&queue_code);
  return p;
}

void *topqueue(queue *q)
{ q_node *n;
  void *p;
  EnterCriticalSection(&queue_code);
  n = q->first;
  if (n == NULL)
  {  critical_error("Access to empty queue");
     return NULL;
  }
  p= n->p;
  LeaveCriticalSection(&queue_code);
  return p;
}


int is_empty(queue *q)
{ int ret;
  EnterCriticalSection(&queue_code);
  ret =q->first == NULL; 
  LeaveCriticalSection(&queue_code);
  return ret;
}

int queue_size(queue *q)
{ int ret;
  EnterCriticalSection(&queue_code);
  ret = q->count; 
  LeaveCriticalSection(&queue_code);
  return ret;
}

void wait_nonempty(queue *q)
{ 
  WaitForSingleObject(q->nonempty,INFINITE);
}
