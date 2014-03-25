#include "TeacherRunningImpl.h"
#include <algorithm>
#include "define.h"
#include <math.h>

TeacherWorkingThread::TeacherWorkingThread(KVConfig *cfg, ptz_t *ptz) 
	: cfg_(cfg), zvc(cfg), ptz_(ptz)
{
	mylog_init();
	fprintf(stdout, "%s: started ....\n", __FUNCTION__);

	tick_init_ = GetTickCount();
	running_state_ = 0;

	stamp_begin_ = now();
	stamp_last_trace_ = stamp_begin_;

	tracing_data_.cfg_ = cfg;
	trace_last_pos_ = cv::Point(0, 0);

	state_funcs_[FULLVIEW] = &TeacherWorkingThread::state_fullview;
	state_funcs_[TO_CLOSE] = &TeacherWorkingThread::state_toclose;
	state_funcs_[CLOSEVIEW] = &TeacherWorkingThread::state_closeview;

#ifdef TEST
	state_ = TESTING;
	state_funcs_[TESTING] = &TeacherWorkingThread::state_testing;
#else
	state_ = FULLVIEW;
	change_state(FULLVIEW);	// ȱʡȫ��״̬

#endif // testing

	test_ptz_last_dir_ = -1;

	debug_trace_pos_ = cv::Point(-1, -1);

	int initx = atoi(cfg_->get_value("ptz_init_x", "0"));
	int inity = atoi(cfg_->get_value("ptz_init_y", "0"));
	ax_ = atof(cfg_->get_value("cam_trace_min_hangle", "0.075"));
	ay_ = atof(cfg_->get_value("cam_trace_min_vangle", "0.075"));

	// ��̨�Ƚ��г�ʼλ�� ...
	reset_ptz(ptz_);

	// ���������ʼ�н�
	init_h_ = initx * ax_ * M_PI / 180.0;	// ת��Ϊ����
	init_v_ = inity * ay_ * M_PI / 180.0;

	cp_trace_.f = atof(cfg_->get_value("cam_trace_f", "4.0"));
	cp_trace_.w = atof(cfg_->get_value("cam_trace_ccd_w", "4.8"));
	cp_trace_.h = atof(cfg_->get_value("cam_trace_ccd_h", "2.7"));
	cp_trace_.image.width = atoi(cfg_->get_value("video_width", "960"));
	cp_trace_.image.height = atoi(cfg_->get_value("video_height", "540"));
	cp_trace_.ag_x = init_h_;
	cp_trace_.ag_y = init_v_;
	cp_trace_.scale = -1.0;		// ����������ʱ���ã�����

	cp_detect_.f = atof(cfg_->get_value("cam_detect_f", "4.0"));
	cp_detect_.w = atof(cfg_->get_value("cam_detect_ccd_w", "4.8"));
	cp_detect_.h = atof(cfg_->get_value("cam_detect_ccd_h", "2.7"));
	cp_detect_.image.width = atoi(cfg_->get_value("video_width", "960"));
	cp_detect_.image.height = atoi(cfg_->get_value("video_height", "540"));
	cp_detect_.ag_x = 0.0;
	cp_detect_.ag_y = 0.0;
	cp_detect_.scale = 1.0;

	// �ȶ���
	stable_size_.width = atoi(cfg_->get_value("stable_width", "660"));
	stable_size_.height = atoi(cfg_->get_value("stable_height", "340"));
	int stop = atoi(cfg_->get_value("stable_top", "100"));
	stable_center_ = cv::Point(cp_trace_.image.width/2, stop + stable_size_.height/2);	// �ȶ�������λ��.

	policy_ = new TracePolicy(cfg_);
	analyzer_ = new TeacherAnalyze(cfg_, policy_);

	last_hog_size_ = cv::Size(-1, -1);

	quit_ = false;
	start();
}

TeacherWorkingThread::~TeacherWorkingThread()
{
	quit_ = true;
	sem_.post();
	join();
	delete analyzer_;
	delete policy_;
}

void TeacherWorkingThread::reset_ptz(ptz_t *ptz)
{
	if (ptz) {
		add(task_ptz_reset(ptz, 
			atoi(cfg_->get_value("ptz_init_x", "0")),
			atoi(cfg_->get_value("ptz_init_y", "0")),
			atoi(cfg_->get_value("ptz_init_z", "5000"))));
	}
	test_ptz_last_dir_  = -1;
}

void TeacherWorkingThread::stop_ptz(ptz_t *ptz)
{
	if (ptz) {
		ptz_stop(ptz);
		ptz_zoom_stop(ptz);
	}
}

void TeacherWorkingThread::set_ptz_abs_pos(ptz_t *ptz, const cv::Point &pt, int speed, int *hh /* = 0 */, int *vv /* = 0 */)
{
	// �����������Ҫ�ľ���λ�ã����ȣ���Ҫת��Ϊ��̨����
	if (ptz) {
		Rot_Angle ang = cc_.point_cov(pt, cp_detect_, cp_trace_);

		int h = (int)(ang.x * 180.0 / M_PI / ax_);
		int v = (int)(ang.y * 180.0 / M_PI / ay_);

		ptz_set_absolute_position(ptz, h, v, speed);

		if (hh) *hh = h;
		if (vv) *vv = v;
	}
}

int TeacherWorkingThread::get_ptz_abs_pos(ptz_t *ptz, int *h, int *v)
{
	if (ptz) {
		ptz_get_current_position(ptz, h, v);
		return 0;
	}
	else
		return -1;
}

Rot_Angle TeacherWorkingThread::get_trace_ptz_abs_pos()
{
	Rot_Angle ra = { 0.0, 0.0 };

	if (ptz_) {
		int h, v;
		ptz_get_current_position(ptz_, &h, &v);

		ra.x = h * atof(cfg_->get_value("cam_trace_min_hangle", "0.075"));
		ra.y = v * atof(cfg_->get_value("cam_trace_min_vangle", "0.075"));

		ra.x *= M_PI, ra.x /= 180.0;
		ra.y *= M_PI, ra.y /= 180.0;
	}

	return ra;
}

double TeacherWorkingThread::get_trace_zoom_scale()
{
	double scale = 1.0;

	if (ptz_) {
		int z = ptz_zoom_get(ptz_);
		scale = zvc.mp_zoom(z);
	}

	return scale;
}

void TeacherWorkingThread::det2tra(const cv::Rect &rc, const cv::Point &lastpt, const Rot_Angle &ang, cv::Rect &trace_rc, cv::Point &trace_lastpt)
{
	Range_parameter rp;
	rp.angle = ang;

	rp.current_p = cv::Point(rc.x, rc.y);	// ����
	cv::Point2f ul = cc_.cam_rotation(rp, cp_detect_, cp_trace_);

	rp.current_p = cv::Point(rc.x+rc.width, rc.y+rc.height);	// ����
	cv::Point2f dr = cc_.cam_rotation(rp, cp_detect_, cp_trace_);

	rp.current_p = lastpt;
	cv::Point2f lpt = cc_.cam_rotation(rp, cp_detect_, cp_trace_);

	trace_rc = cv::Rect((int)ul.x, (int)ul.y, (int)(dr.x-ul.x), (int)(dr.y-ul.y));
	trace_lastpt = cv::Point(lpt.x, lpt.y);
}

void TeacherWorkingThread::change_state(TeacherImplRunningState s2)
{
	static const char *desc[] = {
		"FullView state",
		"F2C state",
		"CloseView state", 
		"Testing ....",
	};

	fprintf(stderr, "INFO: state from '%s' change to '%s':\n", desc[state_], desc[s2]);

	state_ = s2;	// IMPORTANT: 

	if (s2 == FULLVIEW) {
		// ֻҪ�ص�ȫ��״̬���ͳ�ʼ�� TracingData��Ϊ���ܵĸ�����׼��
		tracing_data_.init();
	}

	// TODO: ���� notify ???
}

/** �ڹ����߳��е��ã� data �а��������һ��ʱ���ڣ�����̽�⵽������, curr Ϊ��ǰʱ��
 */
int TeacherWorkingThread::trytrace(TEACHERDETECTED &data, double curr)
{
	trace_last_pos_ = cv::Point(-1000, -1000);	// FIXME: ����Ϊ�˲���ʾ

	trace_info_.result = TTR_OK;

	if (!analyzer_->update_data(data)) {
		// ����̫����
		trace_info_.result = TTR_NO_ENONGH_PTS;
	}

	double valid_delta;
	analyzer_->data_duration(valid_delta);

	cv::Point last_valid;
	double last_valid_stamp;
	analyzer_->data_last_valid(last_valid, last_valid_stamp);
	if (curr - last_valid_stamp > policy_->time_to_stop_tracing()) {
		// ̫��ʱ��û��Ŀ����
		trace_info_.result = TTR_TOO_OLD_VALID_PT;
	}

	int valided, invalided;
	if (trace_info_.result == TTR_OK) {
		analyzer_->data_valided(&valided, &invalided);
		/** FIXME: ����������Ч�㣬������Ч��Ӧ�ö�����Ч�����Ŀ����
  			*/
		if (/* valided < invalided || */ valided < 2) {
			fprintf(stderr, "%s: valid=%d, invalid=%d\n", __FUNCTION__, valided, invalided);
			trace_info_.result = TTR_NO_ENOUGH_VALID_PTS;
		}
	}

	// ��ǰ��̨λ�ã�����
	Rot_Angle curr_ang = get_trace_ptz_abs_pos();
	double scale = get_trace_zoom_scale();
	cp_trace_.scale = scale;

	// ������Ϣ
	trace_info_.ptz = ptz_;
	trace_info_.curr = curr;
	trace_info_.policy = policy_;
	trace_info_.cfg = cfg_;

	if (trace_info_.result == TTR_OK) {
		cv::Rect rc_detect;
		analyzer_->data_range(rc_detect);

		int hvec, vvec;
		analyzer_->data_vec(&hvec, &vvec);

		trace_info_.valid_duration = valid_delta;
		trace_info_.pt_detect_last = last_valid;
		trace_info_.last_stamp = last_valid_stamp;
		trace_info_.trace_curr_ang = curr_ang;
		det2tra(rc_detect, last_valid, curr_ang, trace_info_.rc_trace, trace_info_.pt_trace_last); // ��̽��ת��������
		trace_info_.rc_detect = rc_detect;
		trace_info_.ang_trace_to_last = cc_.point_cov(trace_info_.pt_detect_last, cp_detect_, cp_trace_);
		trace_info_.zoom_scale = scale;
		trace_info_.hvec = hvec;
		trace_info_.vvec = vvec;
		trace_info_.curr_state = state_;
		trace_info_.next_state = state_;	// XXX:

		trace_last_pos_ = trace_info_.pt_trace_last;	// ������������е����λ��

		(this->*state_funcs_[state_])(trace_info_);
		if (trace_info_.next_state != state_) {
			change_state(trace_info_.next_state);
		}

		if (last_detected_pt_.x != trace_info_.pt_detect_last.x && last_detected_pt_.y != trace_info_.pt_detect_last.y) {
			fprintf(stderr, "%s: ======= %.3f (%.3f) =========\n"
				"\tvd:%d, invd:%d, valid_delta=%.3f\n"
				"\tlast detected: {%d,%d, %d,%d}, pt={%d,%d}\n"
				"\tlast tracing: {%d,%d, %d,%d}, pt={%d,%d}\n"
				"\thvec:%d, vvec:%d\n"
				"\tptz to set: %.2f, %.2f, zoom scale: %.3f, ptz curr ang={%.0f,%.0f}\n\n",
				__FUNCTION__, curr - stamp_begin_, curr - stamp_last_trace_,
				valided, invalided, valid_delta,
				rc_detect.x, rc_detect.y, rc_detect.width, rc_detect.height, trace_info_.pt_detect_last.x, trace_info_.pt_detect_last.y,
				trace_info_.rc_trace.x, trace_info_.rc_trace.y, trace_info_.rc_trace.width, trace_info_.rc_trace.height, trace_info_.pt_trace_last.x, trace_info_.pt_trace_last.y,
				hvec, vvec,
				trace_info_.ang_trace_to_last.x * 180.0 / M_PI, trace_info_.ang_trace_to_last.y * 180.0 / M_PI, trace_info_.zoom_scale, trace_info_.trace_curr_ang.x*180.0/M_PI, trace_info_.trace_curr_ang.y*180.0/M_PI);

			last_detected_pt_ = trace_info_.pt_detect_last;
		}
	}
	else {
		fprintf(stderr, "%d ", trace_info_.result);

		trace_info_.curr_state = state_;
		trace_info_.next_state = state_;

		(this->*state_funcs_[state_])(trace_info_);
		if (trace_info_.next_state != state_) {
			change_state(trace_info_.next_state);
		}
	}

	stamp_last_trace_ = curr;

	return 0;
}

int TeacherWorkingThread::state_testing(TeacherTracingInfo &info)
{
#ifdef TEST
	info.next_state = TESTING;	// 

	if (!info.ptz) {
		return -1;
	}

	/** ���Ŀ��λ�ã��ڸ�����Ұ�ڣ�ʹ��ƽ��ת��ģʽ������ֱ��ָ��Ŀ�� 

			�Ƿ�����Ұ�ڣ�ʹ�� info.pt_trace_last �жϣ��Ƿ���̽�ⴰ����

	 */

	int vw = atoi(info.cfg->get_value("video_width", "960")), vh = atoi(info.cfg->get_value("video_height", "540"));

	// ���Ŀ��������ıȽϽ�˵��Ŀ���ʱ����Ұ��Χ��
	if (abs(info.pt_trace_last.x) < vw/2 && abs(info.pt_trace_last.y) < vh/2) {
#if 1
		switch (info.Testing.current_dir) {
		case LEFT:
			// һֱת���� pt_trace_last.x ������
			if (info.pt_trace_last.x > 20) {
				ptz_stop(info.ptz);
				info.Testing.current_dir = MIDDLE;
			}
			break;

		case RIGHT:
			if (info.pt_trace_last.x < -20) {
				ptz_stop(info.ptz);
				info.Testing.current_dir = MIDDLE;
			}
			break;

		case UP:
			if (info.pt_trace_last.y > 10) {
				ptz_stop(info.ptz);
				info.Testing.current_dir = MIDDLE;
			}
			break;

		case DOWN:
			if (info.pt_trace_last.x < -10) {
				ptz_stop(info.ptz);
				info.Testing.current_dir = MIDDLE;
			}
			break;

		case MIDDLE:
			if (info.pt_trace_last.x < -80) {
				ptz_stop(info.ptz);
				ptz_left(info.ptz, 6);
				info.Testing.current_dir = LEFT;
			}
			else if (info.pt_trace_last.x > 80) {
				ptz_stop(info.ptz);
				ptz_right(info.ptz, 6);
				info.Testing.current_dir = RIGHT;
			}
			else if (info.pt_trace_last.y < -60) {
				ptz_stop(info.ptz);
				ptz_up(info.ptz, 5);
				info.Testing.current_dir = UP;
			}
			else if (info.pt_trace_last.y > 60) {
				ptz_stop(info.ptz);
				ptz_down(info.ptz, 5);
				info.Testing.current_dir = DOWN;
			}
			break;
		}

#else
		const char *dir_desc;
		pfn_ptz_move ptz_func;
		static Dir last_dir = MIDDLE;

		// FIXME: ����� info.pt_trace_last �е�ֵ���ƺ�����ȡ��������
		Dir dir = ptz_get_dir(cv::Point(0, 0), cv::Point(0 - info.pt_trace_last.x, 0 - info.pt_trace_last.y), &ptz_func, &dir_desc);
		if (last_dir != dir) {
			fprintf(stderr, "============ %s === [%d, %d] =============\n", dir_desc, -info.pt_trace_last.x, -info.pt_trace_last.y);
			last_dir = dir;
		}

		if (sqrt(pow(info.pt_trace_last.x*1.0, 2) + pow(info.pt_trace_last.y*1.0, 2)) < 80) {
			ptz_stop(info.ptz);
		}
		else {
			ptz_func(info.ptz, 3);
		}
#endif
	}
	else {
		// ֱ��ָ��Ŀ��
		double hh = info.ang_trace_to_last.x * 180.0 / M_PI;
		double vv = info.ang_trace_to_last.y * 180.0 / M_PI;
		double ax = atof(info.cfg->get_value("cam_trace_min_hangle", "0.075"));
		double ay = atof(info.cfg->get_value("cam_trace_min_vangle", "0.075"));

		int h = (int)(hh / ax), v = (int)(vv / ay);

		ptz_set_absolute_position(info.ptz, h, v, 30);

		info.Testing.current_dir = MIDDLE;

		fprintf(stderr, "%s[%.3f]: en, need set PTZ absolute pos: %d, %d, curr pos=%.0f, %.0f\n"
			"\ttrace info: %d, %d\n"
			"\tdetect info: %d, %d\n",
			__FUNCTION__, info.curr - stamp_begin_,
			h, v, info.trace_curr_ang.x*180/M_PI, info.trace_curr_ang.y*180/M_PI,
			info.pt_trace_last.x, info.pt_trace_last.y,
			info.pt_detect_last.x, info.pt_detect_last.y);
	}
#endif // TEST
	return 0;
}

int TeacherWorkingThread::state_fullview(TeacherTracingInfo &info)
{
	if (info.result == TTR_OK) {
		// ��� info.range С���ȶ��򣬲����˶�ʸ��С��rc_detect��˵��Ŀ�������ض��������������ΪĿ�����������
		int stable_width = stable_size_.width, stable_height = stable_size_.height;

		if (info.valid_duration >= policy_->analyze_interval() &&
			info.rc_trace.width < stable_width && info.rc_trace.height < stable_height &&
			info.hvec < info.rc_detect.width * 2 / 3 && info.vvec <= info.rc_detect.height) {

			mylog("%s: en, goto CLOSEVIEW: tracing size=(%d, %d), vector=(%d, %d), stable size=(%d,%d)\n",
				__FUNCTION__, info.rc_trace.width, info.rc_trace.height,
				info.hvec, info.vvec, stable_width, stable_height);

			// ����̨ת��ָ��Ŀ�����λ�ã���������һ��״̬
			set_ptz_abs_pos(info.ptz, info.pt_detect_last, 30, &info.ToClose.h, &info.ToClose.v);
			info.next_state = TO_CLOSE;
		}
		else {
			// TODO: ��ʱ˵��Ŀ����Χ̫����Ҫת����̨ʵʱ����Ŀ�� ...
			fprintf(stderr, "%s: traced range size=(%d, %d), vector=(%d, %d), stable_size=(%d, %d)\n",
				__FUNCTION__, info.rc_trace.width, info.rc_trace.height,
				info.hvec, info.vvec, stable_width, stable_height);
		}
	}
	else if (info.result == TTR_TOO_OLD_VALID_PT) {
		// ����̨��λ
		reset_ptz(info.ptz);
	}

	return 0;
}

int TeacherWorkingThread::state_toclose(TeacherTracingInfo &info)
{
	// FIXME��������̨ת����λ��ʱ��Ƚ϶̣������м䲻����Ŀ�궪ʧ������ˣ�������ǽ��� CLOSEVIEW �����жϰ�.
	int h, v;
	if (get_ptz_abs_pos(info.ptz, &h, &v) < 0) {
		// ��̨��Ч��ֱ�ӷ��� FULLVIEW
		info.next_state = FULLVIEW;
	}
	else {
		if (std::abs(h - info.ToClose.h) < 3 &&
			std::abs(v - info.ToClose.v) < 3) {
			// FIXME: ��̨���ܲ�̫׼ȷ��ֻ�ܴ��²�� ...
			info.next_state = CLOSEVIEW;
		}
	}

	return 0;
}

int TeacherWorkingThread::state_closeview(TeacherTracingInfo &info)
{
	/** ����״̬��ֻҪĿ������λ�ã��뿪�ȶ���������ת���� FULLVIEW ģʽ
	 */
	if (std::abs(info.pt_trace_last.x) > stable_size_.width / 2) {
		mylog("%s: en, back to FULLVIEW, last trace point={%d,%d}, stable width=%d(%d)\n",
			__FUNCTION__, info.pt_trace_last.x, info.pt_trace_last.y,
			stable_size_.width, stable_size_.width/2);
		info.next_state = FULLVIEW;
	}

	return 0;
}
