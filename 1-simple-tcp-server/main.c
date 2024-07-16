#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * Set the socket options.
 * [See setsockopt manual](https://man.freebsd.org/cgi/man.cgi?setsockopt(2))
 * @param fd File descriptor
 * @return void
 */
void set_socket_options(int fd)
{

    // Array of int of size 4
    int opt[2] = {
        SO_DEBUG,
        SO_REUSEADDR,
    };
    int value = 1;
    int opt_len = sizeof(opt) / sizeof(int);
    for (int i = 0; i < opt_len; i++)
    {
        int option = opt[i];
        int set_option_result = setsockopt(fd, SOL_SOCKET, option, &value, sizeof(int));
        if (set_option_result == -1)
        {
            perror("Could not set socket option");
            exit(1);
        }
    }
}

/**
 * Handle TCP connection.
 * read() syscall to read incoming data.
 * write() syscall to write data back.
 * @param connfd Connection file descriptor
 * @return void
 */
static void handle_connection(int connfd)
{
    /**
     * @return Number of bytes read or -1 if an error occurred.
     * @note The read() function shall attempt to read nbyte bytes from the file associated with the open file descriptor.
     * @param connfd Connection file descriptor
     *
     *
     * [See read manual](https://pubs.opengroup.org/onlinepubs/009604599/functions/read.html)
     */
    char buffer[1024];
    printf("Buffer %s\n", buffer);
    int b_read = read(connfd, buffer, sizeof(buffer));
    if (b_read < 0)
    {
        perror("Could not read from connection");
        exit(1);
    }
    if (b_read == 0)
    {
        printf("Connection closed\n");
        return;
    }
    printf("Received: %s\n", buffer);
    // HTTP response
    // important to add \n at the end of the string to avoid HTTP parsing errors
    char hi[] = "HTTP/1.1 200 OK\n";
    /**
     * @return Number of bytes written or -1 if an error occurred.
     * @note The write() function shall attempt to write nbyte bytes from the buffer pointed to by buf to the file associated with the open file descriptor, fildes.
     * @param connfd Connection file descriptor
     * @param buffer Buffer to write
     * @param sizeof(buffer) Size of the buffer
     *
     */
    int b_written = write(connfd, hi, sizeof(hi));
    if (b_written < 0)
    {
        perror("Could not write to connection");
        exit(1);
    }
    printf("Sent: %s\n", hi);
}

// I'm gonna comment everything, the entire code will be painful to read :D
int main(int argc, char const *argv[])
{
    /**
     * Create a socket.
     * Does a syscall and then get the socket file descriptor
     *
     * @return File descriptor
     */
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("Could not create socket");
        exit(1);
    }

    set_socket_options(fd);

    /**
     * Bind the socket to an address.
     *
     * Create a sockaddr_in struct and set the family, port and address.
     *
     * [See docs](https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html)
     */
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0); // wildcard address 0.0.0.0
    // Casting necessary because sockaddr_in is a struct of sockaddr
    // Casting example: https://www.geeksforgeeks.org/c-typecasting/
    int bind_op_status = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (bind_op_status == -1)
    {
        perror("Could not bind socket");
        exit(1);
    }
    /**
     * Listen for incoming connections.
     * @note SOMAXCONN is a constant that represents the maximum number of connections that can be queued.
     */
    int listen_op_status = listen(fd, SOMAXCONN);
    if (listen_op_status == -1)
    {
        perror("Could not listen on socket");
        exit(1);
    }

    /**
     * Accept incoming connections.
     * @note This is a blocking call.
     */
    while (1)
    {
        struct sockaddr_in client_addr = {};
        socklen_t client_addr_len = sizeof(client_addr);
        int connection_fd = accept(fd, (struct sockaddr *)&client_addr, &client_addr_len);
        printf("Connection accepted\n");
        /**
         * [See recommended Beej's guide to network programming](https://beej.us/guide/bgnet/html/split/ip-addresses-structs-and-data-munging.html)
         * [Example](https://stackoverflow.com/a/5328184)
         */
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
        if (connection_fd == -1)
        {
            perror("Could not accept connection");
            exit(1);
        }
        printf("Connection accepted\n");
        handle_connection(connection_fd);
        close(connection_fd);
    }

    return 0;
}
