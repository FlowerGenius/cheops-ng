#ifndef GUI_MONITORING_H
#define GUI_MONITORING_H

#include "cheops-monitoring.h"
#include "cheops-gui.h"

void gui_monitoring_make_dialog(GtkWidget *w, struct _page_object *po);

/* 
 * you must setup po->monitoring_vbox before calling this
 */
void gui_monitoring_toplevel(struct _page_object *po);

#endif /* GUI_MONITORING_H */
