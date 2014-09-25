/* queue.h $Revision: 1.1 $ $Date: 2009/05/11 09:02:51 $
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
typedef struct q_node
{  struct q_node *next;
   void *p; } q_node;

typedef struct {
int count;
HANDLE nonempty;
q_node *first;
q_node *last; } queue;

extern void enqueue(queue *q, void *p);
extern void *dequeue(queue *q);
extern void *topqueue(queue *q);
extern int is_empty(queue *q);
extern void exit_queue(void);
extern void init_queue(void);
extern int queue_size(queue *q);
extern void wait_nonempty(queue *q);
extern void create_queue(queue *q);