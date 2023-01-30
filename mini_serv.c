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

void	send_message(char *msg, t_client *client_lst, t_client *sender)
{
	while (client_lst)
	{
		if (client_lst != sender && FD_ISSET(client_lst->fd, &fd_wr))
			send(client_lst->fd, msg, strlen(msg), 0);
		client_lst = client_lst->next;
	}
}

t_client	*add_newclient(t_client *client_lst, int fd, int id)
{
	t_client	*newcli, *last;
	char		str[1000];
	
	newcli->id = id;
	newcli->fd = fd;
	newcli->buffer = NULL;
	newcli->next = NULL;
	FD_SET(fd, &fd_all);
	sprintf(str, "server: client %d just arrived\n", id);
	send_message(str, client_lst, newcli);
	if (!client_lst)
		return (newcli);
	last = client_lst;
	while (last->next)
		last = last->next;
	last->next = newcli;
	return (client_lst);
}

char	*str_join(char *s1, char *s2)
{
	char *join;
	int	i;
	int	j;

	i = 0;
	j = 0;
	join = malloc(sizeof(char) * strlen(s1) + strlen(s2) + 1);
	if (!join)
		return NULL;
	while (s1[i])
		join[j++] = s1[i++];
	i = 0;
	while(s2[i])
		join[j++] = s2[i++];
	join[j] = 0;
	free(s1);
	return (join);
}

ssize_t	receive_message(t_client *client_lst, t_client *curr_cli)
{
	ssize_t	size, ret;
	char	buffer[1001];

	size = 0;
	while ((ret = recv(curr_cli->fd, buffer, 10000, 0)) > 0)
	{
		size += ret;
		buffer[ret] = 0;
		curr_cli->buffer = str_join(curr_cli->buffer, buffer);
		if (!curr_cli->buffer)
			exit_error("Fattal Error\n", client_lst);
	}
	if (size == 0 && ret == -1)
		return (-1);
	return (size);
}

t_client	*remove_client(t_client *client_lst, t_client *rmcli)
{
	char	str[1000];
	t_client	*prev;

	if (client_lst == rmcli)
		client_lst = rmcli->next;
	else
	{
		prev = client_lst;
		while (prev->next != rmcli)
			prev = prev->next;
		prev->next = rmcli->next;
	}
	sprintf(str, "server: client %d, just left\n", rmcli->id);
	send_message(str, client_lst, rmcli);
	FD_CLR(rmcli->fd, &fd_all);
	close(rmcli->fd);
	free(rmcli->buffer);
	free(rmcli);
	return (client_lst);
}

int	extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof (*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

t_client	*handle_client(t_client *client_lst)
{
	char	*msg, *str;
	ssize_t	ret;
	t_client	*it, *curr_cli;

	it = client_lst;
	while (it)
	{
		curr_cli = it;
		it = curr_cli->next;
		if(FD_ISSET(curr_cli->fd, &fd_rd))
		{
			ret = receive_message(client_lst, curr_cli);
			if (ret == 0)
				client_lst = remove_client(client_lst, curr_cli);
			else if (ret > 0)
			{
				while ((ret = extract_message(&curr_cli->buffer, &msg)) == 1)
				{
					if (!(str = malloc(sizeof(char) * (20 + strlen(msg)))))
						exit_error("Fatal Error\n", client_lst);
					sprintf(str, "client %d: %s", curr_cli->id, msg);
					send_message(str, client_lst, curr_cli);
					free(msg);
					free(str);
				}
				if (ret == -1)
					exit_error("Fatal Error\n", client_lst);
			}
		}
	}
	return (client_lst);
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
				client_lst = add_newclient(client_lst, connfd, client_fd);
				maxfd = connfd > maxfd ? connfd : maxfd;
				++client_fd;
			}
		}
		else
			client_lst = handle_client(client_lst);
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

