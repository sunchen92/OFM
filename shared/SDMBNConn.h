#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SDMBNConn_H_
#define _SDMBNConn_H_

#include <pthread.h>

extern pthread_mutex_t sdmbn_lock_conn;

///// FUNCTION PROTOTYPES ////////////////////////////////////////////////////
int conn_active_open(const char *host, unsigned short port);
int conn_passive_open(unsigned short port);
int conn_close(int conn);
char *conn_read(int conn);
int conn_write(int conn, char *buf, int len);
int conn_write_append_newline(int conn, char *buf, int len);

#endif

#ifdef __cplusplus
}
#endif
