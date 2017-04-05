/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   live.h
 * Author: gabriele
 *
 * Created on 28 marzo 2017, 18.39
 */

#ifndef LIVE_H
#define LIVE_H

#ifdef __cplusplus
extern "C" {
#endif

#define BUFFSIZE 4096 
#define CMDSIZE 512

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ushare.h"

    char *ffmpeg_cmd = "ffmpeg -threads auto -analyzeduration 10000000 -i mmsh://%s -y -threads auto -c:v mpeg2video -pix_fmt yuv420p -qscale:v 1 -r 24000/1001 -g 15 -c:a ac3 -b:a 384k -ac 2 -map 0:1 -map 0:0 -sn -f vob pipe:";

    char *ffcmd = "ffmpeg -threads auto -analyzeduration 10000000 -i %s -y -threads auto -c:v mpeg2video -pix_fmt yuv420p -qscale:v 1 -r 24000/1001 -g 15 -c:a ac3 -b:a 384k -ac 2 -map 0:1 -map 0:0 -sn -f vob pipe:";

    
    typedef struct src_message {
        char *src;
        int lenght;
    } src_message_t;
    
    
    typedef struct thread_message{
        char *filename;
        char *cmd;
    } th_msg;
    
    
    
    
    char *generate_ffmpeg_cmd(const char *src){
        char *cmd = calloc(strlen(ffmpeg_cmd)+strlen(src), sizeof (char));
        printf("Source is: %s\n", src);
        sprintf(cmd, ffcmd, src);
        printf("Command is: %s\n",cmd);
        return cmd;
    }
    
    void *ffmpeg_open(void *arg) {
        FILE *fp;
        char buff[BUFFSIZE];

        src_message_t *source = (src_message_t *) arg;

        char* media_src = source->src;
        //(strlen(ffmpeg_cmd)+strlen(media_src))
        //char *cmd = calloc(CMDSIZE, sizeof (char));

        //char *data = calloc(BUFFSIZE, sizeof (char));



        printf("Source is: %s\n", media_src);
        //sprintf(cmd, ffmpeg_cmd, media_src);

        add_source(media_src);
        
        
        
         //FILE *fp;
         int fd;
            //int rfd;
            //-analyzeduration 10000000
            char *base="ffmpeg -threads 4 -analyzeduration 10 -f mpegts -i http://%s -y -threads 4 -c:v mpeg2video -pix_fmt yuv420p -qscale:v 1 -r 24000/1001 -g 15 -c:a ac3 -b:a 384k -ac 2 -map 0:1 -map 0:0 -sn -b 20000k -f vob pipe:";
            char *cmd = calloc(strlen(base)+strlen(media_src), sizeof (char));
            printf("Source is: %s\n", media_src);
            sprintf(cmd, base, media_src);
        
        
            printf("Command is: %s\n", cmd);
            fp = popen(cmd, "r");
        
            if (!fp<0) {
                return NULL;
            }
            fd=fileno(fp);
            fcntl(fd, F_SETFL, O_RDONLY);
        
        live_transcoding_t obj;//=calloc(1,sizeof(live_transcoding_t));
            obj.id=5;
            obj.fd=fd;
            
            live_objects=(live_transcoding_t*)realloc(live_objects,(live_number+1)*sizeof(live_transcoding_t));
            live_objects[live_number]=obj;
            live_number++;

       /* printf("Command is: %s", cmd);
        if (!(fp = popen(cmd, "r"))) {
            exit(1);
        }


        while (fgets(buff, sizeof (buff), fp) != NULL) {
            printf(data, "%s", buff);

            
        }
        pclose(fp);*/
    }
    
    
    void *ffmpeg_to_pipe(void *arg){
        FILE* fp,*fo;
        char buff[BUFFSIZE];
        char *data = calloc(BUFFSIZE, sizeof (char));
        
        th_msg *msg = (th_msg *) arg;
        
        mkfifo(msg->filename, 0666);
        if ( (fo = open(msg->filename, O_WRONLY)) < 0)
            exit(1);
        
        printf("Command is: %s", msg->cmd);
        if (!(fp = popen(msg->cmd, "r"))) {
            exit(1);
        }

        

        while (fgets(buff, sizeof (buff), fp) != NULL) {
            printf(data, "%s", buff);
            if ( write(fo, data, strlen(data)) != strlen(data)) { 
            perror("Error on PIPE");
        }
        }
    }
    
    char *get_ff_filename(char *src){
        
        //FILE* fp,*fo;
        char* filename="/tmp/live.mp4";
        
        
        th_msg *msg = calloc(1,sizeof(th_msg));

        char *cmd = calloc(strlen(ffcmd)+strlen(src), sizeof (char));

         
        printf("Source is: %s\n", src);
        sprintf(cmd, ffcmd, src);
        
        msg->filename=calloc(strlen(filename),sizeof(char));
        msg->cmd=calloc(strlen(cmd),sizeof(char));
        
        strcpy(msg->cmd,cmd);
        strcpy(msg->filename,filename);
        
        pthread_t th;
        
        pthread_create(&th,NULL,ffmpeg_to_pipe,(void*)msg);
        
        return filename;
     
    }
        
    


#ifdef __cplusplus
}
#endif

#endif /* LIVE_H */

