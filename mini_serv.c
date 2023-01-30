#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/select.h>
#include <fcntl.h>

typedef struct s_client
{
	int	id;
	int	fd;
	char	*buffer;
	struct s_client	*next;
}	t_client;

fd_set	fd_all;
fd_set	fd_rd;
fd_set	fd_wr;

void	exit_error(char *str, t_client *client_lst)
{
	t_client	*tmp;
	
	tmp = client_lst;
	while (client_lst)
	{
		tmp = tmp->next;
		close(client_lst->fd);
		free(client_lst->buffer);
		free(client_lst);
		client_lst = tmp;
	}
	write(STDERR_FILENO, str, strlen(str));
	exit(1);
}

void	handle_server(int sockfd)
{
	int	connfd, maxfd, client_fd;
	t_client	*client_lst;

	FD_ZERO(&fd_all);
	FD_SET(sockfd, &fd_all);
	maxfd = sockfd;
	client_lst = 0;
	while (1)
	{
		fd_rd = fd_all;
		fd_wr = fd_all;
		if (select(maxfd + 1, &fd_rd, &fd_wr, NULL, NULL) < 0)
			continue ;
		else if (FD_ISSET(sockfd, &fd_rd))
		{
			connfd = accept(sockfd, NULL, NULL);
			if (connfd >= 0)
			{
			//	client_lst = add_newclient(client_lst, connfd, client_fd);
				maxfd = connfd > maxfd ? connfd : maxfd;
				++client_fd;
			}
		}
		//else
		//	client_lst = handle_client(client_lst);
	}
}
int main(int ac, char **av)
{
	int	port, sockfd;
	struct	sockaddr_in	servaddr;
	
	if (ac < 2)
		exit_error("Wrong number of arguments\n", NULL);
	port = atoi(av[1]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		exit_error("Fatal Error\n", NULL);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(port);
	if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)))
		exit_error("Fatal Error\n", NULL);
	if (listen(sockfd, 10) != 0)
		exit_error("Fatal Error\n", NULL);
	handle_server(sockfd);
	return 0;
}

