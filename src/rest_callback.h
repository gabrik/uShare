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
int callback_test (const struct _u_request * request, struct _u_response * response, void * user_data) {
  ulfius_set_string_response(response, 200, "Hello World!");
  return U_OK;
}





#ifdef __cplusplus
}
#endif

#endif /* REST_CALLBACK_H */

