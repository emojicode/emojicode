//
//  allegro.c
//  Packages
//
//  Created by Theo Weidmann on 14.07.2016.
//  Copyright (c) 2016 Theo Weidmann. All rights reserved.
//

#include "../../EmojicodeReal-TimeEngine/EmojicodeAPI.hpp"
#include "../../EmojicodeReal-TimeEngine/String.hpp"
#include "../../EmojicodeReal-TimeEngine/Thread.hpp"
#include "../../EmojicodeReal-TimeEngine/Class.hpp"
#include "../../EmojicodeReal-TimeEngine/Memory.hpp"
#include <allegro5/allegro.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>

using Emojicode::Class;
using Emojicode::Object;
using Emojicode::Thread;
using Emojicode::stringToCString;
using Emojicode::Value;
using Emojicode::EmojicodeInteger;

static Class *CL_APPLICATION;
static Class *CL_EVENT;
static Class *CL_EVENT_KEY_CHAR;
static Class *CL_EVENT_KEY_DOWN;
static Class *CL_EVENT_KEY_UP;
static Class *CL_BITMAP;
static Class *CL_EVENT_MOUSE_AXES;
static Class *CL_EVENT_MOUSE_DOWN;
static Class *CL_EVENT_MOUSE_UP;
static Class *CL_EVENT_TIMER;

#define display(c) (*(c)->val<ALLEGRO_DISPLAY *>())
#define bitmap(c) (*(c)->val<ALLEGRO_BITMAP *>())
#define font(c) (*(c)->val<ALLEGRO_FONT *>())
#define color(c) (*(ALLEGRO_COLOR *)(c)->value)
#define eventQueue(c) (*(c)->val<ALLEGRO_EVENT_QUEUE *>())
#define sample(c) (*(c)->val<ALLEGRO_SAMPLE *>())
#define timer(c) (*(c)->val<ALLEGRO_TIMER *>())

Emojicode::PackageVersion version(0, 1);

static Object *emojicodeMain;
static Thread *thread;

ALLEGRO_COLOR allegroColorFromColor(Value *valueType) {
    return al_map_rgba(valueType[0].raw, valueType[1].raw, valueType[2].raw, valueType[3].raw);
}

Emojicode::EmojicodeInteger errnoToError() {
    switch (al_get_errno()) {
        case EACCES:
            return 1;
        case EEXIST:
            return 2;
        case ENOMEM:
            return 3;
        case ENOSYS:
            return 4;
        case EDOM:
            return 5;
        case EINVAL:
            return 6;
        case EILSEQ:
            return 7;
        case ERANGE:
            return 8;
        case EPERM:
            return 9;
    }
    return 0;
}

static int redundantMain(int  /*argc*/, char ** /*argv*/) {
    al_init();
    al_install_keyboard();
    al_install_mouse();
    al_init_primitives_addon();
    al_init_image_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_install_audio();
    al_init_acodec_addon();
    Value args[] = {Value(Emojicode::newObject(CL_APPLICATION))};
    executeCallableExtern(emojicodeMain, args, 1, thread);
    return 0;
}

void appInit(Thread *e) {
    emojicodeMain = e->variable(0).object;
    thread = e;
    al_run_main(0, nullptr, redundantMain);
    e->returnFromFunction();
}

void displayInitWithDimensions(Thread *thread) {
    int w = static_cast<int>(thread->variable(0).raw);
    int h = static_cast<int>(thread->variable(1).raw);
    display(thread->thisObject()) = al_create_display(w, h);
    Emojicode::registerForDeinitialization(thread->thisObject());
    thread->returnFromFunction(thread->thisContext());
}

void appFlip(Thread *thread) {
    al_flip_display();
    thread->returnFromFunction();
}

void appDrawLine(Thread *thread) {
    float x1 = static_cast<float>(thread->variable(0).doubl);
    float y1 = static_cast<float>(thread->variable(1).doubl);
    float x2 = static_cast<float>(thread->variable(2).doubl);
    float y2 = static_cast<float>(thread->variable(3).doubl);
    float t = static_cast<float>(thread->variable(4).doubl);
    al_draw_line(x1, y1, x2, y2, allegroColorFromColor(thread->variableDestination(5)), t);
    thread->returnFromFunction();
}

void appDrawTriangle(Thread *thread) {
    float x1 = static_cast<float>(thread->variable(0).doubl);
    float y1 = static_cast<float>(thread->variable(1).doubl);
    float x2 = static_cast<float>(thread->variable(2).doubl);
    float y2 = static_cast<float>(thread->variable(3).doubl);
    float x3 = static_cast<float>(thread->variable(4).doubl);
    float y3 = static_cast<float>(thread->variable(5).doubl);
    float t = static_cast<float>(thread->variable(6).doubl);
    al_draw_triangle(x1, y1, x2, y2, x3, y3, allegroColorFromColor(thread->variableDestination(7)), t);
    thread->returnFromFunction();
}

void appDrawFilledTriangle(Thread *thread) {
    float x1 = static_cast<float>(thread->variable(0).doubl);
    float y1 = static_cast<float>(thread->variable(1).doubl);
    float x2 = static_cast<float>(thread->variable(2).doubl);
    float y2 = static_cast<float>(thread->variable(3).doubl);
    float x3 = static_cast<float>(thread->variable(4).doubl);
    float y3 = static_cast<float>(thread->variable(5).doubl);
    al_draw_filled_triangle(x1, y1, x2, y2, x3, y3, allegroColorFromColor(thread->variableDestination(6)));
    thread->returnFromFunction();
}

void displaySetTitle(Thread *thread) {
    const char *title = stringToCString(thread->variable(0).object);
    al_set_window_title(display(thread->thisObject()), title);
    thread->returnFromFunction();
}

void appDrawRectangle(Thread *thread) {
    float x1 = static_cast<float>(thread->variable(0).doubl);
    float y1 = static_cast<float>(thread->variable(1).doubl);
    float x2 = static_cast<float>(thread->variable(2).doubl);
    float y2 = static_cast<float>(thread->variable(3).doubl);
    float t = static_cast<float>(thread->variable(4).doubl);
    al_draw_rectangle(x1, y1, x2, y2, allegroColorFromColor(thread->variableDestination(5)), t);
    thread->returnFromFunction();
}

void appDrawFilledRectangle(Thread *thread) {
    float x1 = static_cast<float>(thread->variable(0).doubl);
    float y1 = static_cast<float>(thread->variable(1).doubl);
    float x2 = static_cast<float>(thread->variable(2).doubl);
    float y2 = static_cast<float>(thread->variable(3).doubl);
    al_draw_filled_rectangle(x1, y1, x2, y2, allegroColorFromColor(thread->variableDestination(4)));
    thread->returnFromFunction();
}

void appClear(Thread *thread) {
    al_clear_to_color(allegroColorFromColor(thread->variableDestination(0)));
    thread->returnFromFunction();
}

void appDrawCircle(Thread *thread) {
    float cx = static_cast<float>(thread->variable(0).doubl);
    float cy = static_cast<float>(thread->variable(1).doubl);
    float r = static_cast<float>(thread->variable(2).doubl);
    float t = static_cast<float>(thread->variable(3).doubl);
    al_draw_circle(cx, cy, r, allegroColorFromColor(thread->variableDestination(4)), t);
    thread->returnFromFunction();
}

void appDrawFilledCircle(Thread *thread) {
    float cx = static_cast<float>(thread->variable(0).doubl);
    float cy = static_cast<float>(thread->variable(1).doubl);
    float r = static_cast<float>(thread->variable(2).doubl);
    al_draw_filled_circle(cx, cy, r, allegroColorFromColor(thread->variableDestination(3)));
    thread->returnFromFunction();
}

void appDrawRoundedRectangle(Thread *thread) {
    float x1 = static_cast<float>(thread->variable(0).doubl);
    float y1 = static_cast<float>(thread->variable(1).doubl);
    float x2 = static_cast<float>(thread->variable(2).doubl);
    float y2 = static_cast<float>(thread->variable(3).doubl);
    float rx = static_cast<float>(thread->variable(4).doubl);
    float ry = static_cast<float>(thread->variable(5).doubl);
    float t = static_cast<float>(thread->variable(6).doubl);
    al_draw_rounded_rectangle(x1, y1, x2, y2, rx, ry, allegroColorFromColor(thread->variableDestination(7)), t);
    thread->returnFromFunction();
}

void appDrawFilledRoundedRectangle(Thread *thread) {
    float x1 = static_cast<float>(thread->variable(0).doubl);
    float y1 = static_cast<float>(thread->variable(1).doubl);
    float x2 = static_cast<float>(thread->variable(2).doubl);
    float y2 = static_cast<float>(thread->variable(3).doubl);
    float rx = static_cast<float>(thread->variable(4).doubl);
    float ry = static_cast<float>(thread->variable(5).doubl);
    al_draw_filled_rounded_rectangle(x1, y1, x2, y2, rx, ry, allegroColorFromColor(thread->variableDestination(6)));
    thread->returnFromFunction();
}

void appDrawBitmap(Thread *thread) {
    ALLEGRO_BITMAP *bmp = bitmap(thread->variable(0).object);
    float dx = static_cast<float>(thread->variable(1).doubl);
    float dy = static_cast<float>(thread->variable(2).doubl);
    al_draw_bitmap(bmp, dx, dy, 0);
    thread->returnFromFunction();
}

void appDrawScaledBitmap(Thread *thread) {
    ALLEGRO_BITMAP *bmp = bitmap(thread->variable(0).object);
    float sx = static_cast<float>(thread->variable(1).doubl);
    float sy = static_cast<float>(thread->variable(2).doubl);
    float sw = static_cast<float>(thread->variable(3).doubl);
    float sh = static_cast<float>(thread->variable(4).doubl);
    float dx = static_cast<float>(thread->variable(5).doubl);
    float dy = static_cast<float>(thread->variable(6).doubl);
    float dw = static_cast<float>(thread->variable(7).doubl);
    float dh = static_cast<float>(thread->variable(8).doubl);
    al_draw_scaled_bitmap(bmp, sx, sy, sw, sh, dx, dy, dw, dh, 0);
    thread->returnFromFunction();
}

void appDrawText(Thread *thread) {
    ALLEGRO_FONT *font = font(thread->variable(0).object);
    const char *text = stringToCString(thread->variable(1).object);
    float x = static_cast<float>(thread->variable(2).doubl);
    float y = static_cast<float>(thread->variable(3).doubl);
    al_draw_text(font, allegroColorFromColor(thread->variableDestination(5)), x, y,
                 static_cast<int>(thread->variable(4).raw), text);
    thread->returnFromFunction();
}

void appSetTargetBitmap(Thread *thread) {
    ALLEGRO_BITMAP *bmp = bitmap(thread->variable(0).object);
    al_set_target_bitmap(bmp);
    thread->returnFromFunction();
}

void appSetTargetBackbuffer(Thread *thread) {
    ALLEGRO_DISPLAY *display = display(thread->variable(0).object);
    al_set_target_backbuffer(display);
    thread->returnFromFunction();
}

void bitmapInitFile(Thread *thread) {
    const char *path = stringToCString(thread->variable(0).object);
    ALLEGRO_BITMAP *bmp = al_load_bitmap(path);
    if (bmp != nullptr) {
        bitmap(thread->thisObject()) = bmp;
        Emojicode::registerForDeinitialization(thread->thisObject());
        thread->returnOEValueFromFunction(thread->thisContext());
    }
    else {
        thread->returnErrorFromFunction(errnoToError());
    }
}

void bitmapInitSize(Thread *thread) {
    ALLEGRO_BITMAP *bmp = al_create_bitmap(static_cast<int>(thread->variable(0).raw), static_cast<int>(thread->variable(1).raw));
    if (bmp != nullptr) {
        bitmap(thread->thisObject()) = bmp;
        Emojicode::registerForDeinitialization(thread->thisObject());
        thread->returnOEValueFromFunction(thread->thisContext());
    }
    else {
        thread->returnErrorFromFunction(errnoToError());
    }
}

void fontInitFile(Thread *thread) {
    const char *path = stringToCString(thread->variable(0).object);
    ALLEGRO_FONT *font = al_load_ttf_font(path, static_cast<int>(thread->variable(1).raw), 0);
    if (font != nullptr) {
        font(thread->thisObject()) = font;
        Emojicode::registerForDeinitialization(thread->thisObject());
        thread->returnOEValueFromFunction(thread->thisContext());
    }
    else {
        thread->returnErrorFromFunction(errnoToError());
    }
}

void eventQueueInit(Thread *thread) {
    eventQueue(thread->thisObject()) = al_create_event_queue();
    Emojicode::registerForDeinitialization(thread->thisObject());
    thread->returnFromFunction(thread->thisContext());
}

void eventQueueWait(Thread *thread) {
    Object *event = newObject(CL_EVENT);
    al_wait_for_event(eventQueue(thread->thisObject()), event->val<ALLEGRO_EVENT>());
    switch (event->val<ALLEGRO_EVENT>()->type) {
        case ALLEGRO_EVENT_KEY_CHAR:
            event->klass = CL_EVENT_KEY_CHAR;
            break;
        case ALLEGRO_EVENT_KEY_DOWN:
            event->klass = CL_EVENT_KEY_DOWN;
            break;
        case ALLEGRO_EVENT_KEY_UP:
            event->klass = CL_EVENT_KEY_UP;
            break;
        case ALLEGRO_EVENT_MOUSE_AXES:
            event->klass = CL_EVENT_MOUSE_AXES;
            break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            event->klass = CL_EVENT_MOUSE_DOWN;
            break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            event->klass = CL_EVENT_MOUSE_UP;
            break;
        case ALLEGRO_EVENT_TIMER:
            event->klass = CL_EVENT_TIMER;
            break;
        default:
            break;
    }
    thread->returnFromFunction(event);
}

void eventQueueRegisterMouse(Thread *thread) {
    al_register_event_source(eventQueue(thread->thisObject()), al_get_mouse_event_source());
    thread->returnFromFunction();
}

void eventQueueRegisterKeyboard(Thread *thread) {
    al_register_event_source(eventQueue(thread->thisObject()), al_get_keyboard_event_source());
    thread->returnFromFunction();
}

void eventQueueRegisterTimer(Thread *thread) {
    al_register_event_source(eventQueue(thread->thisObject()),
                             al_get_timer_event_source(timer(thread->variable(0).object)));
    thread->returnFromFunction();
}

void keyboardEventKeycode(Thread *thread) {
    EmojicodeInteger integer = thread->thisObject()->val<ALLEGRO_EVENT>()->keyboard.keycode;
    thread->returnFromFunction(integer);
}

void keyboardEventSymbol(Thread *thread) {
    EmojicodeInteger symbol = thread->thisObject()->val<ALLEGRO_EVENT>()->keyboard.unichar;
    if (symbol < 1) {
        thread->returnNothingnessFromFunction();
    }
    else {
        thread->returnOEValueFromFunction(symbol);
    }
}

void keyboardEventRepeated(Thread *thread) {
    thread->returnFromFunction(thread->thisObject()->val<ALLEGRO_EVENT>()->keyboard.repeat);
}

void mouseAxesEventX(Thread *thread) {
    double x = thread->thisObject()->val<ALLEGRO_EVENT>()->mouse.x;
    thread->returnFromFunction(x);
}

void mouseAxesEventY(Thread *thread) {
    double y = thread->thisObject()->val<ALLEGRO_EVENT>()->mouse.y;
    thread->returnFromFunction(y);
}

void sampleInitFile(Thread *thread) {
    const char *path = stringToCString(thread->variable(0).object);
    ALLEGRO_SAMPLE *sample = al_load_sample(path);
    if (sample != nullptr) {
        sample(thread->thisObject()) = sample;
        Emojicode::registerForDeinitialization(thread->thisObject());
        thread->returnOEValueFromFunction(thread->thisContext());
    }
    else {
        thread->returnErrorFromFunction(errnoToError());
    }
}

void samplePlay(Thread *thread) {
    ALLEGRO_SAMPLE *sample = sample(thread->thisObject());
    float gain = static_cast<float>(thread->variable(0).doubl);
    float pan = static_cast<float>(thread->variable(1).doubl);
    float speed = static_cast<float>(thread->variable(2).doubl);
    al_reserve_samples(1);
    al_play_sample(sample, gain, pan, speed, ALLEGRO_PLAYMODE_ONCE, nullptr);
    thread->returnFromFunction();
}

void timerInit(Thread *thread) {
    timer(thread->thisObject()) = al_create_timer(thread->variable(0).doubl);
    thread->returnFromFunction(thread->thisContext());
}

void timerStart(Thread *thread) {
    al_start_timer(timer(thread->thisObject()));
    thread->returnFromFunction();
}

void timerStop(Thread *thread) {
    al_stop_timer(timer(thread->thisObject()));
    thread->returnFromFunction();
}

void timerResume(Thread *thread) {
    al_resume_timer(timer(thread->thisObject()));
    thread->returnFromFunction();
}

LinkingTable {
    nullptr,
    appInit, //🙋
    appFlip, //🎦
    appSetTargetBitmap, //🏓
    appSetTargetBackbuffer, //🏏
    appClear, //🚿
    appDrawLine, //🖍
    appDrawTriangle, //📐
    appDrawFilledTriangle, //🚩
    appDrawRectangle, //📋
    appDrawFilledRectangle, //🗄
    appDrawCircle, //⚽
    appDrawFilledCircle, //🏀
    appDrawRoundedRectangle, //🏉
    appDrawFilledRoundedRectangle, //🏈
    appDrawBitmap, //📼
    appDrawScaledBitmap, //💽
    appDrawText, //🔡
    bitmapInitFile, //📄
    bitmapInitSize, //🆕
    fontInitFile, //🕉
    timerInit,
    timerStart,
    timerResume,
    timerStop,
    eventQueueInit,
    eventQueueWait,
    eventQueueRegisterMouse,
    eventQueueRegisterKeyboard,
    eventQueueRegisterTimer,
    sampleInitFile,
    samplePlay,
    displayInitWithDimensions,
    displaySetTitle,
    keyboardEventKeycode,
    keyboardEventSymbol,
    keyboardEventRepeated,
    mouseAxesEventX,
    mouseAxesEventY,
};

extern "C" void prepareClass(Class *klass, EmojicodeChar name) {
    switch (name) {
        case 0x1f3d4:
            CL_APPLICATION = klass;
            break;
        case 0x1f4fa: //📺
            klass->valueSize = sizeof(ALLEGRO_DISPLAY*);
            klass->deinit = [](Object *object) {
                al_destroy_display(*object->val<ALLEGRO_DISPLAY *>());
            };
            break;
        case 0x1f5bc: //🖼
            CL_BITMAP = klass;
            klass->valueSize = sizeof(ALLEGRO_BITMAP*);
            klass->deinit = [](Object *object) {
                al_destroy_bitmap(*object->val<ALLEGRO_BITMAP *>());
            };
            break;
        case 0x1f549: //🕉
            klass->valueSize = sizeof(ALLEGRO_FONT*);
            klass->deinit = [](Object *object) {
                al_destroy_font(*object->val<ALLEGRO_FONT *>());
            };
            break;
        case 0x1f5c3: //🗃
            klass->valueSize = sizeof(ALLEGRO_EVENT_QUEUE*);
            klass->deinit = [](Object *object) {
                al_destroy_event_queue(*object->val<ALLEGRO_EVENT_QUEUE *>());
            };
            break;
        case 0x1f389: //🎉
            CL_EVENT = klass;
            klass->valueSize = sizeof(ALLEGRO_EVENT);
            break;
        case 0x1f4e9: //📩
            CL_EVENT_KEY_CHAR = klass;
            break;
        case 0x1f4e4: //📤
            CL_EVENT_KEY_UP = klass;
            break;
        case 0x1f4e5: //📥
            CL_EVENT_KEY_DOWN = klass;
            break;
        case 0x2747: //❇
            CL_EVENT_MOUSE_AXES = klass;
            break;
        case 0x1f51b: //🔛
            CL_EVENT_MOUSE_DOWN = klass;
            break;
        case 0x1f51d: //🔝
            CL_EVENT_MOUSE_UP = klass;
            break;
        case 0x1f6ce: //🛎
            CL_EVENT_TIMER = klass;
            break;
        case 0x1f3b6: //🎶
            klass->valueSize = sizeof(ALLEGRO_SAMPLE*);
            klass->deinit = [](Object *object) {
                al_destroy_sample(*object->val<ALLEGRO_SAMPLE *>());
            };
            break;
    }
}
