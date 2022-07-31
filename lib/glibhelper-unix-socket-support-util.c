/**
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file	glibhelper-unix-socket-support-util.c
 * @brief	util for unix domain socket seq packet helper.
 */
#include <glib.h>
#include <gio/gio.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#include "glibhelper-unix-socket-support-util.h"

/**
 * Socket buffer size calculator
 *
 * @param [in]	packet_max_size	Max bytes of packet.
 * @param [in]	packet_max_num	Number of maximum queuing packet.
 *
 * @return int
 * @retval >~0 Calculated socket buffer size.
 * @retval <0 Larger than system limit size.
 */
int glibhelper_calculate_socket_buffer_size(int packet_max_size, int packet_max_num)
{
	int buffsize = (packet_max_size * packet_max_num * 2) / 3;

	if ( buffsize < 2048 )
		buffsize = 2048;

	if ( buffsize > 2000000)
		buffsize = -1;

	return buffsize;
}
/**
 * Getter for the data pool service socket name type.
 *
 * @return int	 0	Socket file
 * @return int	 >0	Abstract socket and that name length
 * @return int	 <0	Arg error
 */
int glibhelper_get_socket_name_type(char *str)
{
	int ret = 0;

	if (str == NULL)
		return -1;

	if (str[0] == '\0') {
		int bytes = 1;
		for (bytes = 1; bytes < 92; bytes++) { //92 is a max length of socket name in linux
			if (str[bytes] == '\0')
				break;
		}
		ret = bytes;
	} else 
		ret = 0;

	return ret;
}
