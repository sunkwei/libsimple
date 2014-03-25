#include "StudentRunning.h"

StudentRunning::StudentRunning(WindowThread *th, KVConfig *cfg)
	: Running(th, cfg, STUDENT_DETWINNAME, STUDENT_TACWINNAME)
{
	running_state_ = RUNNING;

	load_camera_params();

	ptz_ = new StudentPtz(cfg);
	det_ = new StudentDetecting(cfg);
}

StudentRunning::~StudentRunning(void)
{
	delete det_;
	delete ptz_;
}

void StudentRunning::load_camera_params()
{
	int initx = atoi(cfg_->get_value("ptz_init_x", "0"));
	int inity = atoi(cfg_->get_value("ptz_init_y", "0"));
	double ax = atof(cfg_->get_value("cam_trace_min_hangle", "0.075"));
	double ay = atof(cfg_->get_value("cam_trace_min_vangle", "0.075"));

	cp_detecting_.f = atof(cfg_->get_value("cam_detect_f", "4.0"));
	cp_detecting_.image = cv::Size(atoi(cfg_->get_value("video_width", "960")), atoi(cfg_->get_value("video_height", "540")));
	cp_detecting_.scale = 1.0;
	cp_detecting_.w = atof(cfg_->get_value("cam_detect_ccd_w", "4.8"));
	cp_detecting_.h = atof(cfg_->get_value("cam_detect_ccd_h", "2.7"));
	cp_detecting_.ag_x = 0.0;
	cp_detecting_.ag_y = 0.0;

	cp_tracing_.f = atof(cfg_->get_value("cam_trace_f", "4.0"));
	cp_tracing_.w = atof(cfg_->get_value("cam_trace_ccd_w", "4.8"));
	cp_tracing_.h = atof(cfg_->get_value("cam_trace_ccd_h", "2.7"));
	cp_tracing_.image.width = atoi(cfg_->get_value("video_width", "960"));
	cp_tracing_.image.height = atoi(cfg_->get_value("video_height", "540"));
	cp_tracing_.ag_x = initx * ax * M_PI / 180.0;
	cp_tracing_.ag_y = inity * ay * M_PI / 180.0;
	cp_tracing_.scale = -1.0;		// 必须在运行时设置！！！


}

int StudentRunning::key_handle(int key)
{
	switch (running_state_) {
	case RUNNING:
		key_handle_running(key);
		break;

	case PTZINITING:
		key_handle_ptziniting(key);
		break;

	case CALIBRATING2:
		key_handle_calibrating(key);
		break;
	}

	return 0;
}

void StudentRunning::one_image(IplImage *raw_img, IplImage *img, double stamp)
{
	if (!img) img = raw_img;

	std::vector<StudentAction> actions = det_->one_frame(img);

	if (running_state_ == CALIBRATING2) {
		show_calibrating(img);
	}

	for (int i = 0; i < actions.size(); i++)
	{
		if (actions[i].action == 1)
		{
			cvRectangle(img, cvPoint(actions[i].pos.x, actions[i].pos.y), cvPoint(actions[i].pos.x + actions[i].pos.width, actions[i].pos.y + actions[i].pos.height),
				cvScalar(255, 255, 0), 2);
		}
		else
			cvRectangle(img, cvPoint(actions[i].pos.x, actions[i].pos.y), cvPoint(actions[i].pos.x + actions[i].pos.width, actions[i].pos.y + actions[i].pos.height),
				cvScalar(0, 0, 255), 2);	
	}

	show_info(img);
	show_image(false, true, img);
	
	
	// TODO: 这里处理 actions 中所有动作！！！
}

void StudentRunning::mouse_detected(int ev, int x, int y, int flags)
{
	/** 处理探测窗口的鼠标事件 */
	if (running_state_ == CALIBRATING2) {
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

void StudentRunning::mouse_traced(int ev, int x, int y, int flags)
{
	/** 处理跟踪窗口的鼠标事件 */
	if (running_state_ == PTZINITING) {
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

void StudentRunning::show_calibrating(IplImage *img)
{
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

void StudentRunning::show_info(IplImage *img)
{
	/** 在 img 左侧，显示帮助信息 */
	CvPoint pt = { 10, 100 };
	CvFont font = cvFont(1.0);
	std::string info;

	switch (running_state_) {
	case RUNNING:
		// cvPutText(img, "'c' to Calibrate teacher area...", pt, &font, CV_RGB(255, 255, 0));
		cvPutText(img, "'B' to Calibrate detecting 1st line....", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		cvPutText(img, "'P' to set ptz init position....", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		info = "... running ...";
		break;

	case CALIBRATING2:
		cvPutText(img, "ESC to cancel and back.", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		cvPutText(img, "ENTER to save and back.", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		info = "using mouse to set teacher calibration area...";
		break;

	case PTZINITING:
		cvPutText(img, "ESC to cancel and back.", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		cvPutText(img, "ENTER to save and back.", pt, &font, CV_RGB(255, 255, 0));
		pt.y += 15;
		info = "mouse click for the init point in 'Tracing Window' ... ";

		/** 在屏幕中心显示一个小方块，便于对准
		 */
		{
			cv::Rect r(0, 0, width_, height_);
			cv::Point ul(CENTER_X(r)-1, CENTER_Y(r)-1), dr(CENTER_X(r)+1, CENTER_Y(r)+1);
			cvRectangle(img, ul, dr, CV_RGB(255, 0, 0));
		}

		break;
	}

	// 显示状态信息：
	pt.x = 10, pt.y = img->height - 10 - 15;
	cvPutText(img, info.c_str(), pt, &font, CV_RGB(255, 0, 0));
}

void StudentRunning::key_handle_running(int key)
{
	switch (key) {
	case 'B':
		{
			running_state_ = CALIBRATING2;
			ost::MutexLock al(cs_calibrations_);
			calibrations_.clear();
			enable_mask(false);	// 临时禁用 ...
		}
		break;

	case 'P':
		if (ptz_->ptz()) {
			ptz_stop(ptz_->ptz());
			ptz_zoom_stop(ptz_->ptz());
		}
		ptz_->set_running_state(1);
		running_state_ = PTZINITING;
		ptz_->reset_ptz();
		break;
	}
}

void StudentRunning::key_handle_ptziniting(int key)
{
	SHORT ks = GetKeyState(VK_SHIFT);

	int move_timeout = 250;
	if (ks != 0) {
		move_timeout = 50;
	}

	switch (key) {
	case 27:	// esc
		ptz_->set_running_state(0);
		running_state_ = RUNNING;
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
			running_state_ = RUNNING;
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

void StudentRunning::key_handle_calibrating(int key)
{
	switch (key) {
	case 27:	// ESC 取消
		running_state_ = RUNNING;
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

				if (running_state_ == CALIBRATING)
					cfg_->set_value("calibration_data", pts);
				else if (running_state_ == CALIBRATING2)
					cfg_->set_value("calibration_data2", pts);
				else if (running_state_ == CALIBRATING3)
					cfg_->set_value("calibration_data3", pts);

				cfg_->save_as(0);
			}
			{
				ost::MutexLock al(cs_mask_);
				if (img_mask_) cvReleaseImage(&img_mask_);
				if (running_state_ == CALIBRATING)
					img_mask_ = build_mask("calibration_data");
				else
					img_mask_ = build_mask("calibration_data2", "calibration_data3");
			}

			enable_mask(true);
			running_state_ = RUNNING;	// 进入RUNNING
		}
		break;
	}
}
