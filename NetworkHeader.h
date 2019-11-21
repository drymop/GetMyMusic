#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <netdb.h>

#define SERVER_HOST "mathcs01"  /* default host */
#define SERVER_PORT "30450"

#define SA struct sockaddr

#define	BUFFSIZE	8192	/* buffer size for reads and writes */

static const int VERSION = 0x1;