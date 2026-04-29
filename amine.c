#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>

int extract_message(char **buf, char **msg)
{

    char *newbuf;
    int i;

    *msg = 0;
    if (*buf == 0)
        return (0);
    i = 0;
    while ((*buf)[i])
    {
        if ((*buf)[i] == '\n')
        {
            newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
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

char *str_join(char *buf, char *add)
{
    char *newbuf;
    int len;

    if (buf == 0)
        len = 0;
    else
        len = strlen(buf);

    newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
    if (newbuf == 0)
        return (0);
    newbuf[0] = 0;
    if (buf != 0)
        strcat(newbuf, buf);
    free(buf);
    strcat(newbuf, add);
    return (newbuf);
} 


//my code here
int max = 0;
int count = 0;

int ids[65536];
char *msgs[65536];

char buff[500000];
fd_set rfds, wfds, fds;


//dialk
void fatal()
{
    write(2, "Fatal error\n", 12);
    exit(1);
}

//dialk
void ft_broadcast(int fd, char *msg)
{
    for (int i = 0; i <= max; i++)
        if (FD_ISSET(i, &wfds) && i != fd)
            send(i, msg, strlen(msg), 0);
}

//dialk
void accpetnew(int fd)
{
    int client  = accept(fd, 0, 0);
    if (client >= 0)
    {
        FD_SET(client, &fds);
        ids[client] = count++;
        msgs[client] = NULL;    
        if (client > max)
            max = client;
        sprintf(buff, "server: client %d just arrived\n", ids[client]);
        ft_broadcast(client, buff);
    }
}

//dialk
void handleold(int fd)
{
    ssize_t recv_b;

    recv_b = recv(fd, buff, 4090, 0);
    if (recv_b <= 0)
    {
        sprintf(buff, "server: client %d just left\n", ids[fd]);
        ft_broadcast(fd, buff);
        FD_CLR(fd, &fds);
        close(fd);
        if (msgs[fd])
            free(msgs[fd]);
    }   
    else
    {
        buff[recv_b] = '\0';
        msgs[fd] = str_join(msgs[fd], buff);
        char *tmp = NULL;
        while (extract_message(&msgs[fd], &tmp))
        {
            sprintf(buff, "client %d: %s", ids[fd], tmp);
            ft_broadcast(fd, buff);
            free(tmp);
        }
    }
}

int main(int ac, char **av)
{
    if (ac != 2)
    {
        write(2, "Wrong number of arguments\n", 26);
        exit(1);
    }
    for (int i = 0; i < 65536; i++)
        msgs[i] = NULL;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        fatal();
    
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr)); 
    addr.sin_family = AF_INET; 
    addr.sin_addr.s_addr = htonl(htonl(2130706433)); // 127.0.0.1
    addr.sin_port = htons(atoi(av[1])); // atoi(av[1]) dialk

    if ((bind(sock, (const struct sockaddr *)&addr, sizeof(addr))) != 0) 
    {
        //dialk
        fatal();
    }
    if (listen(sock, 128) != 0) //128 dialk 10
    {
        //dialk
        fatal();
    }



    //dialk
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    max = sock;
    while (1)
    {
        rfds = wfds = fds;
        if (select(max + 1, &rfds, &wfds, NULL, NULL) < 0)
            continue;
        for (int fd = 0; fd <= max; fd++)
        {
            if (FD_ISSET(fd, &rfds))
            {
                if (fd == sock)
                    accpetnew(sock);
                else
                    handleold(fd);
            }
        }
    }
}
