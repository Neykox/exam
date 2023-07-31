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
	int fd;
	int id;
	int n;
	struct s_client *next;
}				t_client;

void fatal()
{
	write(1, "Fatal error\n", 12);
	exit(1);
}

int get_max(int fd, t_client *client)
{
	while(client)
	{
		if (client->fd > fd)
			fd = client->fd;
		client = client->next;
	}
	return fd + 1;
}

void	send_all(t_client *client, char *msg, int fd)
{
	int l = strlen(msg);

	while (client)
	{
		if (client->fd != fd)
			send(client->fd, msg, l, 0);
		client = client->next;
	}
}

t_client *new_client(int fd, int id)
{
	t_client *new = malloc(sizeof(t_client));

	if (new == NULL)
		fatal();
	new->fd = fd;
	new->id = id;
	new->n = 1;
	new->next = NULL;
	return new;
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
	int ids = 0;
	t_client *client = NULL;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	while (1)
	{
		FD_ZERO(&read);
		FD_SET(sockfd, &read);
		t_client *set = client;
		while (set)
		{
			FD_SET(set->fd, &read);
			set = set->next;
		}
		int ret = select(get_max(sockfd, client), &read, NULL, NULL, &tv);
		if (ret > 0)
		{
			if (FD_ISSET(sockfd, &read))
			{
				int new_fd = accept(sockfd, NULL, NULL);
				if (new_fd < 0)
					fatal();
				t_client *new = new_client(new_fd, ids);
				if (client == NULL)
					client = new;
				else
				{
					t_client *add = client;
					while (add && add->next)
						add = add->next;
					add->next = new;
				}
				sprintf(msg, "server: client %d just arrived\n", ids);
				send_all(client, msg, new_fd);
				ids++;
				ret--;
			}
			t_client *tmp = client;
			while (ret > 0 && tmp)
			{
				if (FD_ISSET(tmp->fd, &read))
				{
					char buff[1048];
					bzero(buff, 1048);
					int bytes = recv(tmp->fd, buff, 200, 0);
					if (bytes < 0)
						fatal();
					if (bytes == 0)
					{
						sprintf(msg, "server: client %d just left\n", tmp->fd);
						send_all(client, msg, tmp->fd);
						t_client *prev = client;

						if (tmp == client)
						{
							client = client->next;
							close(tmp->fd);
							free(tmp);
						}
						else
						{
							while (prev && prev->next != tmp)
								prev = prev->next;
							prev->next = tmp->next;
							close(tmp->fd);
							free(tmp);
						}
					}
					else
					{
						sprintf(msg, "client %d: ", tmp->fd);
						char *token = ft_strtok(buff, '\n');
						for (; token != NULL; token = ft_strtok(NULL, '\n'))
						{
							if (tmp->n)
								send_all(client, msg, tmp->fd);
							send_all(client, token, tmp->fd);
							tmp->n = (token[strlen(token) - 1] == '\n');
						}
					}
					ret--;
				}
				tmp = tmp->next;
			}
		}
	}

	return 0;
}

















