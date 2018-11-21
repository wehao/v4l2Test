#include "ImageProc.h"

int openDevice(char* dev) {
	struct stat st;
	int length = sizeof(dev)/sizeof(char);
	if (length == 0) {
		ALOGE("error param %s", dev);
	}
	if (-1 == stat(dev, &st)) {
		ALOGE("open device %s failed, dev is not exist", dev);
	}
	fd = open(dev, O_RDWR| O_NONBLOCK, 0);
	if (fd == -1) {
		ALOGE("open device %s failed", dev);
	}
	return 0;
}

int getCapability(int fd) {
	struct v4l2_capability capability;
	int ret = 0;
	ret = xioctl(fd, VIDIOC_QUERYCAP, &capability);
	if (ret == -1) {
		ALOGE("not support querycap");
	} else {
		ALOGE("V4L2_CAP_VIDEO_CAPTURE %d", capability.capabilities&V4L2_CAP_VIDEO_CAPTURE);
		//ALOGE("V4L2_CAP_TUNER %d", capability.capabilities&V4L2_CAP_TUNER);
		//ALOGE("V4L2_CAP_AUDIO %d", capability.capabilities&V4L2_CAP_AUDIO);
		ALOGE("V4L2_CAP_STREAMING %d", capability.capabilities&V4L2_CAP_STREAMING);
	}
	return ret;
}

int setCropCap(int fd) {
	int ret = 0;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	CLEAR(cropcap);
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = xioctl(fd, VIDIOC_CROPCAP, &cropcap);
	if (ret == -1) {
		ALOGE("VIDIOC_CROPCAP error %d, %s", error, strerror(error));
	} else if (ret == 0) {
		ALOGE("cropcap type %d", cropcap.type);
		ALOGE("cropcap bounds rect left %d", cropcap.bounds.left);
		ALOGE("cropcap bounds rect top %d", cropcap.bounds.top);
		ALOGE("cropcap bounds rect width %d", cropcap.bounds.width);
		ALOGE("cropcap bounds rect height %d", cropcap.bounds.height);

		ALOGE("cropcap defrect rect left %d", cropcap.defrect.left);
		ALOGE("cropcap defrect rect top %d", cropcap.defrect.top);
		ALOGE("cropcap defrect rect width %d", cropcap.defrect.width);
		ALOGE("cropcap defrect rect height %d", cropcap.defrect.height);

		IMG_WIDTH = cropcap.defrect.width;
		IMG_HEIGHT = cropcap.defrect.height;

		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect;
		ret = xioctl(fd, VIDIOC_S_CROP, &crop);
		if (ret == -1)
		{
			ALOGE("setCropCap  failed %d, %s", error, strerror(error));
		}
		else {
			ALOGE("setCropCap  success");
		}
	} else {
		ALOGE("cropcap else");
	}

	return ret;
}

int setVideoFmt(int fd) {
	struct v4l2_format fmt;
	int min;
	int ret;

	fmt.type = V4L2_CAP_VIDEO_CAPTURE;
	fmt.fmt.pix.width = IMG_WIDTH;
	fmt.fmt.pix.height = IMG_HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	ret = xioctl(fd, VIDIOC_S_FMT, &fmt);
	if (ret == -1)
	{
		ALOGE("setVideoFmt failed");
		return ret;
	} else{
		ALOGE("setVideoFmt success");
	}

	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
	{
		fmt.fmt.pix.bytesperline = min;
	}
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
	{
		fmt.fmt.pix.sizeimage = min;
	}

	return ret;
}

int queryBuffers(int fd) {
	struct v4l2_requestbuffers req;
	CLEAR(req);
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
	{
		ALOGE("VIDIOC_REQBUFS failed %d, %s", error, strerror(error));
		return -1;
	}
	if (req.count < 2)
	{
		ALOGE("VIDIOC_REQBUFS count is  %d", req.count);
		return -1;
	}

	buffers = NULL;
	buffers = calloc(req.count, sizeof(*buffers));
	for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
	{
		struct v4l2_buffer buf;
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;
		if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)) {
			ALOGE("VIDIOC_REQBUF failed");
			return -1;
		}
		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
		if (MAP_FAILED == buffers[n_buffers].start) {
			ALOGE("mmap failed ");
			return -1;
		}
	}
	return 0;
}

int startStream() {
	unsigned int i;
	enum v4l2_buf_type type;
	for (i = 0; i < n_buffers; ++i)
	{
		struct v4l2_buffer buf;
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
			ALOGE("V4L2_QBUF failed");
			return -1;
		}
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)) {
		ALOGE("VIDIOC_STREAMON failed %d, %s", error, strerror(error));
		return -1;
	}
	return 0;
} 

int stopStream() {
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
		return -1;

	return 0;
} 

int closeDevice() {
	if (-1 == close (fd)){
		fd = -1;
		ALOGE("close fialed");
	}

	fd = -1;
	return 0;
}

void uninitdevice() {
	unsigned int i;

	for (i = 0; i < n_buffers; ++i)
		if (-1 == munmap (buffers[i].start, buffers[i].length))
			return -1;

	free (buffers);

	return 0;
}

int readframeonce(void) {
	for (;;) {
		fd_set fds;
		struct timeval tv;
		int r;

		FD_ZERO (&fds);
		FD_SET (fd, &fds);

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		r = select (fd + 1, &fds, NULL, NULL, &tv);

		if (-1 == r) {
			if (EINTR == errno)
				continue;

			ALOGE("select failed");
			return -1;
		}

		if (0 == r) {
			ALOGE("select timeout");
			return -1;
		}

		if (readframe ()==1);
		break;
	}

	return 0;
}

int readframe(void) {
	struct v4l2_buffer buf;
	CLEAR(buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
	{
		ALOGE("VIDIOC_DQBUF failed");
		return -1;
	}
	assert(buf.index < n_buffers);
	processimage(buffers[buf.index].start);
	if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
	{
		ALOGE("VIDIOC_QBUF after VIDIOC_DQBUF failed");
		return -1;
	}
	ALOGE("readframe success");
	return 1;
}

void processimage(void* start) {
	yuyv422toABGRY((unsigned char*)start);
}

int errorexit() {
	return 0;
}

int xioctl(int fd, int request, void* arg) {
	int ret;
	ret = ioctl(fd, request, arg);
	return ret;
}

void yuyv422toABGRY(unsigned char *src)
{

	int width=0;
	int height=0;

	width = IMG_WIDTH;
	height = IMG_HEIGHT;

	int frameSize =width*height*2;

	int i;

	if((!rgb || !ybuf)){
		return;
	}
	int *lrgb = NULL;
	int *lybuf = NULL;
		
	lrgb = &rgb[0];
	lybuf = &ybuf[0];

	if(yuv_tbl_ready==0){
		for(i=0 ; i<256 ; i++){
			y1192_tbl[i] = 1192*(i-16);
			if(y1192_tbl[i]<0){
				y1192_tbl[i]=0;
			}

			v1634_tbl[i] = 1634*(i-128);
			v833_tbl[i] = 833*(i-128);
			u400_tbl[i] = 400*(i-128);
			u2066_tbl[i] = 2066*(i-128);
		}
		yuv_tbl_ready=1;
	}

	for(i=0 ; i<frameSize ; i+=4){
		unsigned char y1, y2, u, v;
		y1 = src[i];
		u = src[i+1];
		y2 = src[i+2];
		v = src[i+3];

		int y1192_1=y1192_tbl[y1];
		int r1 = (y1192_1 + v1634_tbl[v])>>10;
		int g1 = (y1192_1 - v833_tbl[v] - u400_tbl[u])>>10;
		int b1 = (y1192_1 + u2066_tbl[u])>>10;

		int y1192_2=y1192_tbl[y2];
		int r2 = (y1192_2 + v1634_tbl[v])>>10;
		int g2 = (y1192_2 - v833_tbl[v] - u400_tbl[u])>>10;
		int b2 = (y1192_2 + u2066_tbl[u])>>10;

		r1 = r1>255 ? 255 : r1<0 ? 0 : r1;
		g1 = g1>255 ? 255 : g1<0 ? 0 : g1;
		b1 = b1>255 ? 255 : b1<0 ? 0 : b1;
		r2 = r2>255 ? 255 : r2<0 ? 0 : r2;
		g2 = g2>255 ? 255 : g2<0 ? 0 : g2;
		b2 = b2>255 ? 255 : b2<0 ? 0 : b2;

		*lrgb++ = 0xff000000 | b1<<16 | g1<<8 | r1;
		*lrgb++ = 0xff000000 | b2<<16 | g2<<8 | r2;

		if(lybuf!=NULL){
			*lybuf++ = y1;
			*lybuf++ = y2;
		}
	}

}

int getBrightness() {
	ctrl.id=BRIGHTNESS_ID;
    	if(ioctl(fd,VIDIOC_G_CTRL,&ctrl)==-1)
    	{
        ALOGE("getBrightness failed");
        return -1;
    	} else{
    		ALOGE("getBrightness success brightness is %d", ctrl.value);
    	}
    	return ctrl.value;
}

int setBrightness(int value) {
	ctrl.id = BRIGHTNESS_ID;
	ctrl.value = value;
	if(ioctl(fd,VIDIOC_S_CTRL,&ctrl)==-1)
    	{
        ALOGE("setBrightness failed");
        return -1;
    	} else{
    		ALOGE("setBrightness success brightness is %d", value);
    	}
	return 1;
}

int getContrast() {
	ctrl.id=CONTRAST_ID;
    	if(ioctl(fd,VIDIOC_G_CTRL,&ctrl)==-1)
    	{
        ALOGE("getContrast failed");
        return -1;
    	} else{
    		ALOGE("getContrast success contrast is %d", ctrl.value);
    	}
    	return ctrl.value;
}

int setContrast(int contrast) {
	ctrl.id = CONTRAST_ID;
	ctrl.value = contrast;
	if(ioctl(fd,VIDIOC_S_CTRL,&ctrl)==-1)
    	{
        ALOGE("setContrast failed");
        return -1;
    	} else{
    		ALOGE("setContrast success contrast is %d", contrast);
    	}
	return 1;
}

void 
Java_com_camera_simplewebcam_CameraPreview_pixeltobmp( JNIEnv* env,jobject thiz,jobject bitmap){

	jboolean bo;


	AndroidBitmapInfo  info;
	void*              pixels;
	int                ret;
	int i;
	int *colors;

	int width=0;
	int height=0;

	if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
		ALOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
		return;
	}
    
	width = info.width;
	height = info.height;

	if(!rgb || !ybuf) return;

	if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
		ALOGE("Bitmap format is not RGBA_8888 !");
		return;
	}

	if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
		ALOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
	}

	colors = (int*)pixels;
	int *lrgb =NULL;
	lrgb = &rgb[0];

	for(i=0 ; i<width*height ; i++){
		*colors++ = *lrgb++;
	}

	AndroidBitmap_unlockPixels(env, bitmap);

}

jint 
Java_com_camera_simplewebcam_CameraPreview_prepareCamera( JNIEnv* env,jobject thiz, jint videoid){
	ALOGE("opendevice");
	char devName[16] = "/dev/video21";
	if (videoid != 0)
	{
		sprintf(devName,"/dev/video%d", videoid);
	} else {
		ALOGE("videoid is 0");
	}
	openDevice(devName);
	getCapability(fd);
	setCropCap(fd);
	setVideoFmt(fd);
	queryBuffers(fd);
	startStream();
	rgb = (int *)malloc(sizeof(int) * (IMG_WIDTH*IMG_HEIGHT));
	ybuf = (int *)malloc(sizeof(int) * (IMG_WIDTH*IMG_HEIGHT));
	return 0;

	// int ret;

	// if(camerabase<0){
	// 	camerabase = checkCamerabase();
	// }

	// ret = opendevice(camerabase + videoid);

	// if(ret != ERROR_LOCAL){
	// 	ret = initdevice();
	// }
	// if(ret != ERROR_LOCAL){
	// 	ret = startcapturing();

	// 	if(ret != SUCCESS_LOCAL){
	// 		stopcapturing();
	// 		uninitdevice ();
	// 		closedevice ();
	// 		LOGE("device resetted");	
	// 	}

	// }

	// if(ret != ERROR_LOCAL){
	// 	rgb = (int *)malloc(sizeof(int) * (IMG_WIDTH*IMG_HEIGHT));
	// 	ybuf = (int *)malloc(sizeof(int) * (IMG_WIDTH*IMG_HEIGHT));
	// }
	// return ret;
}	



jint 
Java_com_camera_simplewebcam_CameraPreview_prepareCameraWithBase( JNIEnv* env,jobject thiz, jint videoid, jint videobase){
		int ret;

		// camerabase = videobase;
	
		return Java_com_camera_simplewebcam_CameraPreview_prepareCamera(env,thiz,videoid);
	
}

void 
Java_com_camera_simplewebcam_CameraPreview_processCamera( JNIEnv* env,
										jobject thiz){

	readframeonce();
}

void 
Java_com_camera_simplewebcam_CameraPreview_stopCamera(JNIEnv* env,jobject thiz){

	stopStream ();

	uninitdevice ();

	closeDevice ();

	if(rgb) free(rgb);
	if(ybuf) free(ybuf);
        
	fd = -1;
}

jint Java_com_camera_simplewebcam_CameraPreview_getBrightness( JNIEnv* env,jobject thiz) {
	return getBrightness();
}

jint Java_com_camera_simplewebcam_CameraPreview_setBrightness( JNIEnv* env,jobject thiz, jint brightness) {
	return setBrightness(brightness);
}

jint Java_com_camera_simplewebcam_CameraPreview_getContrast( JNIEnv* env,jobject thiz) {
	return getContrast();
}

jint Java_com_camera_simplewebcam_CameraPreview_setContrast( JNIEnv* env,jobject thiz, jint contrast) {
	return setContrast(contrast);
}

