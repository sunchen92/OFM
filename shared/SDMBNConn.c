#include "SDMBNConn.h"
#include "SDMBNDebug.h"
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t sdmbn_lock_conn;

/**
  * Get a socket address for a connection.
  * @param hostname Hostname or IP address for the connection
  * @param port Port for the connection
  * @param addr Location to store the address
  * @return The socket address structure supplied as argument; NULL on error
  */ 
static struct sockaddr *conn_get_sockaddr(const char* hostname, 
        unsigned short port, struct sockaddr* addr)
{
    struct addrinfo hints;
    struct addrinfo* results;

    /* Setup hints structure for getaddrinfo */
    bzero(&hints,sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;

    /* Attempt to get address info */
    getaddrinfo(hostname, NULL, &hints, &results);
    if (results == NULL) 
    {
        return NULL;
    }
    
    /* Copy first matching address into addr structure */
    memcpy(addr, results->ai_addr, sizeof(struct sockaddr));
    ((struct sockaddr_in *)addr)->sin_port = htons(port);

    /* Free address info */
    freeaddrinfo(results);
    return addr;
}

/**
  * Open a socket.
  * @return Connected socket file descriptor; -1 on error
  */
int conn_active_open(const char *host, unsigned short port)
{
    int sock = -1;
    struct sockaddr dst;

	DEBUG_PRINT("Opening socket");
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        ERROR_PRINT("Could not get socket: %s", strerror(errno));
        return -1;
    }

    /* added so that state channel does not buffer to be full; disable
     * Nagle algorithm
     */
    int i = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void*)&i, sizeof(i));

    if (NULL == conn_get_sockaddr(host, port, &dst))
    { 
        ERROR_PRINT("Could not get sockaddr"); 
        return -1; 
    }
    if (connect(sock, &dst, sizeof(struct sockaddr)) < 0)
    {
        ERROR_PRINT("Could not connect to %s:%d:  %s", host, port,
			strerror(errno)); 
        return -1; 
    }

    /* added so that state channel does not buffer to be full; disable
     * Nagle algorithm
     */
    /*int i = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void*)&i, sizeof(i));
	*/
	DEBUG_PRINT("Opened socket");
    return sock;
}

/**
  * Listen for a socket connection and accept it. 
  * @return Connected socket file descriptor; -1 on error
  */
int conn_passive_open(unsigned short port)
{
    int server = -1, client = -1;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr);
    server = socket(AF_INET, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server, (struct sockaddr *)&addr, addrlen) < 0)
    {
        ERROR_PRINT("Could not bind: %s", strerror(errno)); 
        return -1; 
    }
    DEBUG_PRINT("Listening for connection");
    listen(server, 0);
    client = accept(server, (struct sockaddr *)&addr, &addrlen);
    close(server);
    return client;
}

int conn_close(int conn)
{
    if (close(conn) < 0)
    { return -1; }
    return 0;
}

char *conn_read(int conn)
{
    int readlen = 0, result = 0;

    // Attempt to read length of string
    char lenchars[sizeof(int)];
    unsigned int count = 0;
    while (count < sizeof(int))
    {
        result = read(conn, &lenchars[count], sizeof(int) - count);
        if (result < 0)
        {
            perror("Failure in conn_read");
        	return NULL;
        }
        count += result;
    }
    int strlen = ntohl(*((int *)lenchars));
    DEBUG_PRINT("Should read %d bytes", strlen);
   
    // Limit read size 
    #define SDMBN_CONN_READ_MAX (1024 * 1024 * 1024)
    assert(strlen < SDMBN_CONN_READ_MAX);

    // Allocate buffer for string
    char *buf = (char *)malloc(strlen+1);
    assert(buf != NULL);
    char *cur = buf;

    // Read string
    while (readlen < strlen)
    {
        // Read from connection
        result = read(conn, cur, strlen - readlen);

        // Encountered error
        if (result < 0)
        { 
            ERROR_PRINT("Error reading from socket: %d", result);
            free(buf);
            return NULL;
        }

        // Data was read
        cur += result;
        readlen += result;
    }
    buf[readlen] = '\0';
    return buf;
}

int conn_write(int conn, char *buf, int len)
{
    pthread_mutex_lock(&sdmbn_lock_conn);
    int result = write(conn, (void *)buf, len);
    pthread_mutex_unlock(&sdmbn_lock_conn);
    return result;
}

int conn_write_append_newline(int conn, char *buf, int len)
{
    int tmplen = htonl(len);
    pthread_mutex_lock(&sdmbn_lock_conn);
    write(conn, &tmplen, sizeof(tmplen));
    int result = write(conn, (void *)buf, len);
    //write(conn, "\n", 1);
    pthread_mutex_unlock(&sdmbn_lock_conn);
    return result;
}
