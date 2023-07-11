#ifndef CHEOPS_MONITORING_H
#define CHEOPS_MONITORING_H

#include <glib.h>
#include "config.h"
#include "cheops-config.h"

enum {
	TOPLEVEL_MONITOR_OS = 0,
	TOPLEVEL_MONITOR_PORT,
	TOPLEVEL_MONITOR_STATE,
};

enum {
	MONITOR_OS_CHANGE = 0,
};

enum {
	MONITOR_PORT_OPEN = 0,
	MONITOR_PORT_VERSION_DOESNT_CHANGE,
	MONITOR_PORT_NO_NEW_PORTS_ADDED,
};

enum {
	MONITOR_STATE_PING_HOST = 0,
	MONITOR_STATE_CHECK_PORT,
};

enum {
	MONITOR_STATE_OK = 0,
	MONITOR_STATE_WARNING,
	MONITOR_STATE_CRITICAL,
	MONITOR_STATE_ERROR,
};

typedef struct _monitoring_session {
	char *id;                 // this is a dotted decimal path to the monitor 
	                          // like "1.2" for TOPLEVEL_MONITOR_PORT, MONITOR_PORT_NO_NEW_PORTS_ADDED
	unsigned int  host;       // the host we are monitoring
	char         *user;       // the user that made the monitoring
	char         *action;     // the script that the user wants us to run when the state is bad
	unsigned int  state;      // the current state of the monitored host MONITOR_STATE_*
	char         *error;      // a error string (malloced)
	unsigned int  seconds;    // seconds inbetween checks
	void         *arg;        // argument used by the monitoring code (in the gui we use this for the page_object)
} monitoring_session;

struct _monitoring_node;
/**
 * Callback when a specific monitoring node is selected/activated
 * @param session the monitoring session associated with this callback
 */
typedef void (*monitoring_callback)(monitoring_session *session, struct _monitoring_node *node);

typedef struct _monitoring_node {
	char *name;
	unsigned int id;
	monitoring_callback callback;
	char *path;                       // this is a dotted decimal path to this monitoring_node
	GList *children;
} monitoring_node;



monitoring_node *monitoring_node_find_toplevel(unsigned int id);
monitoring_node *monitoring_node_find(monitoring_node *parent, unsigned int child_id);
monitoring_node *monitoring_node_new(char *name, unsigned int id, monitoring_callback cb);

/*
 * 0 good, 1 bad
 */
int monitoring_node_add_toplevel(monitoring_node *m);

/*
 * 0 good, 1 bad
 */
int monitoring_node_add(monitoring_node *parent, monitoring_node *child);

unsigned int monitoring_session_run(monitoring_session *session);

monitoring_session *monitoring_session_new(char *id);

void monitoring_session_destroy(monitoring_session *mon);

void monitoring_list_foreach_toplevel(void (*func)(monitoring_node *node, void *arg), void *arg);

void monitoring_list_foreach(monitoring_node *node, void (*func)(monitoring_node *node, void *arg), void *arg);

monitoring_node *monitoring_node_find_by_name_toplevel(char *child_name);

monitoring_node *monitoring_node_find_by_name(monitoring_node *parent, char *child_name);

#endif /* CHEOPS_MONITORING_H */
