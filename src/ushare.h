/*
 * ushare.h : GeeXboX uShare UPnP Media Server header.
 * Originally developped for the GeeXboX project.
 * Parts of the code are originated from GMediaServer from Oskar Liljeblad.
 * Copyright (C) 2005-2007 Benjamin Zores <ben@geexbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _USHARE_H_
#define _USHARE_H_

#include <upnp/upnp.h>
#include <upnp/upnptools.h>
#include <stdbool.h>
#include <pthread.h>
#include <strings.h>
#include <stdio.h>

#ifdef HAVE_DLNA
#include <dlna.h>
#endif /* HAVE_DLNA */


//for json parsing
#include <jansson.h>

//for rest
#include <ulfius.h>

//for list
#include <glib.h>

#include "content.h"
#include "buffer.h"
#include "redblack.h"

#define VIRTUAL_DIR "/web"
#define XBOX_MODEL_NAME "Windows Media Connect Compatible"
#define DEFAULT_UUID "198f9738-d930-8db8-a3cf"

#define UPNP_MAX_CONTENT_LENGTH 4096

#define STARTING_ENTRY_ID_DEFAULT 0
#define STARTING_ENTRY_ID_XBOX360 100000

#define UPNP_DESCRIPTION \
"<?xml version=\"1.0\" encoding=\"utf-8\"?>" \
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\" >" \
"  <specVersion>" \
"    <major>1</major>" \
"    <minor>0</minor>" \
"  </specVersion>" \
"  <device>" \
"    <deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>" \
"    <friendlyName>%s: 1</friendlyName>" \
"    <manufacturer>GeeXboX Team</manufacturer>" \
"    <manufacturerURL>http://ushare.geexbox.org/</manufacturerURL>" \
"    <modelDescription>GeeXboX uShare : UPnP Media Server</modelDescription>" \
"    <modelName>%s</modelName>" \
"    <modelNumber>001</modelNumber>" \
"    <modelURL>http://ushare.geexbox.org/</modelURL>" \
"    <serialNumber>GEEXBOX-USHARE-01</serialNumber>" \
"    <UDN>uuid:%s</UDN>" \
"    <serviceList>" \
"      <service>" \
"        <serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType>" \
"        <serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId>" \
"        <SCPDURL>/web/cms.xml</SCPDURL>" \
"        <controlURL>/web/cms_control</controlURL>" \
"        <eventSubURL>/web/cms_event</eventSubURL>" \
"      </service>" \
"      <service>" \
"        <serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType>" \
"        <serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId>" \
"        <SCPDURL>/web/cds.xml</SCPDURL>" \
"        <controlURL>/web/cds_control</controlURL>" \
"        <eventSubURL>/web/cds_event</eventSubURL>" \
"      </service>" \
"      <service>" \
"        <serviceType>urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1</serviceType>\n" \
"        <serviceId>urn:microsoft.com:serviceId:X_MS_MediaReceiverRegistrar</serviceId>\n" \
"        <SCPDURL>/web/msr.xml</SCPDURL>" \
"        <controlURL>/web/msr_control</controlURL>" \
"        <eventSubURL>/web/msr_event</eventSubURL>" \
"      </service>\n" \
"    </serviceList>" \
"    <presentationURL>/web/ushare.html</presentationURL>" \
"  </device>" \
"</root>"

struct ushare_t {
  char *name;
  char *interface;
  char *model_name;
  content_list *contentlist;
  struct rbtree *rb;
  struct upnp_entry_t *root_entry;
  int nr_entries;
  int starting_id;
  int init;
  UpnpDevice_Handle dev;
  char *udn;
  char *ip;
  unsigned short port;
  unsigned short telnet_port;
  struct buffer_t *presentation;
  bool use_presentation;
  bool use_telnet;
#ifdef HAVE_DLNA
  bool dlna_enabled;
  dlna_t *dlna;
  dlna_org_flags_t dlna_flags;
#endif /* HAVE_DLNA */
  bool xbox360;
  bool verbose;
  bool daemon;
  bool override_iconv_err;
  char *cfg_file;
  pthread_mutex_t termination_mutex;
  pthread_cond_t termination_cond;
};

struct action_event_t {
  struct Upnp_Action_Request *request;
  bool status;
  struct service_t *service;
};

inline void display_headers (void);




// INPUT Version Data Structures


 typedef struct {
    char* data;
    size_t dimension;
    off_t length;
} video_buffer;

typedef struct {
    char *source;
    char* buffer;
    pthread_mutex_t lock;
    size_t dimension;
    off_t i;
    off_t j;
    enum status {
        ON,
        OFF
    } status;
    int fd;
    int f_out;
    int f_in;
    int upnp_id;
    
} transcode_args;

typedef struct live_transcoding{
    int fd;
    int id;
    FILE *fp;
    time_t last_read;
    int startup;
    transcode_args live;
    char* inet_address_client;
    
} live_transcoding_t;

typedef struct {
    int id;
    char *src;
    int c_id;
    
} live_objects_t;



GSList *live_streams_list;

GSList *stream_map;



size_t live_number;
int cmpfunc(const void *a, const void *b);
int cmpfunc_streams(const void*,const void*);
gint g_cmpfunc(gpointer,gpointer);
gint g_cmpfunc_ip(gpointer,gpointer);
gint g_cmpfunc_stream(gpointer, gpointer);


#define PA_PORT 50050
#define PA_HTTP_PORT 8090
char *personal_acquirer_address;


char* get_pa_media_uri(char*);
void clear_obj(int,int);
void add_source(char*);
void add_source_pa(char*);
void add_from_pa(char*);

void* register_to_pa(void*);
void* connect_to_pa(void*);
void* t_add_from_pa(void*);
void* t_monitoring_pipe(void*);
#endif /* _USHARE_H_ */


