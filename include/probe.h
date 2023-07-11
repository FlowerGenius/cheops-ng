
#ifndef PROBE_H
#define PROBE_H

typedef void (*probe_callback)(unsigned int ip, unsigned short port, char *version, void *user_data);

/* the fd has not been closed yet, when this is called, so you can poke at it
 * and figure out why there was no connection to the service (mabe the server is busy)
 */
typedef void (*probe_timeout_callback)(int fd, unsigned int ip, unsigned short port, void *user_data);

struct probe;

struct probe_data {
	probe_callback cb;
	probe_timeout_callback timeout_cb;
	void *user_data;
	int socket;
	unsigned int ip;
	int *siid;
	int *soid;
	int tid;
	int flags;
	struct probe *p;
};

#define PROBE_DATA_FLAGS_SEND 0x00000001

/* the socket will close after this callback, so if you dont get enough information 
 * from the read sorrn :( just email me and i will fix it :)
 */
typedef char *(*probe_recieve_cb)(int socket, void *recieve_arg, void *user_data);

/* this is a function that will let you know when the socket connection is open
 * you can also send information on the fd (as in http "GET / HTTP/1.0\n\n")
 */
typedef void (*probe_send_cb)(int socket, void *send_arg, void *user_data);

/* you should not need this for your plugin efforts :)
 */
struct probe {
	unsigned short    port;        // the port for the service
	probe_send_cb     send;        // the function to call when the socket is open to send
	void             *send_arg;    // an argument that is passed into the function
	probe_recieve_cb  recieve;     // the function that is called when we recieved data
	void             *recieve_arg; // yet another argument with a lame nonusable comment
};

struct probe *get_probe(unsigned short port);

void remove_probe(struct probe *p);

void register_probe(unsigned short port, 
                    probe_send_cb send, 
                    void *send_arg, 
                    probe_recieve_cb recieve, 
                    void *recieve_arg);

void init_probes(void);

char *after_text(int fd, void *arg, void *user_data);
char *get_text(int fd, void *arg, void *user_data);
char *strip_version(int fd, void *arg, void *user_data);
char *strip_newline(int fd, void *arg, void *user_data);

void send_text(int fd, void *arg, void *user_data);

void get_version(unsigned int     addr, 
                 unsigned short   port, 
                 unsigned int     timeout_ms, 
                 probe_callback   cb,
                 probe_timeout_callback timeout_cb, 
                 void            *user_data);

int make_nonblock_stream_socket(int addr, unsigned short port);

#endif /* PROBE_H */
