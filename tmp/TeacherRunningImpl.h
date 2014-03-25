#pragma once

#include "TeacherAnalyze.h"
#include "ptz_oper.h"
#include "log.h"

// #define TEST 1

//#define fprintf(out, ...) mylog(__VA_ARGS__)

/** IMPORTANT: 对于教师探测跟踪来说，有两个状态，
		1：全景状态：
			a. 目标持续在较大范围走动
			b. 或者无目标状态；
			c. 此时跟踪相机应该追逐目标；
			d. 此时电影模式输出全景相机画面；

		2：近景状态：
			a. 目标持续不动，或者活动范围很小；
			b. 跟踪相机不转动；
			c. 电影模式输出跟踪相机图像；
			d. 全景需要检测，目标是否离开当前探测相机的视野；
	*/
enum TeacherImplRunningState
{
	FULLVIEW,	// 全景状态
	TO_CLOSE,	// 正在走到近景模式，呵呵，主要是考虑云台转动到目标位置，可能需要一段时间的
	CLOSEVIEW,	// 近景状态

#ifdef TEST
	TESTING,
#endif 

	STATE_LAST,
};

// 用于删除超时的
struct ChkTimeoutTeacherDetected
{
	double stamp_;
	ChkTimeoutTeacherDetected(double stamp) : stamp_(stamp)
	{
	}

	bool operator()(const TeacherDetected &td) const
	{
		return td.stamp < stamp_;
	}
};

enum TeacherTracingResult
{
	TTR_OK,						// 数据有效
	TTR_NO_ENONGH_PTS,			// 没有足够的点子
	TTR_NO_ENOUGH_VALID_PTS,	// 没有足够的有效点
	TTR_TOO_OLD_VALID_PT,		// 有效点的时间过时
};

struct TeacherTracingInfo
{
	TeacherTracingResult result;		// 结果类型

	ptz_t *ptz;		// 云台控制

	cv::Rect rc_detect;	// 最近一段时间内，探测到目标的活动范围
	cv::Rect rc_trace;	// 对应到跟踪相机中的活动范围

	double curr, last_stamp;	// 当前和最后一个有效点的时间
	cv::Point pt_detect_last;	// 最后点，在探测中的位置
	cv::Point pt_trace_last;	// 最后点，在跟踪当前的位置，注意：使用 
	double valid_duration;		// 数据中，第一个有效点到最后一个有效点的时间差

	Rot_Angle ang_trace_to_last;	// 跟踪相机，如果需要指向最后目标需要转动的弧度
	double zoom_scale;				// 跟踪相机倍率
	Rot_Angle trace_curr_ang;	// 当前跟踪云台的角度，弧度

	int hvec, vvec;		// 此段时间内探测窗口中的活动向量

	TeacherImplRunningState curr_state, next_state;	// 状态

	KVConfig *cfg;
	TracePolicy *policy;

	struct Testing_t
	{
		Testing_t() { current_dir = MIDDLE; }

		Dir current_dir;	// 当前转动方向
	} Testing;

	struct ToClose_t
	{
		int h, v;	// 云台的绝对位置
	} ToClose;
};

/** 云台工作线程
 */
class TeacherWorkingThread : ost::Thread
{
	double stamp_begin_;	// ...
	double stamp_last_trace_;
	cv::Point last_detected_pt_;

	// 状态处理函数原型
	typedef int (TeacherWorkingThread::*pfn_state_func)(TeacherTracingInfo &info);
	pfn_state_func state_funcs_[STATE_LAST];

	KVConfig *cfg_;

	bool quit_;
	ost::Semaphore sem_;
	ost::Mutex cs_;
	std::deque<Task*> tasks_;
		
	Camera_parameter cp_trace_, cp_detect_;
	Range_parameter rp_;
	Coordinate_Conversion cc_;
	TracePolicy *policy_;
	TeacherAnalyze *analyzer_;

	double init_v_, init_h_; // 云台初始夹角
	double ax_, ay_;		// 0.075
	cv::Size stable_size_;	// 稳定框大小
	cv::Point stable_center_;	// 稳定框中心

	cv::Size last_hog_size_;	// 用于保存最后的头肩大小
	double last_hog_size_stamp_;	// 设置时间
	cv::Point trace_last_pos_;	// 

	ptz_t *ptz_;
	ZoomValueConvert zvc;

	DWORD tick_init_;
	DWORD curr() { return GetTickCount() - tick_init_; }

	cv::Point debug_trace_pos_;	// debug

	int test_ptz_last_dir_;		// FIXME: 貌似频繁发出云台转动命令，如果相同的方向，会不停地卡顿 ???? 这是云台的问题么？

	TeacherImplRunningState state_;

	// 用于保存状态的上下文数据
	struct RunningStateContext
	{
		struct {
			int h, v;	// 希望云台的指向
		} to_close;
	};
	RunningStateContext state_ctx_;

	// 跟踪云台当前参数，仅仅用于转动状态中间 ....
	class TracingData
	{
	public:
		KVConfig *cfg_;

	public:
		int max_speed, min_speed;	// 最大最小速度
		int speed;	// 当前云台速度
		Dir dir;	// 当前方向
		int distance;	// 到中心的距离

		void init()
		{
			max_speed = atoi(cfg_->get_value("ptz_max_speed", "10"));
			min_speed = atoi(cfg_->get_value("ptz_min_speed", "1"));
			speed = atoi(cfg_->get_value("ptz_init_speed", "2"));
			dir = MIDDLE;
			distance = -1;
		}
	};
	TracingData tracing_data_;

	TEACHERDETECTED teacher_detected_data_;	// 保存一段时间内的探测数据
	ost::Mutex cs_teacher_detected_data_;
	TeacherTracingInfo trace_info_;

	int running_state_;	// 0: 正常，1: 云台初始化
			
public:
	TeacherWorkingThread(KVConfig *cfg, ptz_t *ptz);
	~TeacherWorkingThread();

	void set_ptz(ptz_t *ptz)
	{
		ptz_ = ptz;
	}

	void set_running_state(int state)
	{
		running_state_ = state;
	}

	void add(Task *t)
	{
		ost::MutexLock al(cs_);
		tasks_.push_back(t);
		sem_.post();
	}

	size_t count()
	{
		ost::MutexLock al(cs_);
		return tasks_.size();
	}

	// 当收到 camshift 成功时，pt 为 camshift 的中心点
	// 当 camshift 失败时，必须清空 ...
	void set_camshift_pt(const cv::Point &pt)
	{
		ost::MutexLock al(cs_teacher_detected_data_);
		TeacherDetected td;
		td.stamp = now();
		td.pt = pt;
		td.stamp_size = last_hog_size_stamp_;
		td.hog_size = last_hog_size_;

		teacher_detected_data_.push_back(td);

		ChkTimeoutTeacherDetected cttd(td.stamp - atof(cfg_->get_value("analyze_duration", "10.0")));	// 最多需要保留的
		TEACHERDETECTED::iterator itf = std::remove_if(teacher_detected_data_.begin(), teacher_detected_data_.end(), cttd);
		teacher_detected_data_.erase(itf, teacher_detected_data_.end());

		sem_.post();
	}

	void set_hog_size(int width, int height)
	{
		ost::MutexLock al(cs_teacher_detected_data_);
		last_hog_size_.width = width, last_hog_size_.height = height;
		last_hog_size_stamp_ = now();
	}

	cv::Point get_trace_last_pos() const { return trace_last_pos_; }

private:
	int state_fullview(TeacherTracingInfo &info);
	int state_closeview(TeacherTracingInfo &info);
	int state_toclose(TeacherTracingInfo &info);
	int state_testing(TeacherTracingInfo &info);

	Task *next()
	{
		ost::MutexLock al(cs_);
		Task *t = 0;
		if (!tasks_.empty()) {
			t = tasks_.front();
			tasks_.pop_front();
		}

		return t;
	}

	void run()
	{
		while (!quit_) {
			sem_.wait();
			if (quit_)
				break;

			if (running_state_ == 0) {
				TEACHERDETECTED datas;
				do {
					ost::MutexLock al(cs_teacher_detected_data_);
					datas = teacher_detected_data_;
				} while (0);

				if (datas.size() >= 2)	// FIXME: 至少两个点以上 ...
					trytrace(datas, now());
			}

			Task *t = next();	// 下个等待的任务
			if (t) {
				// 如果 should_do 有效，并且认为无需执行，则掠过
				if (t->should_do && !t->should_do(t, time(0))) {
				}
				else {
					if (t->handle)
						t->handle(t);
				}

				if (t->free) 
					t->free(t);
				else 
					delete t;
			}
		}
	}

	int trytrace(TEACHERDETECTED &data, double curr);

	void reset_ptz(ptz_t *ptz);
	void stop_ptz(ptz_t *ptz);

	// 转动跟踪云台，指向跟踪相机的点
	void set_ptz_abs_pos(ptz_t *ptz, const cv::Point &pt, int speed, int *hh = 0, int *vv = 0);
	int get_ptz_abs_pos(ptz_t *ptz, int *h, int *v);

	Rot_Angle get_trace_ptz_abs_pos();
	double get_trace_zoom_scale();

	// 返回稳定框的中心
	cv::Point get_stable_center() const	{ return stable_center_; }

	// 从探测相机的矩形，转换到跟踪相机的矩形
	// WARNING: 这个矩形仅仅适合表达大小，位置信息是相对的
	void det2tra(const cv::Rect &rc, const cv::Point &lastpt, const Rot_Angle &ang, cv::Rect &trace_rc, cv::Point &trace_lastpt);

	// 从 state_ 转换到 state2
	void change_state(TeacherImplRunningState s2);
};
