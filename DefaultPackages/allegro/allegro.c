//
//  allegro.c
//  Packages
//
//  Created by Theo Weidmann on 14.07.2016.
//  Copyright (c) 2016 Theo Weidmann. All rights reserved.
//

#include "EmojicodeAPI.h"
#include "EmojicodeString.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

static Class *CL_EVENT;
static Class *CL_EVENT_KEY_CHAR;
static Class *CL_EVENT_KEY_DOWN;
static Class *CL_EVENT_KEY_UP;
static Class *CL_BITMAP;
static Class *CL_EVENT_MOUSE_AXES;
static Class *CL_EVENT_MOUSE_DOWN;
static Class *CL_EVENT_MOUSE_UP;
static Class *CL_EVENT_TIMER;

#define display(c) (*(ALLEGRO_DISPLAY **)(c)->value)
#define bitmap(c) (*(ALLEGRO_BITMAP **)(c)->value)
#define font(c) (*(ALLEGRO_FONT **)(c)->value)
#define color(c) (*(ALLEGRO_COLOR *)(c)->value)
#define eventQueue(c) (*(ALLEGRO_EVENT_QUEUE **)(c)->value)
#define event(c) (*(ALLEGRO_EVENT *)(c)->value)
#define sample(c) (*(ALLEGRO_SAMPLE **)(c)->value)
#define timer(c) (*(ALLEGRO_TIMER **)(c)->value)

PackageVersion getVersion() {
    return (PackageVersion){0, 1};
}

static Something emojicodeMain;
static Thread *thread;

static int redundantMain(int argc, char **argv) {
    al_init();
    al_install_keyboard();
    al_install_mouse();
    al_init_primitives_addon();
    al_init_image_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_install_audio();
    al_init_acodec_addon();
    executeCallableExtern(emojicodeMain.object, NULL, thread);
    return 0;
}

Something appInit(Thread *e) {
    emojicodeMain = stackvariable(0, e);
    thread = e;
    al_run_main(0, NULL, redundantMain);
    return NOTHINGNESS;
}

void displayInitWithDimensions(Thread *thread) {
    int w = (int)stackvariable(0, thread).raw;
    int h = (int)stackvariable(1, thread).raw;
    display(stackthisObject(thread)) = al_create_display(w, h);
}

void displayDe(void *v) {
    al_destroy_display(*(ALLEGRO_DISPLAY **)v);
}

Something appFlip(Thread *thread) {
    al_flip_display();
    return NOTHINGNESS;
}

Something appDrawLine(Thread *thread) {
    float x1 = (float)stackvariable(0, thread).doubl;
    float y1 = (float)stackvariable(1, thread).doubl;
    float x2 = (float)stackvariable(2, thread).doubl;
    float y2 = (float)stackvariable(3, thread).doubl;
    float t = (float)stackvariable(5, thread).doubl;
    al_draw_line(x1, y1, x2, y2, color(stackvariable(4, thread).object), t);
    return NOTHINGNESS;
}

Something appDrawTriangle(Thread *thread) {
    float x1 = (float)stackvariable(0, thread).doubl;
    float y1 = (float)stackvariable(1, thread).doubl;
    float x2 = (float)stackvariable(2, thread).doubl;
    float y2 = (float)stackvariable(3, thread).doubl;
    float x3 = (float)stackvariable(4, thread).doubl;
    float y3 = (float)stackvariable(5, thread).doubl;
    float t = (float)stackvariable(7, thread).doubl;
    al_draw_triangle(x1, y1, x2, y2, x3, y3, color(stackvariable(6, thread).object), t);
    return NOTHINGNESS;
}

Something appDrawFilledTriangle(Thread *thread) {
    float x1 = (float)stackvariable(0, thread).doubl;
    float y1 = (float)stackvariable(1, thread).doubl;
    float x2 = (float)stackvariable(2, thread).doubl;
    float y2 = (float)stackvariable(3, thread).doubl;
    float x3 = (float)stackvariable(4, thread).doubl;
    float y3 = (float)stackvariable(5, thread).doubl;
    al_draw_filled_triangle(x1, y1, x2, y2, x3, y3, color(stackvariable(6, thread).object));
    return NOTHINGNESS;
}

Something displaySetTitle(Thread *thread) {
    char *title = stringToChar(stackvariable(0, thread).object->value);
    al_set_window_title(display(stackthisObject(thread)), title);
    free(title);
    return NOTHINGNESS;
}

Something appDrawRectangle(Thread *thread) {
    float x1 = (float)stackvariable(0, thread).doubl;
    float y1 = (float)stackvariable(1, thread).doubl;
    float x2 = (float)stackvariable(2, thread).doubl;
    float y2 = (float)stackvariable(3, thread).doubl;
    float t = (float)stackvariable(5, thread).doubl;
    al_draw_rectangle(x1, y1, x2, y2, color(stackvariable(4, thread).object), t);
    return NOTHINGNESS;
}

Something appDrawFilledRectangle(Thread *thread) {
    float x1 = (float)stackvariable(0, thread).doubl;
    float y1 = (float)stackvariable(1, thread).doubl;
    float x2 = (float)stackvariable(2, thread).doubl;
    float y2 = (float)stackvariable(3, thread).doubl;
    al_draw_filled_rectangle(x1, y1, x2, y2, color(stackvariable(4, thread).object));
    return NOTHINGNESS;
}

Something appClear(Thread *thread) {
    al_clear_to_color(color(stackvariable(0, thread).object));
    return NOTHINGNESS;
}

Something appDrawCircle(Thread *thread) {
    float cx = (float)stackvariable(0, thread).doubl;
    float cy = (float)stackvariable(1, thread).doubl;
    float r = (float)stackvariable(2, thread).doubl;
    float t = (float)stackvariable(4, thread).doubl;
    al_draw_circle(cx, cy, r, color(stackvariable(3, thread).object), t);
    return NOTHINGNESS;
}

Something appDrawFilledCircle(Thread *thread) {
    float cx = (float)stackvariable(0, thread).doubl;
    float cy = (float)stackvariable(1, thread).doubl;
    float r = (float)stackvariable(2, thread).doubl;
    al_draw_filled_circle(cx, cy, r, color(stackvariable(3, thread).object));
    return NOTHINGNESS;
}

Something appDrawRoundedRectangle(Thread *thread) {
    float x1 = (float)stackvariable(0, thread).doubl;
    float y1 = (float)stackvariable(1, thread).doubl;
    float x2 = (float)stackvariable(2, thread).doubl;
    float y2 = (float)stackvariable(3, thread).doubl;
    float rx = (float)stackvariable(4, thread).doubl;
    float ry = (float)stackvariable(5, thread).doubl;
    float t = (float)stackvariable(7, thread).doubl;
    al_draw_rounded_rectangle(x1, y1, x2, y2, rx, ry, color(stackvariable(6, thread).object), t);
    return NOTHINGNESS;
}

Something appDrawFilledRoundedRectangle(Thread *thread) {
    float x1 = (float)stackvariable(0, thread).doubl;
    float y1 = (float)stackvariable(1, thread).doubl;
    float x2 = (float)stackvariable(2, thread).doubl;
    float y2 = (float)stackvariable(3, thread).doubl;
    float rx = (float)stackvariable(4, thread).doubl;
    float ry = (float)stackvariable(5, thread).doubl;
    al_draw_filled_rounded_rectangle(x1, y1, x2, y2, rx, ry, color(stackvariable(6, thread).object));
    return NOTHINGNESS;
}

Something appDrawBitmap(Thread *thread) {
    ALLEGRO_BITMAP *bmp = bitmap(stackvariable(0, thread).object);
    float dx = (float)stackvariable(1, thread).doubl;
    float dy = (float)stackvariable(2, thread).doubl;
    al_draw_bitmap(bmp, dx, dy, 0);
    return NOTHINGNESS;
}

Something appDrawScaledBitmap(Thread *thread) {
    ALLEGRO_BITMAP *bmp = bitmap(stackvariable(0, thread).object);
    float sx = (float)stackvariable(1, thread).doubl;
    float sy = (float)stackvariable(2, thread).doubl;
    float sw = (float)stackvariable(3, thread).doubl;
    float sh = (float)stackvariable(4, thread).doubl;
    float dx = (float)stackvariable(5, thread).doubl;
    float dy = (float)stackvariable(6, thread).doubl;
    float dw = (float)stackvariable(7, thread).doubl;
    float dh = (float)stackvariable(8, thread).doubl;
    al_draw_scaled_bitmap(bmp, sx, sy, sw, sh, dx, dy, dw, dh, 0);
    return NOTHINGNESS;
}

Something appDrawText(Thread *thread) {
    ALLEGRO_FONT *font = font(stackvariable(0, thread).object);
    char *text = stringToChar(stackvariable(4, thread).object->value);
    float x = (float)stackvariable(2, thread).doubl;
    float y = (float)stackvariable(3, thread).doubl;
    al_draw_text(font, color(stackvariable(1, thread).object), x, y, (int)stackvariable(5, thread).raw, text);
    free(text);
    return NOTHINGNESS;
}

Something appSetTargetBitmap(Thread *thread) {
    ALLEGRO_BITMAP *bmp = bitmap(stackvariable(0, thread).object);
    al_set_target_bitmap(bmp);
    return NOTHINGNESS;
}

Something appSetTargetBackbuffer(Thread *thread) {
    ALLEGRO_DISPLAY *display = display(stackvariable(0, thread).object);
    al_set_target_backbuffer(display);
    return NOTHINGNESS;
}

void colorInitRGBA(Thread *thread) {
    int r = (int)stackvariable(0, thread).raw;
    int g = (int)stackvariable(1, thread).raw;
    int b = (int)stackvariable(2, thread).raw;
    int a = (int)stackvariable(3, thread).raw;
    color(stackthisObject(thread)) = al_map_rgba(r, g, b, a);
}

void bitmapInitFile(Thread *thread) {
    char *path = stringToChar(stackvariable(0, thread).object->value);
    ALLEGRO_BITMAP *bmp = al_load_bitmap(path);
    if (bmp) {
        bitmap(stackthisObject(thread)) = bmp;
    }
    else {
        stackthisObject(thread)->value = NULL;
    }
    free(path);
}

void bitmapDe(void *v) {
    al_destroy_bitmap(*(ALLEGRO_BITMAP **)v);
}

void bitmapInitSize(Thread *thread) {
    ALLEGRO_BITMAP *bmp = al_create_bitmap((int)stackvariable(0, thread).raw, (int)stackvariable(1, thread).raw);
    if (bmp) {
        bitmap(stackthisObject(thread)) = bmp;
    }
    else {
        stackthisObject(thread)->value = NULL;
    }
}

void fontInitFile(Thread *thread) {
    char *path = stringToChar(stackvariable(0, thread).object->value);
    ALLEGRO_FONT *font = al_load_ttf_font(path, (int)stackvariable(1, thread).raw, 0);
    if (font) {
        font(stackthisObject(thread)) = font;
    }
    else {
        stackthisObject(thread)->value = NULL;
    }
    free(path);
}

void fontDe(void *v) {
    al_destroy_font(*(ALLEGRO_FONT **)v);
}

void eventQueueInit(Thread *thread) {
    eventQueue(stackthisObject(thread)) = al_create_event_queue();
}

void eventQueueDe(void *v) {
    al_destroy_event_queue(*(ALLEGRO_EVENT_QUEUE **)v);
}

Something eventQueueWait(Thread *thread) {
    Object *event = newObject(CL_EVENT);
    al_wait_for_event(eventQueue(stackthisObject(thread)), event->value);
    switch (event(event).type) {
        case ALLEGRO_EVENT_KEY_CHAR:
            event->class = CL_EVENT_KEY_CHAR;
            break;
        case ALLEGRO_EVENT_KEY_DOWN:
            event->class = CL_EVENT_KEY_DOWN;
            break;
        case ALLEGRO_EVENT_KEY_UP:
            event->class = CL_EVENT_KEY_UP;
            break;
        case ALLEGRO_EVENT_MOUSE_AXES:
            event->class = CL_EVENT_MOUSE_AXES;
            break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            event->class = CL_EVENT_MOUSE_DOWN;
            break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            event->class = CL_EVENT_MOUSE_UP;
            break;
        case ALLEGRO_EVENT_TIMER:
            event->class = CL_EVENT_TIMER;
            break;
        default:
            break;
    }
    return somethingObject(event);
}

Something eventQueueRegisterMouse(Thread *thread) {
    al_register_event_source(eventQueue(stackthisObject(thread)), al_get_mouse_event_source());
    return NOTHINGNESS;
}

Something eventQueueRegisterKeyboard(Thread *thread) {
    al_register_event_source(eventQueue(stackthisObject(thread)), al_get_keyboard_event_source());
    return NOTHINGNESS;
}

Something eventQueueRegisterTimer(Thread *thread) {
    al_register_event_source(eventQueue(stackthisObject(thread)),
                             al_get_timer_event_source(timer(stackvariable(0, thread).object)));
    return NOTHINGNESS;
}

Something keyboardEventKeycode(Thread *thread) {
    return somethingInteger(event(stackthisObject(thread)).keyboard.keycode);
}

Something keyboardEventSymbol(Thread *thread) {
    EmojicodeInteger symbol = event(stackthisObject(thread)).keyboard.unichar;
    return symbol < 1 ? NOTHINGNESS : somethingSymbol(symbol);
}

Something keyboardEventRepeated(Thread *thread) {
    return somethingBoolean(event(stackthisObject(thread)).keyboard.repeat);
}

Something mouseAxesEventX(Thread *thread) {
    return somethingDouble(event(stackthisObject(thread)).mouse.x);
}

Something mouseAxesEventY(Thread *thread) {
    return somethingDouble(event(stackthisObject(thread)).mouse.y);
}

void sampleInitFile(Thread *thread) {
    char *path = stringToChar(stackvariable(0, thread).object->value);
    ALLEGRO_SAMPLE *sample = al_load_sample(path);
    if (sample) {
        sample(stackthisObject(thread)) = sample;
    }
    else {
        stackthisObject(thread)->value = NULL;
    }
    free(path);
}

void sampleDe(void *v) {
    al_destroy_sample(*(ALLEGRO_SAMPLE **)v);
}

Something samplePlay(Thread *thread) {
    ALLEGRO_SAMPLE *sample = sample(stackthisObject(thread));
    float gain = (float)stackvariable(0, thread).doubl;
    float pan = (float)stackvariable(1, thread).doubl;
    float speed = (float)stackvariable(2, thread).doubl;
    al_reserve_samples(1);
    al_play_sample(sample, gain, pan, speed, ALLEGRO_PLAYMODE_ONCE, NULL);
    return NOTHINGNESS;
}

void timerInit(Thread *thread) {
    timer(stackthisObject(thread)) = al_create_timer(stackvariable(0, thread).doubl);
}

Something timerStart(Thread *thread) {
    al_start_timer(timer(stackthisObject(thread)));
    return NOTHINGNESS;
}

Something timerStop(Thread *thread) {
    al_stop_timer(timer(stackthisObject(thread)));
    return NOTHINGNESS;
}

Something timerResume(Thread *thread) {
    al_resume_timer(timer(stackthisObject(thread)));
    return NOTHINGNESS;
}

FunctionFunctionPointer handlerPointerForMethod(EmojicodeChar cl, EmojicodeChar symbol, MethodType t) {
    switch (symbol) {
        case 0x1f64b: //🙋
            return appInit;
        case 0x1f3a6: //🎦
            return appFlip;
        case 0x1f3f7: //🏷
            return displaySetTitle;
        case 0x1f58d: //🖍
            return appDrawLine;
        case 0x1f4d0: //📐
            return appDrawTriangle;
        case 0x1f6a9: //🚩
            return appDrawFilledTriangle;
        case 0x1f4cb: //📋
            return appDrawRectangle;
        case 0x1f5c4: //🗄
            return appDrawFilledRectangle;
        case 0x1f6bf: //🚿
            return appClear;
        case 0x26bd: //⚽
            return appDrawCircle;
        case 0x1f3c0: //🏀
            return appDrawFilledCircle;
        case 0x1f3c9: //🏉
            return appDrawRoundedRectangle;
        case 0x1f3c8: //🏈
            return appDrawFilledRoundedRectangle;
        case 0x1f4fc: //📼
            return appDrawBitmap;
        case 0x1f4bd: //💽
            return appDrawScaledBitmap;
        case 0x1f521: //🔡
            return appDrawText;
        case 0x1f3d3: //🏓
            return appSetTargetBitmap;
        case 0x1f3cf: //🏏
            return appSetTargetBackbuffer;
        case 0x23f3: //⏳
            return eventQueueWait;
        case 0x1f5b1: //🖱
            return eventQueueRegisterMouse;
        case 0x2328: //⌨
            return eventQueueRegisterKeyboard;
        case 0x23f2: //⏲
            return eventQueueRegisterTimer;
        case 0x1f4df: //📟
            return keyboardEventKeycode;
        case 0x1f523: //🔣
            return keyboardEventSymbol;
        case 0x1f503: //🔃
            return keyboardEventRepeated;
        case 0x1f3c1: //🏁
            switch (cl) {
                case 0x23f2: //⏲
                    return timerStart;
                default:
                    return samplePlay;
            }
        case 0x1f449: //👉
            return mouseAxesEventX;
        case 0x1f447: //👇
            return mouseAxesEventY;
        case 0x1f6a6: //🚦
            return timerResume;
        case 0x26d4: //⛔
            return timerStop;
    }
    return NULL;
}

InitializerFunctionFunctionPointer handlerPointerForInitializer(EmojicodeChar cl, EmojicodeChar symbol) {
    switch (cl) {
        case 0x1f4fa: //📺
            return displayInitWithDimensions;
        case 0x1f3a8: //🎨
            return colorInitRGBA;
        case 0x23f2: //⏲
            return timerInit;
        case 0x1f5bc: //🖼
            switch (symbol) {
                case 0x1f4c4: //📄
                    return bitmapInitFile;
                case 0x1f195: //🆕
                    return bitmapInitSize;
            }
        case 0x1f549: //🕉
            return fontInitFile;
        case 0x1f5c3: //🗃
            return eventQueueInit;
        case 0x1f3b6: //🎶
            return sampleInitFile;
    }
    return NULL;
}

Marker markerPointerForClass(EmojicodeChar cl) {
    return NULL;
}

uint_fast32_t sizeForClass(Class *cl, EmojicodeChar name) {
    switch (name) {
        case 0x1f4fa: //📺
            return sizeof(ALLEGRO_DISPLAY*);
        case 0x1f3a8: //🎨
            return sizeof(ALLEGRO_COLOR);
        case 0x1f5bc: //🖼
            CL_BITMAP = cl;
            return sizeof(ALLEGRO_BITMAP*);
        case 0x1f549: //🕉
            return sizeof(ALLEGRO_FONT*);
        case 0x1f5c3: //🗃
            return sizeof(ALLEGRO_EVENT_QUEUE*);
        case 0x1f389: //🎉
            CL_EVENT = cl;
            return sizeof(ALLEGRO_EVENT);
        case 0x1f4e9: //📩
            CL_EVENT_KEY_CHAR = cl;
            break;
        case 0x1f4e4: //📤
            CL_EVENT_KEY_UP = cl;
            break;
        case 0x1f4e5: //📥
            CL_EVENT_KEY_DOWN = cl;
            break;
        case 0x2747: //❇
            CL_EVENT_MOUSE_AXES = cl;
            break;
        case 0x1f51b: //🔛
            CL_EVENT_MOUSE_DOWN = cl;
            break;
        case 0x1f51d: //🔝
            CL_EVENT_MOUSE_UP = cl;
            break;
        case 0x1f6ce: //🛎
            CL_EVENT_TIMER = cl;
            break;
        case 0x1f3b6: //🎶
            return sizeof(ALLEGRO_SAMPLE*);
    }
    return 0;
}

Deinitializer deinitializerPointerForClass(EmojicodeChar cl) {
    switch (cl) {
        case 0x1f4fa: //📺
            return displayDe;
        case 0x1f5bc: //🖼
            return bitmapDe;
        case 0x1f549: //🕉
            return fontDe;
        case 0x1f5c3: //🗃
            return eventQueueDe;
        case 0x1f3b6: //🎶
            return sampleDe;
    }
    return NULL;
}
