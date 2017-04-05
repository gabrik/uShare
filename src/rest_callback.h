/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   rest_callback.h
 * Author: gabriele
 *
 * Created on 28 marzo 2017, 17.45
 */

#ifndef REST_CALLBACK_H
#define REST_CALLBACK_H

#ifdef __cplusplus
extern "C" {
#endif


#include <ulfius.h>
#include <jansson.h>
#include <string.h>
#include "live.h"

    char * print_map(const struct _u_map * map) {
        char * line, * to_return = NULL;
        const char **keys;
        int len, i;
        if (map != NULL) {
            keys = u_map_enum_keys(map);
            for (i = 0; keys[i] != NULL; i++) {
                len = snprintf(NULL, 0, "key is %s, value is %s\n", keys[i], u_map_get(map, keys[i]));
                line = malloc((len + 1) * sizeof (char));
                snprintf(line, (len + 1), "key is %s, value is %s\n", keys[i], u_map_get(map, keys[i]));
                if (to_return != NULL) {
                    len = strlen(to_return) + strlen(line) + 1;
                    to_return = realloc(to_return, (len + 1) * sizeof (char));
                } else {
                    to_return = malloc((strlen(line) + 1) * sizeof (char));
                    to_return[0] = 0;
                }
                strcat(to_return, line);
                free(line);
            }
            return to_return;
        } else {
            return NULL;
        }
    }

    int callback_test(const struct _u_request * request, struct _u_response * response, void * user_data) {
        ulfius_set_string_response(response, 200, "Hello World!");
        return U_OK;
    }

    int callback_add_source(const struct _u_request * request, struct _u_response * response, void * user_data) {
        
        char* url = request->http_url;
        //char * url_params = print_map(request->map_url);
        char* media_src = u_map_get(request->map_url, "ip");
        printf("Called add source from url %s\n",url);
        
        response->string_body = nstrdup("ok");
        response->status = 200;
        //url_params
        //printf("Params are: %s\n", url_params);
        printf("Values are: %s\n", media_src);
        printf("length %zd\n" , strlen(media_src));
        
        
        //safe passing parameters
        src_message_t* msg;
        msg=calloc(1,sizeof(src_message_t));
        msg->lenght=strlen(media_src);
        msg->src=calloc(msg->lenght,sizeof(char));
        
        strcpy(msg->src,media_src);
        
        
        
        pthread_t ffmpeg_pipe;
        if(pthread_create(&ffmpeg_pipe, NULL, ffmpeg_open,(void *)msg)) {
            fprintf(stderr, "Error creating thread\n");
            return U_ERROR;
        }
  
        return U_OK;
    }



#ifdef __cplusplus
}
#endif

#endif /* REST_CALLBACK_H */

