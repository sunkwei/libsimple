#include "Running.h"
#include "../simple_teacher_trace/fullimage_recognition.h"

Running::Running(WindowThread *winth, KVConfig *cfg, const char *detname, const char *tacname)
	: cfg_(cfg)
	, win_thread_(winth)
	, detect_win_name_(detname)
	, trace_win_name_(tacname)
{
	masked_ = false;

	last_width_[0] = last_width_[1] = -1;
	sws_[0] = sws_[1] = 0;

	distortion_ = atoi(cfg_->get_value("distortion_correction", "0"));
	
	width_ = atoi(cfg_->get_value("video_width", "960"));
	height_ = atoi(cfg_->get_value("video_height", "540"));
	width_tracing_ = atoi(cfg_->get_value("video_width", "960"));
	height_tracing_ = atoi(cfg_->get_value("video_height", "540"));

	debug_ = atoi(cfg_->get_value("debug", "0"));

	enable_mask_ = true;
	mask_ = atoi(cfg_->get_value("mask", "1"));
	if (mask_) {
		if (atoi(cfg_->get_value("hog_enabled", "0")) != 0)
			img_mask_ = build_mask("calibration_data");
		else
			img_mask_ = build_mask("calibration_data2", "calibration_data3");
	}
	else {
		img_mask_ = 0;
	}

	avpicture_alloc(&pic_[0], PIXFMT, width_, height_);
	avpicture_alloc(&pic_[1], PIXFMT, width_tracing_, height_tracing_);
}

Running::~Running(void)
{
	if (img_mask_) cvReleaseImage(&img_mask_);
	if (sws_[0]) sws_freeContext(sws_[0]);
	if (sws_[1]) sws_freeContext(sws_[1]);
	avpicture_free(&pic_[0]);
	avpicture_free(&pic_[1]);
}

int Running::key_pressed(int key)
{
	return key_handle(key);
}

void Running::mouse_of_tracing(int ev, int x, int y, int flags)
{
	mouse_traced(ev, x, y, flags);
}

void Running::mouse_of_detecting(int ev, int x, int y, int flags)
{
	mouse_detected(ev, x, y, flags);
}

static IplImage *avpic2iplimg(AVPicture &pic, int w, int h)
{
	IplImage *image = (IplImage*)malloc(sizeof(IplImage));
	image->nSize = sizeof(IplImage);
	image->ID = 0;
	image->nChannels = 3;
	image->alphaChannel = 0;
	image->depth = 8;
	image->origin = 0;
	image->dataOrder = 0;
	image->width = w;
	image->height = h;

	image->roi = 0;
	image->imageId = 0;
	image->tileInfo = 0;

	image->imageSize = pic.linesize[0] * h;
	image->widthStep = pic.linesize[0];
	image->imageData = (char*)pic.data[0];

	image->imageDataOrigin = 0;

	return image;
}

int Running::write_img(int width, int height, int fmt, unsigned char *data[4], int stride[4], double stamp)
{
	prepare_img(false, width, height, fmt, data, stride);	// 此时 pic_ 数据有效

	IplImage *img = avpic2iplimg(pic_[0], width_, height_), *img_dis = 0;

	// 检查是否需要进行畸变校正

	if (distortion_) {
		img_dis = cvCreateImage(cv::Size(width_, height_), 8, 3);
		Coordinate_Conversion::distortion(img, img_dis);
		IplImage *img_masked = do_mask(img_dis);
		one_image(img, img_masked, stamp);
		if (img_masked)	cvReleaseImage(&img_masked);
		cvReleaseImage(&img_dis);
	}
	else {
		IplImage *img_masked = do_mask(img);
		one_image(img, img_masked, stamp);
		if (img_masked) cvReleaseImage(&img_masked);
	}
	free(img);

	return 0;
}

int Running::write_img_tracing(int width, int height, int fmt, unsigned char *data[4], int stride[4], double stamp)
{
	prepare_img(true, width, height, fmt, data, stride);

	IplImage *img = avpic2iplimg(pic_[1], width_tracing_, height_tracing_);
	before_show_tracing_image(img);
	show_image(is_teacher(), false, img);
	free(img);

	return 0;
}
