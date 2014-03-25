#pragma once

#include <opencv2/opencv.hpp>
#include <cc++/thread.h>
extern "C" {
#	include <libswscale/swscale.h>
}
#include "../libzqpkt_img_fifo/simple_student_trace/KVConfig.h"
#include "../../libptz/ptz.h"
#include "define.h"

/** ��������opencv��ʾ�Ĳ��ֶ�Ӧ����һ�������Ĺ����߳������ */
class WindowThread : ost::Thread
{
	bool quit_;

	bool teacher_enabled_, student_enabled_;

	struct tdt_t *tdt_;
	typedef std::deque<cv::Mat> IMAGES;
	IMAGES teacher_detect_images_, teacher_trace_images_;
	IMAGES student_detect_images_, student_trace_images_;
	ost::Mutex cs_teacher_detect_, cs_teacher_trace_;
	ost::Mutex cs_student_detect_, cs_student_trace_;

public:
	WindowThread(struct tdt_t *tdt, bool te, bool se) : tdt_(tdt)
	{
		teacher_enabled_ = te;
		student_enabled_ = se;

		quit_ = false;
		start();
	}

	~WindowThread()
	{
		quit_ = true;
		join();
	}

	// ���浽 fifo �б��� run() ����ʾ
	void showImage(bool teacher, bool detect, const IplImage *img)
	{
		cv::Mat mat(img);
		cv::Mat copy = mat.clone();

		if (teacher) {
			if (detect) {
				ost::MutexLock al(cs_teacher_detect_);
				teacher_detect_images_.push_back(copy);
			}
			else {
				ost::MutexLock al(cs_teacher_trace_);
				teacher_trace_images_.push_back(copy);
			}
		}
		else {
			if (detect) {
				ost::MutexLock al(cs_student_detect_);
				student_detect_images_.push_back(copy);
			}
			else {
				ost::MutexLock al(cs_student_trace_);
				student_trace_images_.push_back(copy);
			}
		}
	}

private:
	void run()
	{
		// ������ʾ����
		if (teacher_enabled_) {
			cv::namedWindow(TEACHER_DETWINNAME);
			cv::namedWindow(TEACHER_TACWINNAME);
			cvMoveWindow(TEACHER_DETWINNAME, 0, 0);
			cvMoveWindow(TEACHER_TACWINNAME, 0, 540);
			cv::setMouseCallback(TEACHER_DETWINNAME, mouse_cb_teacher_detect, tdt_);
			cv::setMouseCallback(TEACHER_TACWINNAME, mouse_cb_teacher_trace, tdt_);
		}

		if (student_enabled_) {
			cv::namedWindow(STUDENT_DETWINNAME);
			cv::namedWindow(STUDENT_TACWINNAME);
			cvMoveWindow(STUDENT_DETWINNAME, 960, 0);
			cvMoveWindow(STUDENT_TACWINNAME, 960, 540);
			cv::setMouseCallback(STUDENT_DETWINNAME, mouse_cb_student_detect, tdt_);
			cv::setMouseCallback(STUDENT_TACWINNAME, mouse_cb_student_trace, tdt_);

		}

		cv::namedWindow("movie output");	// 
		cvMoveWindow("movie output", 960, 270);

		// ��ѭ��
		while (!quit_) {
			int key = cvWaitKey(1);
			if (key != -1) {
				handle_key(tdt_, key);
			}

			if (teacher_enabled_) {
				showimage(cs_teacher_detect_, teacher_detect_images_, TEACHER_DETWINNAME);
				showimage(cs_teacher_trace_, teacher_trace_images_, TEACHER_TACWINNAME);
			}
			if (student_enabled_) {
				showimage(cs_student_detect_, student_detect_images_, STUDENT_DETWINNAME);
				showimage(cs_student_trace_, student_trace_images_, STUDENT_TACWINNAME);
			}
		}
	}

	void showimage(ost::Mutex &cs, IMAGES &imgs, const char *winname)
	{
		ost::MutexLock al(cs);
		while (!imgs.empty()) {
			cv::Mat img = imgs.front();
			cv::imshow(winname, img);
			imgs.pop_front();
		}
	}

	void handle_key(struct tdt_t *tdt, int key);

	static void mouse_cb_teacher_detect(int ev, int x, int y, int flags, void *opaque);
	static void mouse_cb_student_detect(int ev, int x, int y, int flags, void *opaque);
	static void mouse_cb_teacher_trace(int ev, int x, int y, int flags, void *opaque);
	static void mouse_cb_student_trace(int ev, int x, int y, int flags, void *opaque);
};

// ���� TeacherRunning, StudentRunning �Ļ��࣬����ͼ������֮��� ...
class Running
{
	WindowThread *win_thread_;	// 

	SwsContext *sws_[2];	// ����ת�����ʺ� IplImage
	int last_width_[2], last_height_[2], last_fmt_[2];
	AVPicture pic_[2];		// ���ڴ洢ת�����ͼ��...
	int distortion_;	// �Ƿ���л���У����.
	int mask_;			// �Ƿ���� mask img

protected:
	KVConfig *cfg_;

	int debug_;			// �Ƿ����ִ�У�.

	ost::Mutex cs_mask_;	// 
	IplImage *img_mask_;	// ����Ȥ����
	bool enable_mask_;		// ��ʱ

	int width_, height_;	// ͼ���С
	int width_tracing_, height_tracing_;

	const char *detect_win_name_, *trace_win_name_;

public:
	Running(WindowThread *winthread, KVConfig *cfg, const char *detect_win_name = 0, const char *trace_win_name = 0);
	virtual ~Running(void);

	/** ̽���������ڣ��ɵ����̣߳�����״̬������ */
	int write_img(int width, int height, int fmt, unsigned char *data[4], int stride[4], double stamp);

	/** ���ٵ�ͼ�� */
	int write_img_tracing(int width, int height, int fmt, unsigned char *data[4], int stride[4], double stamp);

	/** ���� */
	int key_pressed(int key);

	/** ����¼� */
	void mouse_of_detecting(int ev, int x, int y, int flags);
	void mouse_of_tracing(int ev, int x, int y, int flags);

protected:
	// ׼���õ� IplImage
	virtual void one_image(IplImage *raw_img, IplImage *masked_img, double stamp) = 0;

	// ��������Ҫָ���Լ������
	virtual bool is_teacher() const = 0;

	// ��Ҫ���ذ����¼�
	virtual int key_handle(int key) = 0;

	// ����¼�
	virtual void mouse_detected(int ev, int x, int y, int flags) {}
	virtual void mouse_traced(int ev, int x, int y, int flags) {}

	// ��ʾ trace image ֮ǰ�����ã��������У�����������ʾ��Ϣ
	virtual void before_show_tracing_image(IplImage *img) {}

protected:
	void show_image(bool teacher, bool detect, const IplImage *img)
	{
		if (win_thread_)
			win_thread_->showImage(teacher, detect, img);
	}

	bool build_mask_internal(const char *key, IplImage *img)
	{
		bool masked = false;

		const char *pts = cfg_->get_value(key, 0);
		std::vector<CvPoint> points;

		if (pts) {
			char *data = strdup(pts);
			char *p = strtok(data, ";");
			while (p) {
				// ÿ��Point ʹ"x,y" ��ʽ
				int x, y;
				if (sscanf(p, "%d,%d", &x, &y) == 2) {
					CvPoint pt = { x, y };
					points.push_back(pt);
				}

				p = strtok(0, ";");
			}
			free(data);
		}

		if (points.size() > 3) {
			int n = points.size();
			CvPoint **pts = (CvPoint**)alloca(sizeof(CvPoint*) * points.size());
			for (int i = 0; i < n; i++) {
				pts[i] = &points[i];
			}

			if (!masked_)
				cvZero(img);
			cvFillPoly(img, pts, &n, 1, CV_RGB(255, 255, 255));

			masked = true;
		}

		return masked;
	}

	bool masked_;

	IplImage *build_mask(const char *key, const char *key2 = 0)
	{
		/** �� cfg �еĲ��������� mask */
		CvSize size = { width_, height_ };
		IplImage *img = cvCreateImage(size, 8, 3);	// FIXME: Ӧ�ø���ʵ�� ...

		if (masked_)
			cvZero(img);
		else
			cvSet(img, CV_RGB(255, 255, 255));

		if (key) {
			masked_ = build_mask_internal(key, img);
		}

		if (key2) {
			build_mask_internal(key2, img);
		}

		return img;
	}

	void enable_mask(bool enable)
	{
		ost::MutexLock al(cs_mask_);
		enable_mask_ = enable;
	}

private:
	int chk_scale(bool tracing, int width, int height, int fmt)
	{
		int index = 0;
		if (tracing) index = 1;

		if (width != last_width_[index] || height != last_height_[index] || fmt != last_fmt_[index]) {
			if (sws_[index]) sws_freeContext(sws_[index]);
			sws_[index] = sws_getContext(width, height, (PixelFormat)fmt, tracing ? width_tracing_ : width_, tracing ? height_tracing_ : height_,
				PIXFMT, SWS_FAST_BILINEAR, 0, 0, 0);
			last_width_[index] = width, last_height_[index] = height, last_fmt_[index] = fmt;

			if (!sws_[index]) {
				fprintf(stderr, "ERR: sws_getContext err, !!!????\n");
				::exit(-1);
			}

			return 1;
		}
		else
			return 0;
	}

	int prepare_img(bool tracing, int width, int height, int fmt, unsigned char *data[4], int stride[4])
	{
		chk_scale(tracing, width, height, fmt);

		int index = tracing ? 1 : 0;
		return sws_scale(sws_[index], data, stride, 0, height, pic_[index].data, pic_[index].linesize);
	}

	IplImage *do_mask(const IplImage *img)
	{
		ost::MutexLock al(cs_mask_);
		if (enable_mask_ && img_mask_) {
			IplImage *masked = cvCreateImage(cv::Size(width_, height_), 8, 3);
			cvAnd(img, img_mask_, masked);
			return masked;
		}
		else
			return 0;
	}
};
