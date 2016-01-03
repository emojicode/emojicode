//
//  net.c
//  DefaultPackages
//
//  Created by Theo Weidmann on 01.04.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeAPI.h"
#include "EmojicodeString.h"
#include <curl/curl.h>
#include <string.h>

#define defaultUserAgent "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/600.5.17 (KHTML, like Gecko) Version/8.0.5 Safari/600.5.17"

//MARK: Shortcuts

static size_t saveData(void *buffer, size_t size, size_t nmemb, Data *data) {
    size_t add = size * nmemb;
    void *dBytes;
    if(data->length){
        dBytes = realloc(data->bytes, data->length + add);
    }
    else {
        dBytes = malloc(add);
    }
    if(!dBytes){
        //memory failure
        return 0;
    }
    memcpy(dBytes + data->length, buffer, add);
    data->bytes = dBytes;
    data->length += add;
    return add;
}

Object* dataFromURL(Class *c, Object **args, Thread *thread){
    CURL *curl;
    CURLcode res;
    
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, stringToChar(args[0]->value));
        /* example.com is redirected, so we tell libcurl to follow redirection */
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, defaultUserAgent);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, saveData);
        
        Data *rd = malloc(sizeof(Data));
        rd->length = 0;
        
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, rd);
        
        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
            return NOTHINGNESS;
        
        /* always cleanup */
        curl_easy_cleanup(curl);
        
        return newObject(rd, CL_DATA);
    }
    return NOTHINGNESS;
}

extern Object *dataUTF8ToString(Object *self, Object **args, Thread *thread);

Object* stringFromURL(Class *c, Object **args, Thread *thread){
    Object* data = dataFromURL(c, args, thread);
    if(data == NOTHINGNESS){
        return data;
    }
    return dataUTF8ToString(data, NULL, thread);
}

//MARK: Instance

typedef struct {
    CURL *curl;
    Object *delegate;
    Thread *thread;
} URLRequest;

static size_t handleData(void *buffer, size_t size, size_t nmemb, Object *self) {
    URLRequest *request = self->value;
    size_t csize = size * nmemb;
    
    char* bytes = malloc(csize);
    
    memcpy(bytes, buffer, csize);
    
    Data *data = malloc(sizeof(Data));
    data->bytes = bytes;
    data->length = csize;
    
    Object *args[] = {self, newObject(data, CL_DATA)};
    
    Method *method = getMethod(0x1F4E8, request->delegate->class);
    executeMethod(method, request->delegate, args, request->thread);
    
    return csize;
}

void* constructURLRequest(Object *self, Object **args, Thread *thread){
    URLRequest *request = malloc(sizeof(URLRequest));
    
    request->delegate = args[1];
    
    incrementReferences(request->delegate);
    
    request->curl = curl_easy_init();
    if(request->curl) {
        curl_easy_setopt(request->curl, CURLOPT_URL, stringToChar(args[0]->value));
        curl_easy_setopt(request->curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(request->curl, CURLOPT_USERAGENT, defaultUserAgent);
        curl_easy_setopt(request->curl, CURLOPT_WRITEFUNCTION, handleData);
        
        curl_easy_setopt(request->curl, CURLOPT_WRITEDATA, self);
    }
    
    return request;
}

void releaseURLRequest(void *f){
    URLRequest *request = f;
    if(request->curl){
        curl_easy_cleanup(request->curl);
    }
    
    decrementReferences(request->delegate);
    
    free(request);
}

Object* URLRequestGo(Object *self, Object **args, Thread *thread){
    URLRequest *request = self->value;
    request->thread = thread;
    
    CURLcode res = curl_easy_perform(request->curl);
    
    if(res != CURLE_OK)
        return newObject(newError(curl_easy_strerror(res), res), CL_ERROR);
        
    return NOTHINGNESS;
}

extern PackageVersion initPackage(EcUnicodeChar namespace) {
    Class *URLRequestClass = newClass(0x1F4E8, namespace, CL_OBJECT, releaseURLRequest); //📨
    
    Protocol *URLRequestDelegate = newProtocol(0x1F4EA, namespace); //📪
    protocolAddMethod(0x1F4E8, URLRequestDelegate, arguments(2, URLRequestClass, CL_DATA), typeForClass(CL_NOTHINGNESS)); //📨
    
    addClassMethod(0x1F4E1, URLRequestClass, arguments(1, CL_STRING), dataFromURL, typeForClassOptional(CL_DATA), PUBLIC, false);
    addClassMethod(0x1F4E0, URLRequestClass, arguments(1, CL_STRING), stringFromURL, typeForClassOptional(CL_STRING), PUBLIC, false);
    
    addConstructor(0x1F4C3, URLRequestClass, argumentsFromTypes(2, typeForClass(CL_STRING), typeForProtocol(URLRequestDelegate)), constructURLRequest, PUBLIC, false, false);
    addMethod(0x1F3C1, URLRequestClass, NULL, URLRequestGo, typeForClassOptional(CL_ERROR), PUBLIC, false);
    
    return (PackageVersion){0, 1};
}