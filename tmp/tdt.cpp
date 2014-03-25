#include <stdio.h>
#include <stdlib.h>
#include "tdt.h"
#include "TeacherRunning.h"
#include "StudentRunning.h"
#include <opencv2/opencv.hpp>
#include <deque>
#include "log.h"

struct tdt_t
{
	int enabled_;

	KVConfig *cfg_t_, *cfg_s_;

	TeacherRunning *running_t_;	// 执行体
	StudentRunning *running_s_; // 

	WindowThread *key_thread_;	// 用于执行 opencv 的窗口消息
};

tdt_t *tdt_open(const char *cfgname, pfn_tdt_notify notify, void *opaque)
{
	mylog_init();
	mylog("%s: en ........ start ...............\n", __FUNCTION__);

	tdt_t *ctx = new tdt_t;
	ctx->enabled_ = 0;		// 缺省不启用

	ctx->cfg_t_ = new KVConfig(cfgname);
	const char *sname = ctx->cfg_t_->get_value("student_detect_cfg_filename", "student_detect_trace.config");
	ctx->cfg_s_ = new KVConfig(sname);

	ctx->running_s_ = 0;
	ctx->running_t_ = 0;

	if (atoi(ctx->cfg_t_->get_value("debug", "0")) == 1) {
		bool te = true, se = true;
		if (atoi(ctx->cfg_t_->get_value("enable_teacher", "1")) == 0)
			te = false;
		if (atoi(ctx->cfg_t_->get_value("enable_student", "1")) == 0)
			se = false;

		ctx->key_thread_ = new WindowThread(ctx, te, se);
	}
	else
		ctx->key_thread_ = 0;

	ctx->running_t_ = new TeacherRunning(ctx->key_thread_, ctx->cfg_t_, notify, opaque, ctx->enabled_);
	ctx->running_s_ = new StudentRunning(ctx->key_thread_, ctx->cfg_s_);

	return ctx;
}

void tdt_close(tdt_t *ctx)
{
	delete ctx->key_thread_;
	delete ctx->running_t_;
	delete ctx->running_s_;
	ctx->cfg_t_->save_as(0);
	ctx->cfg_s_->save_as(0);
	delete ctx->cfg_t_;
	delete ctx->cfg_s_;
	delete ctx;

	mylog("%s: all end !!!\n", __FUNCTION__);
}

int tdt_is_enable(tdt_t *ctx)
{
	return ctx->enabled_;
}

void tdt_enable(tdt_t *ctx, int en)
{
	mylog("%s: en=%d\n", __FUNCTION__, en);
	ctx->enabled_ = en;
}

int tdt_write_teacher_pic(tdt_t *ctx, int width, int height, int fmt, unsigned char *data[4], int stride[4], double stamp)
{
	if (ctx->enabled_)
		return ctx->running_t_->write_img(width, height, fmt, data, stride, stamp);
	else
		return 1;
}

int tdt_write_teacher_pic_tracing(tdt_t *ctx, int width, int height, int fmt, unsigned char *data[4], int stride[4], double stamp)
{
	if (ctx->enabled_)
		return ctx->running_t_->write_img_tracing(width, height, fmt, data, stride, stamp);
	else
		return 1;
}

int tdt_write_student_pic(tdt_t *ctx, int width, int height, int fmt, unsigned char *data[4], int stride[4], double stamp)
{
	if (ctx->enabled_)
		return ctx->running_s_->write_img(width, height, fmt, data, stride, stamp);
	else
		return 1;
}

int tdt_write_student_pic_tracing(tdt_t *ctx, int width, int height, int fmt, unsigned char *data[4], int stride[4], double stamp)
{
	if (ctx->enabled_)
		return ctx->running_s_->write_img_tracing(width, height, fmt, data, stride, stamp);
	else
		return 1;
}

/** 全局处理按键消息，直接传递给 Running
 */
void WindowThread::handle_key(struct tdt_t *tdt, int key)
{
	//mylog("%s: key=%d\n", __FUNCTION__, key);
	tdt->running_t_->key_pressed(key);
	tdt->running_s_->key_pressed(key);
}

void WindowThread::mouse_cb_teacher_detect(int ev, int x, int y, int flags, void *opaque)
{
	if (((tdt_t*)opaque)->running_t_)
		((tdt_t*)opaque)->running_t_->mouse_of_detecting(ev, x, y, flags);
}

void WindowThread::mouse_cb_teacher_trace(int ev, int x, int y, int flags, void *opaque)
{
	if (((tdt_t*)opaque)->running_t_)
		((tdt_t*)opaque)->running_t_->mouse_of_tracing(ev, x, y, flags);
}

void WindowThread::mouse_cb_student_detect(int ev, int x, int y, int flags, void *opaque)
{
	if (((tdt_t*)opaque)->running_s_)
		((tdt_t*)opaque)->running_s_->mouse_of_detecting(ev, x, y, flags);
}

void WindowThread::mouse_cb_student_trace(int ev, int x, int y, int flags, void *opaque)
{
	if (((tdt_t*)opaque)->running_s_)
		((tdt_t*)opaque)->running_s_->mouse_of_tracing(ev, x, y, flags);
}
