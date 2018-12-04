/*
 * ssr_internal.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.4 $
 * $Date: 1993/02/02 00:21:06 $
 */

/* global data for SSR; need data structure for assigned/free port nums */

int num_registered_services = 0;

/* the protocol state */
struct ssr_state {
  mach_port_t wk_port;
  mach_port_t wk_port_send_right;
  XObj session;
  mach_port_t *service_map;
  Map	       service_map_hash;
};

/*
 * The map might someday be keyed on the service_id and reply_port
 *
 */
typedef struct {
  mach_port_type_t     reply_port;
  int           service_id;
  Sessn		sessn;
} SSR_ActiveId;

#define SS_MAP_SIZE  100


