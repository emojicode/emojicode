LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := emojicodec
FILE_LIST := $(wildcard $(LOCAL_PATH)/EmojicodeCompiler/*.cpp)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/EmojicodeReal-TimeEngine/ $(LOCAL_PATH)/
LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_CPPFLAGS := -std=c++14 -Wall -Ofast -fpie -fexceptions -frtti -D__STDINT_LIMITS
LOCAL_LDLIBS := -pie -lm -ldl -rdynamic -latomic
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := emojicode
FILE_LIST := $(wildcard $(LOCAL_PATH)/EmojicodeReal-TimeEngine/*.c)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/
LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_CPPFLAGS := -std=c++14 -Wall -Ofast -fpie -fexceptions -frtti -D__STDINT_LIMITS
LOCAL_LDLIBS := -pie -lm -ldl -rdynamic -latomic
include $(BUILD_EXECUTABLE)
