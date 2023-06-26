#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct	s_client
{
	int fd, num;
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

void send_all(t_client *client, char *msg, int fd)
{
	while (client)
	{
		if (client->fd != fd)
			send(client->fd, msg, strlen(msg), 0);
		// write(client->fd, msg, strlen(msg));
		client = client->next;
	}
}

int main(int argc, char **argv) {
	argc++;
	int sockfd;
	struct sockaddr_in servaddr; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr)); 
	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		printf("socket bind failed...\n"); 
		exit(0); 
	}

	if (listen(sockfd, 10) != 0) {
		printf("cannot listen\n"); 
		exit(0); 
	}

	fd_set read;
	char msg[200];
	char final[400];
	int client_num = 0;
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
		if (select(get_max(client, sockfd), &read, NULL, NULL, &tv) > 0)
		{
			if (FD_ISSET(sockfd, &read))
			{
				int new_fd = accept(sockfd, NULL, NULL);
				if (new_fd == -1)
					exit(1);
				t_client *new = _new(new_fd, client_num);
				if (client == NULL)
					client = new;
				else
				{
					t_client *start = client;
					while (start && start->next)
					start = start->next;
					start->next = new;
				}
				sprintf(&msg[0], "server: client %d just arrived\n", client_num);
				send_all(client, &msg[0], new_fd);
				client_num++;
			}
			else
			{
				t_client *tmp = client;
				while (tmp)
				{
					int skip = 0;
					if (FD_ISSET(tmp->fd, &read))
					{
						int bytes = recv(tmp->fd, &msg[0], 199, 0);
						if (bytes < 0)
							exit(0);
						if (bytes == 0)
						{
							sprintf(&msg[0], "server: client %d just left\n", tmp->num);
							send_all(client, &msg[0], tmp->fd);

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
							msg[bytes] = '\0';
							int f = sprintf(&final[0], "client %d: ", tmp->num);
							int i = 0;
							while (msg[i])
							{
								final[f] = msg[i];
								if (msg[i] == '\n' && msg[i + 1] != '\0')
								{
									final[f + 1] = '\0';
									send_all(client, &final[0], tmp->fd);
									f = sprintf(&final[0], "client %d: ", tmp->num);
								}
								else
									f++;
								i++;
							}
							final[f] = '\0';
							send_all(client, &final[0], tmp->fd);
						}
					}
					if (skip == 0)
						tmp = tmp->next;
				}
			}
		}
	}
}
