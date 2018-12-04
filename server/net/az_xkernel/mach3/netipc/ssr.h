/*
 *
 *  ssr.h
 *
 *  Types shared between SSR protocol and server/client tasks
 *
 */

#ifndef SSRMODULE
extern mach_port_t server_registry_port;
#endif

#define SERVICE_MAX	 25
#define SERVICE_MIN	  1
#define MAX_SSR_DATA	100
#define SS_NAME_SIZE    100

enum ssr_operation_type { REGISTER, UNREGISTER, SHUTDOWN, REQUEST, REPLY, ABSORB, REQUEST_SEND_PORT, REQUEST_SIMPLE_REPLY };

enum ssr_service_type { POSTCARD=SERVICE_MIN, SS_TELNET, SS_FTP, SS_HURRAY,
			SS_COMPLEX, SS_MOVE_RECEIVE, SS_OOL };

/* size of a byte in bits */
#define MNBYTESIZE 8

struct ssrdata {
    enum ssr_service_type service_id;
    enum ssr_operation_type operation;
    int sequence_num;          /* optional; server should copy to reply */
    IPhost destination_host;
    int data_length;           /* if there is data to forward */
  };

struct local_ssr_msg {
    mach_msg_header_t mmhdr;  /* the Mach portions       */
    mach_msg_type_t   mmbody; 
    struct ssrdata    ssrd;   /* for the registry        */
    char data[MAX_SSR_DATA];  /* forwarded to the server */
  };

/* the server registry forwards replies that look like this          */
/* the server will cast them to its local type (postcard, udp, etc.) */
struct server_msg {
  mach_msg_header_t   mmhdr;
  mach_msg_type_t     mmbody;
  char                server_data[MAX_SSR_DATA];
};

