/*
 * http.c : GeeXboX uShare Web Server handler.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

#include <pthread.h>
#include <time.h>

#include <curl/curl.h>
#include <string.h>

#include "services.h"
#include "cds.h"
#include "cms.h"
#include "msr.h"
#include "metadata.h"
#include "http.h"
#include "minmax.h"
#include "trace.h"
#include "presentation.h"
#include "osdep.h"
#include "mime.h"

#define KB 1024
#define MB 1024*KB


#define PROTOCOL_TYPE_PRE_SZ  11   /* for the str length of "http-get:*:" */
#define PROTOCOL_TYPE_SUFF_SZ 2    /* for the str length of ":*" */


pthread_t t_read;

struct web_file_t {
    char *fullpath;
    off_t pos;

    enum {
        FILE_LOCAL,
        FILE_MEMORY,
        FILE_LIVE
    } type;

    union {

        struct {
            int fd;
            struct upnp_entry_t *entry;
        } local;

        transcode_args live;

        struct {
            char *contents;
            off_t len;
        } memory;
    } detail;
};

char* allocate_buffer(size_t dimension) {

    char* vb = NULL;
    vb = (char*) calloc(dimension, sizeof (char));
    if (vb == NULL)
        return NULL;
    return vb;
}

void clear_buffer(char* vb, size_t dimension) {
    bzero(vb, dimension);
}

#define CHUNK_SIZE 1024

int receive_basic(int s, size_t max_len, char *buff) {
    int size_recv, total_size = 0;
    char chunk[CHUNK_SIZE];
    struct timeval begin, now;
    double timediff;
    int timeout = 100000;


    fcntl(s, F_SETFL, O_NONBLOCK);

    //beginning time
    gettimeofday(&begin, NULL);

    //loop
    while (total_size < max_len) {
        gettimeofday(&now, NULL);

        //time elapsed in seconds
        //timediff = (now.tv_sec - begin.tv_sec) + 1e-6 * (now.tv_usec - begin.tv_usec);
        timediff = (now.tv_usec - begin.tv_usec);

        //if you got some data, then break after timeout
        /*if( total_size > 0 && timediff > timeout )
        {
            break;
        }
         
        //if you got no data at all, wait a little longer, twice the timeout
        else if( timediff > timeout*2)
        {
            break;
        }*/

        memset(chunk, 0, CHUNK_SIZE); //clear the variable
        if ((size_recv = recv(s, chunk, CHUNK_SIZE, 0)) < 0) {
            usleep(10000);
        } else {
            char *bb = "%s%s";
            //memcpy(buff+total_size, chunk,size_recv);
            total_size += size_recv;
            sprintf(buff, bb, buff, chunk);

            //printf("%s" , chunk);
        }
    }

    return total_size;
}

size_t read_from_pa(char *data, char* src, size_t lenght) {

    int sock;
    struct sockaddr_in pa;
    char* http_base = "GET %s HTTP/1.1\r\nUser-Agent: Lavf/56.40.101\r\nAccept: */*\r\nRange: bytes=0-\r\nConnection: close\r\nHost: 127.0.0.1:8090\r\nIcy-MetaData: 1\r\n\r\n";
    char* message;
    char* reply;
    size_t len = -1;

    char *response = "HTTP/1.0 200 OK\r\\nContent-type: application/octet-stream\r\nCache-Control: no-cache\r\n\r\n";

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Cannot create socket!");
        exit(EXIT_FAILURE);
    }
    //log_verbose("Socket created\n");

    pa.sin_addr.s_addr = inet_addr(personal_acquirer_address);
    pa.sin_family = AF_INET;
    pa.sin_port = htons(PA_HTTP_PORT);

    //fcntl(sock, F_SETFL,  O_NONBLOCK);


    //Connect to remote server
    if (connect(sock, (struct sockaddr *) &pa, sizeof (pa)) < 0) {
        perror("connect failed. Error");
        exit(EXIT_FAILURE);
    }

    //log_verbose("Connected\n");


    message = (char *) calloc(strlen(http_base) + strlen(src), sizeof (char));
    sprintf(message, http_base, src);

    //log_verbose("Sending\n %s",message);

    if (send(sock, message, strlen(message), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }



    reply = (char *) calloc(lenght, sizeof (char));



    len = receive_basic(sock, lenght, reply);

    if (len < 0) {
        perror("recv failed");
        exit(EXIT_FAILURE);
    }

    if (len == 0) {
        return 0;
    }
    //log_verbose("Len is %zd\n\n",len);

    char *p = strstr(reply, "\r\n\r\n");
    /*snprintf(reply,"%s",p);   
    
    printf("\n\n\n####### Server reply ############\n %s\n####################\n",reply);
     */
    close(sock);

    memcpy(data, p + 4, lenght - strlen(reply));
    free(reply);
    free(message);
    return len;



}

void *read_thread(void *arg) {

    transcode_args* par = (transcode_args*) arg;
    int buffsize = 2147483647; //1048576
    char* buff = (char *) calloc(buffsize, sizeof (char));
    int len = 0;
    int milliseconds = 40;
    struct timespec ts;



    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;

    log_verbose("Read thread started!\n");

    FILE* fp = fdopen(par->f_out, "w");
    while (1) {
        //nanosleep(&ts, NULL);
        pthread_mutex_lock(&(par->lock));
        if (/*par->status == ON*/1) {
            len = read(par->fd, buff, buffsize);
            if (lseek(par->f_in, 0, SEEK_END) == -1) {
                    log_verbose("%s: cannot seek: %s\n", par->source, strerror(errno));
            }
            write(par->f_out, buff, buffsize);

            
            if(fflush(fp)<0)
                perror("Error on fflush()");
            //fprintf(par->f_out, "%s", buff);

            if (fsync(par->f_out) < 0) {
                perror("Error on fsync()");
            }

            /*if (len > 0 && par->j + len <= par->dimension) {

                printf("Buffer is not full,writing in %ld\n", par->j);

                memcpy(&par->buffer[par->j], buff, len * sizeof ( char));
                par->j = (par->j + len) % par->dimension;
            } else {
                if (len > 0 && par->j + len > par->dimension) {


                    int num = par->dimension - par->j;
                    printf("Buffer is full,writing in %ld -  %d bytes\n", par->j, num);

                    memcpy(&par->buffer[par->j], buff, num * sizeof ( char));
                    len = len - num;
                    printf("Buffer is full,writing in 0 another %d bytes \n", len);

                    memcpy(&par->buffer[0], &buff[num], len - 1 * sizeof ( char));
                    if (len > par->i) {
                        par->i = par->i + len;
                    }
                    par->j = par->i - 1;
                }
            }*/
        }
        pthread_mutex_unlock(&(par->lock));
    }
}

static inline void set_info_file(struct File_Info *info, const size_t length,
        const char *content_type) {
    info->file_length = length;
    info->last_modified = 0;
    info->is_directory = 0;
    info->is_readable = 1;
    info->content_type = ixmlCloneDOMString(content_type);
}

static int http_get_info(const char *filename, struct File_Info *info) {
    extern struct ushare_t *ut;
    struct upnp_entry_t *entry = NULL;
    struct stat st;
    int upnp_id = 0;
    char *content_type = NULL;
    char *protocol = NULL;

    if (!filename || !info)
        return -1;

    log_verbose("http_get_info, filename : %s\n", filename);
    //printf ("http_get_info, filename : %s\n", filename);

    if (!strcmp(filename, CDS_LOCATION)) {
        set_info_file(info, CDS_DESCRIPTION_LEN, SERVICE_CONTENT_TYPE);
        return 0;
    }

    if (!strcmp(filename, CMS_LOCATION)) {
        set_info_file(info, CMS_DESCRIPTION_LEN, SERVICE_CONTENT_TYPE);
        return 0;
    }

    if (!strcmp(filename, MSR_LOCATION)) {
        set_info_file(info, MSR_DESCRIPTION_LEN, SERVICE_CONTENT_TYPE);
        return 0;
    }




    if (ut->use_presentation && !strcmp(filename, USHARE_PRESENTATION_PAGE)) {
        if (build_presentation_page(ut) < 0)
            return -1;

        set_info_file(info, ut->presentation->len, PRESENTATION_PAGE_CONTENT_TYPE);
        return 0;
    }

    if (ut->use_presentation && !strncmp(filename, USHARE_CGI, strlen(USHARE_CGI))) {
        if (process_cgi(ut, (char *) (filename + strlen(USHARE_CGI) + 1)) < 0)
            return -1;

        set_info_file(info, ut->presentation->len, PRESENTATION_PAGE_CONTENT_TYPE);
        return 0;
    }

    upnp_id = atoi(strrchr(filename, '/') + 1);
    entry = upnp_get_entry(ut, upnp_id);
    if (!entry)
        return -1;

    if (!entry->fullpath)
        return -1;

    if (!isLiveMedia(entry->fullpath)) {
        if (stat(entry->fullpath, &st) < 0)
            return -1;

        if (access(entry->fullpath, R_OK) < 0) {
            if (errno != EACCES)
                return -1;
            info->is_readable = 0;
        } else
            info->is_readable = 1;

        /* file exist and can be read */
        info->file_length = st.st_size;
        info->last_modified = st.st_mtime;
        info->is_directory = S_ISDIR(st.st_mode);

    } else {

        time_t t;
        time(&t);
        info->is_readable = 1;
        info->file_length = 2147483647; //25*MB; //TODO vedere se corrisponde a lunghezza file
        info->last_modified = t;
        info->is_directory = 0;
    }






    protocol =
#ifdef HAVE_DLNA
            entry->dlna_profile ?
            dlna_write_protocol_info(DLNA_PROTOCOL_INFO_TYPE_HTTP,
            DLNA_ORG_PLAY_SPEED_NORMAL,
            DLNA_ORG_CONVERSION_NONE,
            DLNA_ORG_OPERATION_RANGE,
            ut->dlna_flags, entry->dlna_profile) :
#endif /* HAVE_DLNA */
            mime_get_protocol(entry->mime_type);

    content_type =
            strndup((protocol + PROTOCOL_TYPE_PRE_SZ),
            strlen(protocol + PROTOCOL_TYPE_PRE_SZ)
            - PROTOCOL_TYPE_SUFF_SZ);
    free(protocol);

    if (content_type) {
        info->content_type = ixmlCloneDOMString(content_type);
        free(content_type);
    } else
        info->content_type = ixmlCloneDOMString("");

    return 0;
}

static UpnpWebFileHandle get_file_memory(const char *fullpath, const char *description,
        const size_t length) {
    struct web_file_t *file;

    file = malloc(sizeof (struct web_file_t));
    file->fullpath = strdup(fullpath);
    file->pos = 0;
    file->type = FILE_MEMORY;
    file->detail.memory.contents = strdup(description);
    file->detail.memory.len = length;

    return ((UpnpWebFileHandle) file);
}

static UpnpWebFileHandle http_open(const char *filename, enum UpnpOpenFileMode mode) {
    extern struct ushare_t *ut;
    struct upnp_entry_t *entry = NULL;
    struct web_file_t *file;
    int fd, upnp_id = 0;

    transcode_args* th_args = (transcode_args*) calloc(1, sizeof (transcode_args));

    if (!filename)
        return NULL;

    log_verbose("http_open, filename : %s\n", filename);
    if (mode != UPNP_READ)
        return NULL;

    if (!strcmp(filename, CDS_LOCATION))
        return get_file_memory(CDS_LOCATION, CDS_DESCRIPTION, CDS_DESCRIPTION_LEN);

    if (!strcmp(filename, CMS_LOCATION))
        return get_file_memory(CMS_LOCATION, CMS_DESCRIPTION, CMS_DESCRIPTION_LEN);

    if (!strcmp(filename, MSR_LOCATION))
        return get_file_memory(MSR_LOCATION, MSR_DESCRIPTION, MSR_DESCRIPTION_LEN);

    if (ut->use_presentation && (!strcmp(filename, USHARE_PRESENTATION_PAGE)
            || !strncmp(filename, USHARE_CGI, strlen(USHARE_CGI))))
        return get_file_memory(USHARE_PRESENTATION_PAGE, ut->presentation->buf,
            ut->presentation->len);

    upnp_id = atoi(strrchr(filename, '/') + 1);
    entry = upnp_get_entry(ut, upnp_id);
    if (!entry)
        return NULL;

    if (!entry->fullpath)
        return NULL;

    log_verbose("Fullpath : %s\n", entry->fullpath);



    if (isLiveMedia(entry->fullpath)) {

       /* live_transcoding_t obj; // = calloc(1,sizeof(live_transcoding_t));
        obj.id = entry->id;
        live_transcoding_t* f = (live_transcoding_t*) bsearch((void*) &obj, (void *) live_objects, live_number, sizeof (live_transcoding_t), cmpfunc);

        if (live_number > 0 && f != NULL) {
            printf("There are live streams...\n");
            printf("Stream already found!\n");
            file = malloc(sizeof (struct web_file_t));
            file->fullpath = strdup(entry->fullpath);
            file->pos = 0;
            file->type = FILE_LIVE;
            file->detail.local.entry = entry;
            file->detail.local.fd = f->fd;

            file->detail.live = f->live;

        } else { */

            FILE *fp;
            //int rfd;
            //-analyzeduration 10
            //-b 20000k

            //char *base="ffmpeg -hide_banner -threads auto -acodec mp3 -vcodec h264 -f mpegts -i %s -y -threads auto -c:v mpeg2video -pix_fmt yuv420p -qscale:v 1 -r 24000/1001 -g 15 -c:a ac3 -b:a 384k -ac 2 -map 0:1 -map 0:0 -b 100k -sn -f vob pipe:";
            //char *base="nice -n -20 ffmpeg -hide_banner -acodec mp3 -vcodec h264 -f mpegts -i %s -y -acodec copy -vcodec copy -f mpegts pipe:";

            char *base = "ffmpeg -hide_banner -acodec mp3 -vcodec h264 -f mpegts -i %s -preset ultrafast -y -acodec copy -vcodec copy -f mpegts pipe:";
            //char *base = "ffmpeg -hide_banner -acodec mp3 -vcodec h264 -f mpegts -i %s -y -acodec copy -vcodec copy -segment_time 4 -f segment test.mp4";
            
            char *cmd = calloc(strlen(base) + strlen(entry->fullpath), sizeof (char));
            printf("Source is: %s\n", entry->fullpath);
            sprintf(cmd, base, entry->fullpath);


            printf("Command is: %s\n", cmd);
            fp = popen(cmd, "r");

            if (!fp < 0) {
                return NULL;
            }
            fd = fileno(fp);
            fcntl(fd, F_SETFL, O_RDONLY, O_SYNC, O_NDELAY,O_NONBLOCK);





            file = malloc(sizeof (struct web_file_t));
            file->fullpath = strdup(entry->fullpath);
            file->pos = 0;
            file->type = FILE_LIVE;
            file->detail.local.entry = entry;
            file->detail.local.fd = fd;
            
                   


            if (pthread_mutex_init(&(th_args->lock), NULL) != 0) {
                printf("\n mutex init failed\n");
                exit(EXIT_FAILURE);
            }

/*
            char *fname=(char*)calloc(256,sizeof(char));
            strcpy(fname,"test.mp4XXXXXX");
            int temp = mkstemp(fname);//open("test.mp4", O_DSYNC | O_NDELAY | O_WRONLY | O_EXCL | O_CREAT, 0666);
            if (temp < 0) {
                perror("Error on opening temp file!");
                exit(EXIT_FAILURE);
            }
*/
            /*int temp2=0;
            while(temp2<=0)
            {
                sleep(5);
                temp2 = open("test.mp4", O_NONBLOCK | O_SYNC | O_NDELAY | O_RDONLY);
                if (temp2 < 0) {
                    perror("Error on opening temp file!");
                    //exit(EXIT_FAILURE);
                }
            }*/

            pthread_mutex_lock(&(th_args->lock));
            th_args->source = strdup(entry->fullpath);
            th_args->buffer = allocate_buffer(20 * KB);
            th_args->status = ON;
            th_args->fd = fd;
            th_args->dimension = 20 * KB; //(5 * MB);
            th_args->i = 0;
            th_args->j = 0;
            th_args->upnp_id=entry->id;
            //th_args->f_in = temp2;
            //th_args->f_out = temp2;
            pthread_mutex_unlock(&(th_args->lock));

            file->detail.live = *th_args;




            //lista dei file live accesi
            live_transcoding_t* obj=(live_transcoding_t*)calloc(1,sizeof(live_transcoding_t));
            obj->id = entry->id;
            obj->fd = fd;
            obj->live = *th_args;
            obj->fp=fp;
            obj->last_read=0;
            obj->startup=0;
            /////
            
            live_streams_list = g_slist_append(live_streams_list,(gpointer) obj );
            printf("List size is now: %o\n",g_slist_length (live_streams_list));
            printf("ID is %d\n",((live_transcoding_t *) live_streams_list->data)->id);
            /*
            
            
            live_objects = (live_transcoding_t*) realloc(live_objects, (live_number + 1) * sizeof (live_transcoding_t));
            live_objects[live_number] = obj;
            live_number++;*/

/*
            int err;
            err = pthread_create(&t_read, NULL, &read_thread, (void*) th_args);
            if (err != 0)
                printf("\ncan't create thread :[%s]", strerror(err));
*/

       // }

    } else {
        fd = open(entry->fullpath, O_RDONLY | O_NONBLOCK | O_SYNC | O_NDELAY);
        if (fd < 0)
            return NULL;

        file = malloc(sizeof (struct web_file_t));
        file->fullpath = strdup(entry->fullpath);
        file->pos = 0;
        file->type = FILE_LOCAL;
        file->detail.local.entry = entry;
        file->detail.local.fd = fd;
    }

    return ((UpnpWebFileHandle) file);
}

static int http_read(UpnpWebFileHandle fh, char *buf, size_t buflen) {
    struct web_file_t *file = (struct web_file_t *) fh;
    ssize_t len = -1;
    FILE *f;
    struct stat st;

    log_verbose("http_read\n");
    printf("Buffer size received %zd\n", buflen);

    if (!file)
        return -1;

    switch (file->type) {
        case FILE_LOCAL:
            log_verbose("Read local file.\n");
            len = read(file->detail.local.fd, buf, buflen);
            break;
        case FILE_MEMORY:
            log_verbose("Read file from memory.\n");
            len = (size_t) MIN(buflen, file->detail.memory.len - file->pos);
            memcpy(buf, file->detail.memory.contents + file->pos, (size_t) len);
            break;
        case FILE_LIVE: // CASE FOR LIVE VIDEO



            /////////////// WHITOUT BUFFER
            log_verbose("Read live file.\n");
            len = read(file->detail.live.fd, buf, buflen);
            
            live_transcoding_t obj; // = calloc(1,sizeof(live_transcoding_t));
        
            obj.id = file->detail.live.upnp_id;
            
            GSList* gs =g_slist_find_custom (live_streams_list, (gconstpointer) &obj,(GCompareFunc)g_cmpfunc);
            if(gs){
                live_transcoding_t* f=(live_transcoding_t*) gs->data;        
                f->last_read=time(NULL);
            }
            
            
            
            //////////////////



            //READ FROM SOCKET TEST
            //log_verbose("Read live file. (Socket read)\n");
            //len = read_from_pa(buf,file->detail.live.source,32000);


            /// WITH FILE

//            pthread_mutex_lock(&(file->detail.live.lock));
//            if (fstat(file->detail.live.f_in, &st) < 0)
//                return 0;
//
//            int file_len = st.st_size;
//            if(file_len==0)
//                return 0;
//            if (file_len > buflen) {
//                if (lseek(file->detail.live.f_in, buflen, SEEK_END) == -1) {
//                    log_verbose("%s: cannot seek: %s\n", file->detail.live.source, strerror(errno));
//                    return 0;
//                }
//
//            } else {
//                if (lseek(file->detail.live.f_in, file_len, SEEK_END) == -1) {
//                    log_verbose("%s: cannot seek: %s\n", file->detail.live.source, strerror(errno));
//                    return 0;
//                }
//
//            }
//            len = read(file->detail.live.f_in, buf, buflen);
//            pthread_mutex_unlock(&(file->detail.live.lock));




            ///


            //////////// WITH BUFFER //////////////////

            /*log_verbose("Read live file.\n");
            
            transcode_args f = file->detail.live;
            pthread_mutex_lock(&(f.lock));
            if (f.status == ON) {
                len = MIN(buflen,f.dimension-f.i); 
                if(f.i!=f.j)
                    memcpy(buf, f.buffer[f.i], len);
            }
            pthread_mutex_unlock(&(f.lock));*/
            ////////////////////////////////

            break;
        default:
            log_verbose("Unknown file type.\n");
            break;
    }

    if (len >= 0)
        file->pos += len;

    log_verbose("Read %zd bytes.\n", len);

    return len;
}

static int http_write(UpnpWebFileHandle fh __attribute__((unused)),
        char *buf __attribute__((unused)),
        size_t buflen __attribute__((unused))) {
    log_verbose("http write\n");
    printf("http write\n");

    return 0;
}

static int http_seek(UpnpWebFileHandle fh, off_t offset, int origin) {
    struct web_file_t *file = (struct web_file_t *) fh;
    off_t newpos = -1;

    //log_verbose("http_seek\n");

    if (!file)
        return -1;
    if (file->type == FILE_LIVE) {
        //log_verbose("##\nLIVE FILE SEEK origin %d offset %d\n##", origin, offset);
        return 0;
    }


    switch (origin) {
        case SEEK_SET:
            log_verbose("Attempting to seek to %lld (was at %lld) in %s\n",
                    offset, file->pos, file->fullpath);

            newpos = offset;
            break;
        case SEEK_CUR:
            log_verbose("Attempting to seek by %lld from %lld in %s\n",
                    offset, file->pos, file->fullpath);
            newpos = file->pos + offset;
            break;
        case SEEK_END:
            log_verbose("Attempting to seek by %lld from end (was at %lld) in %s\n",
                    offset, file->pos, file->fullpath);

            if (file->type == FILE_LOCAL) {
                struct stat sb;
                if (stat(file->fullpath, &sb) < 0) {
                    log_verbose("%s: cannot stat: %s\n",
                            file->fullpath, strerror(errno));
                    return -1;
                }
                newpos = sb.st_size + offset;
            } else if (file->type == FILE_MEMORY)
                newpos = file->detail.memory.len + offset;
            break;
    }

    switch (file->type) {
        case FILE_LOCAL:
            /* Just make sure we cannot seek before start of file. */
            if (newpos < 0) {
                log_verbose("%s: cannot seek: %s\n", file->fullpath, strerror(EINVAL));
                return -1;
            }

            /* Don't seek with origin as specified above, as file may have
               changed in size since our last stat. */
            if (lseek(file->detail.local.fd, newpos, SEEK_SET) == -1) {
                log_verbose("%s: cannot seek: %s\n", file->fullpath, strerror(errno));
                return -1;
            }
            break;
        case FILE_MEMORY:
            if (newpos < 0 || newpos > file->detail.memory.len) {
                log_verbose("%s: cannot seek: %s\n", file->fullpath, strerror(EINVAL));
                printf("%s: cannot seek: %s\n", file->fullpath, strerror(EINVAL));
                return -1;
            }
            break;
    }

    file->pos = newpos;



    return 0;
}

static int http_close(UpnpWebFileHandle fh) {
    struct web_file_t *file = (struct web_file_t *) fh;

    log_verbose("http_close\n");

    if (!file)
        return -1;

    switch (file->type) {
        case FILE_LOCAL:
            close(file->detail.local.fd);
            break;
        case FILE_MEMORY:
            /* no close operation */
            if (file->detail.memory.contents)
                free(file->detail.memory.contents);
            break;
        case FILE_LIVE:
            log_verbose("Live file not closing pipe\n");
            break;
        default:
            log_verbose("Unknown file type.\n");
            break;
    }

    if (file->fullpath)
        free(file->fullpath);
    free(file);

    return 0;
}

struct UpnpVirtualDirCallbacks virtual_dir_callbacks = {
    http_get_info,
    http_open,
    http_read,
    http_write,
    http_seek,
    http_close
};
