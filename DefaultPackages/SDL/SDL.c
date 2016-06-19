//
//  SDL.c
//  DefaultPackages
//
//  Created by Theo Weidmann on 01/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "SDLPackage.h"

#define nullOrValue(sth) (isNothingness(sth) ? NULL : (sth).object->value)

#define windowName 0x1F5BC //🖼
#define rendererName 0x1F58C //🖌
#define rectName 0x25FE //◾️
#define appName 0x1F579 // 🕹
#define keyboardEventName 0x2328
#define mouseEventName 0x1F5B1
#define eventName 0x1F389
#define surfaceName 0x26F8 //⛸
#define textureName 0x1F32B //🌫

Class *event;
Class *keyboardEvent;
Class *mouseButtonEvent;

static Something appSetup(Thread *thread){
    if (SDL_Init(SDL_INIT_VIDEO)) {
        return somethingObject(newError(SDL_GetError(), 0));
    }
    return NOTHINGNESS;
}

static Something appQuit(Thread *thread){
    SDL_Quit();
    return NOTHINGNESS;
}

static Something appDelay(Thread *thread){
    SDL_Delay((uint32_t)unwrapInteger(stackGetVariable(0, thread)));
    return NOTHINGNESS;
}

static Something appPollEvent(Thread *thread){
    SDL_Event e;
    if (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                Object *keyboard = newObject(keyboardEvent);
                *(SDL_KeyboardEvent *)keyboard->value = e.key;
                return somethingObject(keyboard);
            }
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                Object *keyboard = newObject(mouseButtonEvent);
                *(SDL_KeyboardEvent *)keyboard->value = e.key;
                return somethingObject(keyboard);
            }
            default:
                return somethingObject(newObject(event));
        }
    }
    return NOTHINGNESS;
}

static void windowInit(Thread *thread){
    char *str = stringToChar(stackGetVariable(0, thread).object->value);
    int x = (int)unwrapInteger(stackGetVariable(1, thread));
    int y = (int)unwrapInteger(stackGetVariable(2, thread));
    int w = (int)unwrapInteger(stackGetVariable(3, thread));
    int h = (int)unwrapInteger(stackGetVariable(4, thread));
    *(SDL_Window **)stackGetThisObject(thread)->value = SDL_CreateWindow(str, x, y, w, h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    free(str);
}

static void windowDeinit(void *w){
    SDL_DestroyWindow(*(SDL_Window **)w);
}

static void rendererInit(Thread *thread){
    SDL_Window *window = *(SDL_Window **)stackGetVariable(0, thread).object->value;
    *(SDL_Renderer **)stackGetThisObject(thread)->value = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
}

static void rendererDeinit(void *r){
    SDL_DestroyRenderer(*(SDL_Renderer **)r);
}

static Something rendererDrawLine(Thread *thread){
    SDL_Renderer *renderer = *(SDL_Renderer **)stackGetThisObject(thread)->value;
    int x1 = (int)unwrapInteger(stackGetVariable(0, thread));
    int y1 = (int)unwrapInteger(stackGetVariable(1, thread));
    int x2 = (int)unwrapInteger(stackGetVariable(2, thread));
    int y2 = (int)unwrapInteger(stackGetVariable(3, thread));
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    SDL_Delay(2000);
    return NOTHINGNESS;
}

static Something rendererFillRect(Thread *thread){
    SDL_Renderer *renderer = *(SDL_Renderer **)stackGetThisObject(thread)->value;
    SDL_RenderFillRect(renderer, (SDL_Rect *)stackGetVariable(0, thread).object->value);
    return NOTHINGNESS;
}

static Something rendererDrawRect(Thread *thread){
    SDL_Renderer *renderer = *(SDL_Renderer **)stackGetThisObject(thread)->value;
    SDL_RenderDrawRect(renderer, (SDL_Rect *)stackGetVariable(0, thread).object->value);
    return NOTHINGNESS;
}

static Something rendererSetDrawColor(Thread *thread){
    SDL_Renderer *renderer = *(SDL_Renderer **)stackGetThisObject(thread)->value;
    int r = (uint8_t)unwrapInteger(stackGetVariable(0, thread));
    int g = (uint8_t)unwrapInteger(stackGetVariable(1, thread));
    int b = (uint8_t)unwrapInteger(stackGetVariable(2, thread));
    int a = (uint8_t)unwrapInteger(stackGetVariable(3, thread));
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    return NOTHINGNESS;
}

static Something rendererPresent(Thread *thread){
    SDL_Renderer *renderer = *(SDL_Renderer **)stackGetThisObject(thread)->value;
    SDL_RenderPresent(renderer);
    return NOTHINGNESS;
}

static Something rendererClear(Thread *thread){
    SDL_Renderer *renderer = *(SDL_Renderer **)stackGetThisObject(thread)->value;
    SDL_RenderClear(renderer);
    return NOTHINGNESS;
}

static void rectInit(Thread *thread){
    SDL_Rect *rect = stackGetThisObject(thread)->value;
    rect->x = (int)unwrapInteger(stackGetVariable(0, thread));
    rect->y = (int)unwrapInteger(stackGetVariable(1, thread));
    rect->w = (int)unwrapInteger(stackGetVariable(2, thread));
    rect->h = (int)unwrapInteger(stackGetVariable(3, thread));
}

static Something rendererCopyTexture(Thread *thread){
    SDL_Renderer *renderer = *(SDL_Renderer **)stackGetThisObject(thread)->value;
    Something source = stackGetVariable(1, thread);
    Something destination = stackGetVariable(2, thread);
    SDL_RenderCopy(renderer, *(SDL_Texture **)stackGetVariable(0, thread).object->value, nullOrValue(source), nullOrValue(destination));
    return NOTHINGNESS;
}

static void textureInitFromSurface(Thread *thread){
    SDL_Renderer **renderer = stackGetVariable(0, thread).object->value;
    SDL_Surface **surface = stackGetVariable(1, thread).object->value;
    *(SDL_Texture **)stackGetThisObject(thread)->value = SDL_CreateTextureFromSurface(*renderer, *surface);
}


static void surfaceInitFromBMP(Thread *thread){
    char *path = stringToChar(stackGetVariable(0, thread).object->value);
    SDL_Surface *surface = SDL_LoadBMP(path);
    
    if (surface == NULL) {
        stackGetThisObject(thread)->value = NULL;
        return;
    }
    
    *(SDL_Surface**)stackGetThisObject(thread)->value = surface;
    free(path);
}

static void surfaceDeinit(void *r){
    SDL_FreeSurface(*(SDL_Surface **)r);
}


//MARK: Events

static Something keyboardEventDown(Thread *thread){
    SDL_KeyboardEvent *e = stackGetThisObject(thread)->value;
    return e->type == SDL_KEYDOWN ? EMOJICODE_TRUE : EMOJICODE_FALSE;
}

static Something keyboardEventSymbol(Thread *thread){
    SDL_KeyboardEvent *e = stackGetThisObject(thread)->value;
    return keyCodeToChar(e->keysym.sym);
}

static Something mouseButtonEventGetX(Thread *thread){
    SDL_MouseButtonEvent *e = stackGetThisObject(thread)->value;
    return somethingInteger(e->x);
}

static Something mouseButtonEventGetY(Thread *thread){
    SDL_MouseButtonEvent *e = stackGetThisObject(thread)->value;
    return somethingInteger(e->y);
}

static Something mouseButtonEventDown(Thread *thread){
    SDL_MouseButtonEvent *e = stackGetThisObject(thread)->value;
    return e->type == SDL_MOUSEBUTTONDOWN ? EMOJICODE_TRUE : EMOJICODE_FALSE;
}

FunctionFunctionPointer handlerPointerForMethod(EmojicodeChar cl, EmojicodeChar symbol, MethodType t){
    switch (cl) {
        case rendererName:
            switch (symbol) {
                case 0x1F4CF: //📏
                    return rendererDrawLine;
                case rectName:
                    return rendererFillRect;
                case 0x25FD: //◽️
                    return rendererDrawRect;
                case 0x1F3A8: //🎨
                    return rendererSetDrawColor;
                case 0x1F37E: //🍾
                    return rendererPresent;
                case 0x1F6BF: //🚿
                    return rendererClear;
                case textureName:
                    return rendererCopyTexture;
            }
        case keyboardEventName:
            switch (symbol) {
                case 0x2B07:
                    return keyboardEventDown;
                case 0x1F523:
                    return keyboardEventSymbol;
            }
        case mouseEventName:
            switch (symbol) {
                case 0x1F3F3:
                    return mouseButtonEventGetX;
                case 0x1F3F4:
                    return mouseButtonEventGetY;
                case 0x2B07:
                    return mouseButtonEventDown;
            }
        case appName:
            switch (symbol) {
                case 0x1F3AC: //🎬
                    return appSetup;
                case 0x26F3: //⛳️
                    return appQuit;
                case 0x1F570: //🕰
                    return appDelay;
                case eventName:
                    return appPollEvent;
            }
    }
    return NULL;
}

InitializerFunctionFunctionPointer handlerPointerForInitializer(EmojicodeChar cl, EmojicodeChar symbol){
    switch (cl) {
        case windowName:
            return windowInit;
        case rendererName:
            return rendererInit;
        case rectName:
            return rectInit;
        case textureName:
            return textureInitFromSurface;
        case surfaceName:
            return surfaceInitFromBMP; //📃 0x1F4C3
    }
    return NULL;
}

Deinitializer deinitializerPointerForClass(EmojicodeChar cl){
    switch (cl) {
        case windowName:
            return windowDeinit;
        case rendererName:
            return rendererDeinit;
        case surfaceName:
            return surfaceDeinit;
    }
    return NULL;
}

Marker markerPointerForClass(EmojicodeChar cl){
    return NULL;
}

uint_fast32_t sizeForClass(Class *cl, EmojicodeChar name) {
    switch (name) {
        case windowName:
            return sizeof(SDL_Window*);
        case rendererName:
            return sizeof(SDL_Renderer*);
        case rectName:
            return sizeof(SDL_Rect);
        case keyboardEventName:
            keyboardEvent = cl;
            return sizeof(SDL_KeyboardEvent);
        case mouseEventName:
            mouseButtonEvent = cl;
            return sizeof(SDL_MouseButtonEvent);
        case textureName:
            return sizeof(SDL_Texture*);
        case surfaceName:
            return sizeof(SDL_Surface*);
        case eventName:
            event = cl;
            break;
    }
    return 0;
}

PackageVersion getVersion(){
    return (PackageVersion){0, 1};
}

