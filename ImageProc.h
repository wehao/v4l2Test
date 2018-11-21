#include <stdio.h>
#include <string.h>
#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <jni.h>
#include <error.h>
#include <assert.h>
#include <errno.h>
#include <android/bitmap.h>

#include <sys/mman.h>


#include <linux/videodev2.h>
#include <linux/usbdevice_fs.h>


#define LOG_TAG 	"WebCam"
// #define LOGE(...) 	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
// #define LOGI(...) 	__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define ERROR_LOCAL -1
#define SUCCESS_LOCAL 0

static int IMG_WIDTH = 640;
static int  IMG_HEIGHT = 480;

#define BRIGHTNESS_ID 0x00980900
#define CONTRAST_ID 0x00980901
#define SATURATION_ID 0x00980902
#define HUE_ID 0x00980903
#define WHITE_BALANCE_TEMP_AUTO_ID 0x0098090c
#define GAMMA_ID 0x00980910
#define POWER_LINE_FREQUENCY_ID 0x00980918
#define WHITE_BALANCE_TEMP_ID 0x0098091a
#define SHARPNESS_ID 0x0098091b
#define BACKLIGHT_COMPENSATION_ID 0x0098091c
#define EXPOSURE_AUTO_ID 0x009a0901
#define EXPOSURE_ABSOLUTE_ID 0x009a0902
#define EXPOSURE_AUTO_PRIORITY_ID 0x009a0903

int openDevice(char* dev);
int getCapability(int fd);
int setCropCap(int fd);
int setVideoFmt(int fd);
int queryBuffers(int fd);
int startStream();
int readframe(void);
int stopStream();
int closeDevice();
void uninitdevice();
int getBrightness();

void yuyv422toABGRY(unsigned char *src);

static fd = -1;
char* devPath;
struct buffer
{
	void* start;
	size_t length;
};
struct buffer* buffers = NULL;
static size_t n_buffers = 0;
struct v4l2_control ctrl;

int *rgb = NULL;
int *ybuf = NULL;

int yuv_tbl_ready=0;
int y1192_tbl[256];
int v1634_tbl[256];
int v833_tbl[256];
int u400_tbl[256];
int u2066_tbl[256];

jint Java_com_camera_simplewebcam_CameraPreview_prepareCamera( JNIEnv* env,jobject thiz, jint videoid);
jint Java_com_camera_simplewebcam_CameraPreview_prepareCameraWithBase( JNIEnv* env,jobject thiz, jint videoid, jint videobase);
void Java_com_camera_simplewebcam_CameraPreview_processCamera( JNIEnv* env,jobject thiz);
void Java_com_camera_simplewebcam_CameraPreview_stopCamera(JNIEnv* env,jobject thiz);
void Java_com_camera_simplewebcam_CameraPreview_pixeltobmp( JNIEnv* env,jobject thiz,jobject bitmap);
jint Java_com_camera_simplewebcam_CameraPreview_getBrightness( JNIEnv* env,jobject thiz);
jint Java_com_camera_simplewebcam_CameraPreview_setBrightness( JNIEnv* env,jobject thiz, jint brightness);
jint Java_com_camera_simplewebcam_CameraPreview_getContrast( JNIEnv* env,jobject thiz);
jint Java_com_camera_simplewebcam_CameraPreview_setContrast( JNIEnv* env,jobject thiz, jint contrast);