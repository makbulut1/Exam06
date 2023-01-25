#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/select.h>
#include <fcntl.h>

typdef struct s_client
{
	int	id;
	int	fd;
	char	*buffer;
	struct s_client	*next;
}	t_client;

fd_set	fd_all;
fd_set	fd_rd;
fd_set	fd_wr;



