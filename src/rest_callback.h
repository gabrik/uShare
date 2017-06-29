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

    int callback_rescanning(const struct _u_request * request, struct _u_response * response, void * user_data) {


        ulfius_set_string_response(response, 200, "{\"status\":\"success\"}");

        //clear_all();
        return U_OK;
    }

    int callback_remove(const struct _u_request * request, struct _u_response * response, void * user_data) {
        char* url = request->http_url;

        const char* key;
        json_t* value;
        json_error_t error;
        json_t* obj = request->json_body; //json_object();

        printf("Called REST URL %s", url);

        response->string_body = nstrdup("{\"status\":\"success\"}");
        response->status = 200;

        //printf("Parameter is %s",obj);

        /*obj=json_loads(data_json,0,&error);
        if(!obj){
            printf("Error on JSON Parsing on line %d: %s",error.line,error.text);
            return U_ERROR;
        }*/



        printf("JSON Object of %zd pairs:\n", json_object_size(obj));

        json_object_foreach(obj, key, value) {
            printf("JSON Key: \"%s\"\n", key);
            switch (json_typeof(value)) {
                case JSON_STRING:
                    printf("JSON String: \"%s\"\n", json_string_value(value));
                    break;
                case JSON_INTEGER:
                    printf("JSON Integer: \"%lld\"\n", json_integer_value(value));
                    break;
            }


            if (strcmp(key, "address") == 0) {
                char* id = json_string_value(value);
                printf("Removing Live Stream %s\n", id);


                // http://172.16.0.8:8090/InputProvider1.mp4

                char* url = get_pa_media_uri(id);
                
                
                
                live_objects_t obj; // = calloc(1,sizeof(live_transcoding_t));
                obj.src = strndup(url, strlen(url));
                
                GSList* gs = g_slist_find_custom(stream_map, (gconstpointer) &obj, (GCompareFunc) g_cmpfunc_stream);
                if (gs) {
                        live_objects_t* f=(live_objects_t*) gs->data;
                        
                        clear_obj(f->c_id,f->id);
                        
                        stream_map = g_slist_remove(stream_map, (gconstpointer) f);
                        printf("New list size is %o\n", g_slist_length(stream_map));
                    }
                }
        }
        json_decref(obj);

        return U_OK;

    }

    int callback_add_live_media(const struct _u_request * request, struct _u_response * response, void * user_data) {

        char* url = request->http_url;
        //char* data_json = u_map_get(request->map_url,"data");        
        const char* key;
        json_t* value;
        json_error_t error;
        json_t* obj = request->json_body; //json_object();

        printf("Called REST URL %s", url);

        response->string_body = nstrdup("{\"status\":\"success\"}");
        response->status = 200;

        //printf("Parameter is %s",obj);

        /*obj=json_loads(data_json,0,&error);
        if(!obj){
            printf("Error on JSON Parsing on line %d: %s",error.line,error.text);
            return U_ERROR;
        }*/



        printf("JSON Object of %zd pairs:\n", json_object_size(obj));

        json_object_foreach(obj, key, value) {
            printf("JSON Key: \"%s\"\n", key);
            switch (json_typeof(value)) {
                case JSON_STRING:
                    printf("JSON String: \"%s\"\n", json_string_value(value));
                    break;
                case JSON_INTEGER:
                    printf("JSON Integer: \"%lld\"\n", json_integer_value(value));
                    break;
            }


            if (strcmp(key, "address") == 0) {
                char* id = json_string_value(value);
                printf("Adding Live Stream %s\n", id);

                char *name = strndup(id, strlen(id));


                pthread_t pa_th;
                if (pthread_create(&pa_th, NULL, t_add_from_pa, (void*) name)) {
                    perror("Error creating thread\n");
                }





            }


        }
        json_decref(obj);




        //ulfius_set_string_body_response(response, 200, "Hello World!");
        return U_OK;
    }

    int callback_add_source(const struct _u_request * request, struct _u_response * response, void * user_data) {

        char* url = request->http_url;
        //char * url_params = print_map(request->map_url);
        char* media_src = request->json_body;
        printf("Called add source from url %s\n", url);

        response->string_body = nstrdup("ok");
        response->status = 200;
        //url_params
        //printf("Params are: %s\n", url_params);
        printf("Values are: %s\n", media_src);
        printf("length %zd\n", strlen(media_src));


        //safe passing parameters
        src_message_t* msg;
        msg = calloc(1, sizeof (src_message_t));
        msg->lenght = strlen(media_src);
        msg->src = calloc(msg->lenght, sizeof (char));

        strcpy(msg->src, media_src);



        pthread_t ffmpeg_pipe;
        if (pthread_create(&ffmpeg_pipe, NULL, ffmpeg_open, (void *) msg)) {
            fprintf(stderr, "Error creating thread\n");
            return U_ERROR;
        }

        return U_OK;
    }



#ifdef __cplusplus
}
#endif

#endif /* REST_CALLBACK_H */

