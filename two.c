#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>


	char final[1024];

typedef struct	s_client
{
	int fd, num, n;
	struct s_client *next;
}				t_client;

t_client *_new(int fd, int num)
{
	t_client *new = malloc(sizeof(t_client));
	if (new == NULL)
		exit(1);
	new->fd = fd;
	new->num = num;
	new->next = NULL;
	new->n = 1;

	return new;
}

int get_max(t_client *client, int max)
{
	while (client)
	{
		if (client->fd > max)
			max = client->fd;
		client = client->next;
	}
	return max + 1;
}

void send_all(t_client *client, char *msg, int lenght, int fd)
{
	while (client)
	{
		if (client->fd != fd)
			send(client->fd, msg, lenght, 0);
		client = client->next;
	}
}

char *ft_strtok(char *s, int c)
{
	static char *next;
	int i = 0;

	bzero(final, 1024);
	if (s != NULL)
		next = s;
	if (*next == '\0' || next == NULL)
		return NULL;
	while (*next != '\0' && *next != c)
	{
		final[i++] = *next;
		next++;
	}
	if (*next != '\0')
	{
		final[i++] = *next;
		*next++ = '\0';
	}
	return final;
}

int main(int argc, char **argv) {

	if (argc != 2){ 
		printf("Wrong number of args\n"); 
		exit(1);
	}

	int sockfd;
	struct sockaddr_in servaddr; 

	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		exit(1);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(atoi(argv[1])); 
  
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		exit(1); 

	if (listen(sockfd, 10) != 0) 
		exit(1);




	fd_set read;
	char msg[100];
	char buff[1048];
	int ids = 0;
	t_client *client = NULL;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	while (1)
	{
		FD_ZERO(&read);
		FD_SET(sockfd, &read);
		t_client *read_set = client;
		while (read_set)
		{
			FD_SET(read_set->fd, &read);
			read_set = read_set->next;
		}
		int ret = select(get_max(client, sockfd), &read, NULL, NULL, &tv);
		if (ret < 0)
			exit(1);

		if (ret > 0)
		{
			if (FD_ISSET(sockfd, &read))
			{
				int new_fd = accept(sockfd, NULL, NULL);
				if (new_fd == -1)
					exit(1);
				t_client *new = _new(new_fd, ids);
				if (client == NULL)
					client = new;
				else
				{
					t_client *start = client;
					while (start && start->next)
					start = start->next;
					start->next = new;
				}
				sprintf(msg, "server: client %d just arrived\n", ids);
				send_all(client, msg, strlen(msg), new_fd);
				ids++;
				ret--;
			}
			if (ret > 0)
			{
				t_client *tmp = client;
				while (tmp && ret > 0)
				{
					int skip = 0;
					if (FD_ISSET(tmp->fd, &read))
					{
						bzero(buff, 1048);
						bzero(msg, 100);
						int bytes = recv(tmp->fd, buff, 200, 0);
						if (bytes < 0)
							exit(1);
						if (bytes == 0)
						{
							sprintf(msg, "server: client %d just left\n", tmp->num);
							send_all(client, msg, strlen(msg), tmp->fd);

							t_client *prev = client;
							if (prev == tmp)
							{
								client = client->next;
								prev = client;
							}
							else
							{
								while (prev->next != tmp)
									prev = prev->next;
								prev->next = tmp->next;
							}
							close(tmp->fd);
							free(tmp);

							tmp = prev;
							skip = 1;
						}
						else
						{
							sprintf(msg, "client %d: ", tmp->num);
							char *token = ft_strtok(buff, '\n');
							for (; token != NULL; token = ft_strtok(NULL, '\n'))
							{
								if (tmp->n)
									send_all(client, msg, strlen(msg), tmp->fd);
								send_all(client, token, strlen(token), tmp->fd);
								tmp->n = (token[(strlen(token) - 1)] == '\n');
							}
							
						}
						ret--;
					}
					if (skip == 0)
						tmp = tmp->next;
				}
			}
		}
	}
}
