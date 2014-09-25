/* broadcast.h $Revision: 1.1 $ $Date: 2009/05/11 09:02:51 $
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
extern void stop_bc(void);
extern void start_bc(void);
extern void broadcast_message(WPARAM wParam, LPARAM lParam);
extern void bc_exit();
extern void bc_init(void);
extern mp3_buffer get_empty_mp3buffer(void);
extern void bc_write(mp3_buffer data);

extern void receive_server_ip(WPARAM wParam, LPARAM lParam);
extern void display_send_status(LPARAM lparam);

/* lparam values for display_send_status */
#define BC_IDLE ((LPARAM)0)
#define BC_SENDING ((LPARAM)1)
#define BC_RECONNECT ((LPARAM)2)
#define BC_UPDATE ((LPARAM)3)

