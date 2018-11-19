#include <stdio.h>
#include <string.h>

#define LOG_TAG 	"WebCam"
#define LOGE(...) 	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) 	__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

int openDevice(char* dev);
int getCapability(char* dev);
int setVideoFmt();
int startStream();
int stopStream();
int closeDevice(char* dev);
char* devPath;