#include "TeacherRunning.h"
#include "../simple_teacher_trace/fullimage_recognition.h"
#include <assert.h>

TeacherRunning::TeacherRunning(WindowThread *th, KVConfig *cfg, pfn_tdt_notify notify, void *opaque, int &en)
	: Running(th, cfg, TEACHER_DETWINNAME, TEACHER_TACWINNAME)
	, notify_(notify)
	, opaque_(opaque)
	, enabled_(en)
{
	state_ = RUNNING;
	switcher_ = FULLVIEW;

	//reset_want_ = false;
	//exec_test_want_ = false;

	roi_ = load_roi(cfg_);

	det_ = new TeacherDetecting(cfg_);
	ptz_ = new TeacherPtz(cfg_);
}

TeacherRunning::~TeacherRunning(void)
{
	delete ptz_;
	delete det_;
}

std::vector<cv::Point> TeacherRunning::load_roi(KVConfig *cfg)
{
	std::vector<cv::Point> points;
	char key[64];

	for (int i = 2; i <= 3; i++) {
		snprintf(key, sizeof(key), "calibration_data%d", i);
		const char *pts = cfg_->get_value(key, 0);

		if (pts) {
			char *data = strdup(pts);
			char *p = strtok(data, ";");
			while (p) {
				// 每个Point 使"x,y" 格式
				int x, y;
				if (sscanf(p, "%d,%d", &x, &y) == 2) {
					CvPoint pt = { x, y };
					points.push_back(pt);
				}

				p = strtok(0, ";");
			}
			free(data);
		}
	}

	return points;
}

void TeacherRunning::key_running_handle(int key)
{
	switch (key) {
	case 'c':
	case 'b':
	case 'd':
		{
			if (key == 'c')
				state_ = CALIBRATING;
			else if (key == 'b')
				state_ = CALIBRATING2;
			else if (key == 'd')
				state_ = CALIBRATING3;

			ost::MutexLock al(cs_calibrations_);
			calibrations_.clear();
			enable_mask(false);	// 临时禁用 ...
		}
		break;

	case 'p':
		if (ptz_->ptz()) {
			ptz_stop(ptz_->ptz());
			ptz_zoom_stop(ptz_->ptz());
		}
		ptz_->set_running_state(1);
		state_ = PTZINITING;
		ptz_->reset_ptz();
		break;

	case 'l':
	case 'r':
		ptz_->set_running_state(1);
		if (ptz_->ptz()) {
			ptz_stop(ptz_->ptz());
			ptz_zoom_stop(ptz_->ptz());
		}

		ptz_->set_running_state(1);
		if (key == 'l') {
			state_ = PTZINIT_LEFT;
			ptz_->turn_left_edge();
		}
		else {
			state_ = PTZINIT_RIGHT;
			ptz_->turn_right_edge();
		}
		
		break;
	}
}

void TeacherRunning::key_ptzinit_left_right_handle(int key)
{
	SHORT ks = GetKeyState(VK_SHIFT);

	int move_timeout = 200;
	if (ks != 0) {
		move_timeout = 100;
	}

	switch (key) {
	case 27:	// esc
		state_ = RUNNING;
		ptz_->set_running_state(0);
		break;

	case 13:	// 回车确认
		{
			if (ptz_->ptz()) {
				int h, v;
				ptz_get_current_position(ptz_->ptz(), &h, &v);
				if (state_ == PTZINIT_LEFT)
					cfg_->set_value("ptz_left", h);
				else
					cfg_->set_value("ptz_right", h);

				cfg_->save_as(0);
			}

			ptz_->set_running_state(0);
			state_ = RUNNING;
		}
		break;

	case 0x250000:	// Left
		if (ptz_->ptz()) ptz_->add(task_ptz_move(ptz_->ptz(), 1, 1, move_timeout));
		break;

	case 0x270000:	// Right
		if (ptz_->ptz()) ptz_->add(task_ptz_move(ptz_->ptz(), 2, 1, move_timeout));
		break;
	}
}

void TeacherRunning::key_ptziniting_handle(int key)
{
	SHORT ks = GetKeyState(VK_SHIFT);

	int move_timeout = 250;
	if (ks != 0) {
		move_timeout = 50;
	}

	switch (key) {
	case 27:	// esc
		ptz_->set_running_state(0);
		state_ = RUNNING;
		break;

	case 13:	// 回车确认
		{
			if (ptz_->ptz()) {
				int h, v, z;
				ptz_get_current_position(ptz_->ptz(), &h, &v);
				z = ptz_zoom_get(ptz_->ptz());

				cfg_->set_value("ptz_init_x", h);
				cfg_->set_value("ptz_init_y", v);
				cfg_->set_value("ptz_init_z", z);

				// 同时设置为预置位 ???
				// 

				cfg_->save_as(0);
			}

			ptz_->set_running_state(0);
			state_ = RUNNING;
		}
		break;

	case 0x3d:	// '='
		if (ptz_->ptz()) ptz_->add(task_ptz_zoom(ptz_->ptz(), true, 7, 100));
		break;

	case 0x2b:	// '+'
		// 倍率加
		if (ptz_->ptz()) ptz_->add(task_ptz_zoom(ptz_->ptz(), true, 7, 200));
		break;

	case 0x5f:
		if (ptz_->ptz()) ptz_->add(task_ptz_zoom(ptz_->ptz(), false, 7, 100));
		break;

	case 0x2d: // '-'
		// 倍率减
		if (ptz_->ptz()) ptz_->add(task_ptz_zoom(ptz_->ptz(), false, 7, 200));
		break;

	case 0x250000:	// Left
		if (ptz_->ptz()) ptz_->add(task_ptz_move(ptz_->ptz(), 1, 1, move_timeout));
		break;

	case 0x270000:	// Right
		if (ptz_->ptz()) ptz_->add(task_ptz_move(ptz_->ptz(), 2, 1, move_timeout));
		break;

	case 0x260000:	// Up
		if (ptz_->ptz()) ptz_->add(task_ptz_move(ptz_->ptz(), 3, 1, move_timeout));
		break;

	case 0x280000:	// Down
		if (ptz_->ptz()) ptz_->add(task_ptz_move(ptz_->ptz(), 4, 1, move_timeout));
		break;
	}
}

void TeacherRunning::key_calibrating_handle(int key)
{
	switch (key) {
	case 27:	// ESC 取消
		state_ = RUNNING;
		enable_mask(true);
		break;

	case 13:	// 回车确认
		{
			// 需要根据 calibrations_ 生成新的 mask_
			if (calibrations_.size() < 4) {
				av_log(0, AV_LOG_WARNING, "%s: en calibration points NOT enough!! Just remove calibration area ...\n", __FUNCTION__);

				cfg_->set_value("calibration_data", "");
				cfg_->save_as(0);
			}
			else {
				char *pts = (char*)alloca(4096);	// FIXME: 应该足够了 :)
				pts[0] = 0;
				int n = 0;

				{
					ost::MutexLock al(cs_calibrations_);
					for (size_t i = 0; i < calibrations_.size(); i++) {
						n += sprintf(pts+n, "%d,%d;", calibrations_[i].pt.x, calibrations_[i].pt.y);
					}
				}

				if (state_ == CALIBRATING)
					cfg_->set_value("calibration_data", pts);
				else if (state_ == CALIBRATING2)
					cfg_->set_value("calibration_data2", pts);
				else if (state_ == CALIBRATING3)
					cfg_->set_value("calibration_data3", pts);

				cfg_->save_as(0);
			}
			{
				ost::MutexLock al(cs_mask_);
				if (img_mask_) cvReleaseImage(&img_mask_);
				if (state_ == CALIBRATING)
					img_mask_ = build_mask("calibration_data");
				else
					img_mask_ = build_mask("calibration_data2", "calibration_data3");
			}

			enable_mask(true);
			state_ = RUNNING;	// 进入RUNNING
		}
		break;
	}
}

void TeacherRunning::mouse_detected(int ev, int x, int y, int flags)
{
	/** 处理探测窗口的鼠标事件 */
	if (state_ == CALIBRATING ||
		state_ == CALIBRATING2 ||
		state_ == CALIBRATING3) {
		if (ev == CV_EVENT_LBUTTONUP) {
			// 增加一个点
			ost::MutexLock al(cs_calibrations_);
			CalibrationPoint cp;
			cp.pt.x = x;
			cp.pt.y = y;

			calibrations_.push_back(cp);
		}
	}
}

void TeacherRunning::mouse_traced(int ev, int x, int y, int flags)
{
	/** 处理跟踪窗口的鼠标事件 */
	if (state_ == PTZINITING) {
		switch (ev) {
		case CV_EVENT_LBUTTONUP:
			{
				// 点哪指哪 ...
				int width = atoi(cfg_->get_value("video_window", "960")), height = atoi(cfg_->get_value("video_height", "540"));
				double hr = (x - width / 2) * 1.0 / (width / 2);
				double vr = (height / 2 - y) * 1.0 / (height / 2);
				if (ptz_->ptz()) {
					ptz_->add(task_ptz_center(ptz_->ptz(), hr, vr, 30));
				}
			}
			break;
		}
	}
}

void TeacherRunning::show_info(IplImage *img)
{
	/** 在 img 左侧，显示帮助信息 */
	CvPoint pt = { 10, 100 };
	CvFont font = cvFont(1.0);

	switch (state_) {
	case RUNNING:
		// cvPutText(img, "'c' to Calibrate teacher area...", pt, &font, CV_RGB(255, 255, 0));
		cvPutText(img, "'b' to Calibrate detecting 1st line....", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		cvPutText(img, "'d' to Calibrate detecting 2nd line....", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		cvPutText(img, "'p' to set ptz init position....", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		cvPutText(img, "'l' to set teacher LEFT threshold ...", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		cvPutText(img, "'r' to set teacher RIGHT threshold ...", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		show_state_ = "... running ...";
		break;

	case CALIBRATING:
		cvPutText(img, "ESC to cancel and back.", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		cvPutText(img, "ENTER to save and back.", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		show_state_ = "using mouse to set teacher calibration area...";
		break;

	case PTZINITING:
		cvPutText(img, "ESC to cancel and back.", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		cvPutText(img, "ENTER to save and back.", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		show_state_ = "mouse click for the init point in 'Tracing Window' ... ";

		/** 在屏幕中心显示一个小方块，便于对准
		 */
		{
			cv::Rect r(0, 0, width_, height_);
			cv::Point ul(CENTER_X(r)-1, CENTER_Y(r)-1), dr(CENTER_X(r)+1, CENTER_Y(r)+1);
			cvRectangle(img, ul, dr, CV_RGB(255, 0, 0));
		}

		break;

	case PTZINIT_LEFT:
	case PTZINIT_RIGHT:
		cvPutText(img, "ESC to calcel and back.", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		cvPutText(img, "ENTER to save and back.", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		show_state_ = "using LEFT/RIGHT key to adjust ptz...";
		break;

	case CALIBRATING2:
	case CALIBRATING3:
		cvPutText(img, "ESC to cancel and back.", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		cvPutText(img, "ENTER to save and back.", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		show_state_ = "using mouse to set teacher calibration area...";
		break;
	}

	// 显示状态信息：
	pt.x = 10, pt.y = img->height - 10 - 15;
	cvPutText(img, show_state_.c_str(), pt, &font, CV_RGB(255, 0, 0));
}

void TeacherRunning::one_image(IplImage *rawimg, IplImage *img, double stamp)
{
	if (!img) img = rawimg;	// 防止没有 masked 的错误

	if (state_ == RUNNING) {

	}
	else if (state_ == CALIBRATING || state_ == CALIBRATING2 || state_ == CALIBRATING3) {
		// 标定状态
		ost::MutexLock al(cs_calibrations_);
		int n = 1;
		CALIBRATIONS::iterator it_last = calibrations_.begin();
		for (CALIBRATIONS::iterator it = calibrations_.begin(); it != calibrations_.end(); ++it) {
			CvPoint p1 = { it->pt.x-2, it->pt.y-2 };
			CvPoint p2 = { it->pt.x+2, it->pt.y+2 };
			cvRectangle(img, p1, p2, CV_RGB(0, 255, 0));

			CvFont font = cvFont(1.0);

			CvPoint pt = { p2.x + 3, p1.y };
			char info[16];
			snprintf(info, sizeof(info), "%d", n++);
			cvPutText(img, info, pt, &font, CV_RGB(0, 255, 0));

			if (it != it_last) {
				cvLine(img, it_last->pt, it->pt, CV_RGB(0, 255, 0));
				it_last = it;
			}
		}

		if (!calibrations_.empty()) {
			if (calibrations_.size() < 4) {
				cvLine(img, it_last->pt, calibrations_.begin()->pt, CV_RGB(255, 0, 0));
			}
			else {
				cvLine(img, it_last->pt, calibrations_.begin()->pt, CV_RGB(0, 255, 0));
			}
		}
	}

	vector<Pos> r;
	if (det_->one_frame(img, roi_, r)) {
		TeacherPtz::DetectedData data;
		data.valid = true;
		data.poss = r;
		ptz_->set_detected_data(data);
	}
	else {
		TeacherPtz::DetectedData data;
		ptz_->set_detected_data(data);
	}

	show_info(img);

	// 用于提示当前输出窗口
	if (switcher_ == FULLVIEW) {
		cvCircle(img, cv::Point(30, 30), 10, CV_RGB(255, 0, 0), 20);

		cvShowImage("movie output", rawimg);
	}

	show_image(true, true, img);

	switch (switcher_) {
	case FULLVIEW:
		if (ptz_->is_stable()) {
			// 说明此时可以切换到跟踪镜头
			switcher_ = CLOSEVIEW;
		}
		break;

	case CLOSEVIEW:
		if (!ptz_->is_stable()) {
			// 需要切换到全景镜头
			switcher_ = FULLVIEW;
		}
		break;
	}
}

void TeacherRunning::before_show_tracing_image(IplImage *img)
{
	if (switcher_ == CLOSEVIEW) {
		// 提示当前那个作为输出窗口
		cvCircle(img, cv::Point(30, 30), 10, CV_RGB(255, 0, 0), 20);
		cvShowImage("movie output", img);
	}

	switch (state_) {
	case PTZINITING:
		{
			// 在图像中心显示一个红点，方便对准
			cv::Rect r(0, 0, img->width, img->height);
			cv::Point ul(CENTER_X(r)-1, CENTER_Y(r)-1), dr(CENTER_X(r)+1, CENTER_Y(r)+1);
			cvRectangle(img, ul, dr, CV_RGB(255, 0, 0));
		}
		break;

	case PTZINIT_LEFT:
	case PTZINIT_RIGHT:
		{
			// 屏幕画竖直红色线
			cvLine(img, cv::Point(img->width/2, 0), cv::Point(img->width/2, img->height), CV_RGB(255, 0, 0));
		}
		break;

	case RUNNING:
		{
#if 0
			// 画稳定框
			int sw = atoi(cfg_->get_value("stable_width", "600")), sh = atoi(cfg_->get_value("stable_height", "360")), st = atoi(cfg_->get_value("stable_top", "100"));
			cv::Point ul((width_ - sw) / 2, st), dr(ul.x + sw, ul.y + sh);
			cvRectangle(img, ul, dr, CV_RGB(0, 0, 255));

			cv::Point trace_pos = ptz_->get_trace_last_pos();
			if (trace_pos.x == -1000) {
				cvCircle(img, cv::Point(width_/2, height_/2), 15, CV_RGB(255, 255, 0), 1, 4);
			}
			else {
				trace_pos.x += width_ / 2;
				trace_pos.y += height_ / 2;
				cvCircle(img, trace_pos, 15, CV_RGB(255, 255, 0), 5);
			}
#endif
		}
		break;
	}
}
