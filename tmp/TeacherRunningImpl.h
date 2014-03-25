#pragma once

#include "TeacherAnalyze.h"
#include "ptz_oper.h"
#include "log.h"

// #define TEST 1

//#define fprintf(out, ...) mylog(__VA_ARGS__)

/** IMPORTANT: ���ڽ�ʦ̽�������˵��������״̬��
		1��ȫ��״̬��
			a. Ŀ������ڽϴ�Χ�߶�
			b. ������Ŀ��״̬��
			c. ��ʱ�������Ӧ��׷��Ŀ�ꣻ
			d. ��ʱ��Ӱģʽ���ȫ��������棻

		2������״̬��
			a. Ŀ��������������߻��Χ��С��
			b. ���������ת����
			c. ��Ӱģʽ����������ͼ��
			d. ȫ����Ҫ��⣬Ŀ���Ƿ��뿪��ǰ̽���������Ұ��
	*/
enum TeacherImplRunningState
{
	FULLVIEW,	// ȫ��״̬
	TO_CLOSE,	// �����ߵ�����ģʽ���Ǻǣ���Ҫ�ǿ�����̨ת����Ŀ��λ�ã�������Ҫһ��ʱ���
	CLOSEVIEW,	// ����״̬

#ifdef TEST
	TESTING,
#endif 

	STATE_LAST,
};

// ����ɾ����ʱ��
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
	TTR_OK,						// ������Ч
	TTR_NO_ENONGH_PTS,			// û���㹻�ĵ���
	TTR_NO_ENOUGH_VALID_PTS,	// û���㹻����Ч��
	TTR_TOO_OLD_VALID_PT,		// ��Ч���ʱ���ʱ
};

struct TeacherTracingInfo
{
	TeacherTracingResult result;		// �������

	ptz_t *ptz;		// ��̨����

	cv::Rect rc_detect;	// ���һ��ʱ���ڣ�̽�⵽Ŀ��Ļ��Χ
	cv::Rect rc_trace;	// ��Ӧ����������еĻ��Χ

	double curr, last_stamp;	// ��ǰ�����һ����Ч���ʱ��
	cv::Point pt_detect_last;	// ���㣬��̽���е�λ��
	cv::Point pt_trace_last;	// ���㣬�ڸ��ٵ�ǰ��λ�ã�ע�⣺ʹ�� 
	double valid_duration;		// �����У���һ����Ч�㵽���һ����Ч���ʱ���

	Rot_Angle ang_trace_to_last;	// ��������������Ҫָ�����Ŀ����Ҫת���Ļ���
	double zoom_scale;				// �����������
	Rot_Angle trace_curr_ang;	// ��ǰ������̨�ĽǶȣ�����

	int hvec, vvec;		// �˶�ʱ����̽�ⴰ���еĻ����

	TeacherImplRunningState curr_state, next_state;	// ״̬

	KVConfig *cfg;
	TracePolicy *policy;

	struct Testing_t
	{
		Testing_t() { current_dir = MIDDLE; }

		Dir current_dir;	// ��ǰת������
	} Testing;

	struct ToClose_t
	{
		int h, v;	// ��̨�ľ���λ��
	} ToClose;
};

/** ��̨�����߳�
 */
class TeacherWorkingThread : ost::Thread
{
	double stamp_begin_;	// ...
	double stamp_last_trace_;
	cv::Point last_detected_pt_;

	// ״̬������ԭ��
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

	double init_v_, init_h_; // ��̨��ʼ�н�
	double ax_, ay_;		// 0.075
	cv::Size stable_size_;	// �ȶ����С
	cv::Point stable_center_;	// �ȶ�������

	cv::Size last_hog_size_;	// ���ڱ�������ͷ���С
	double last_hog_size_stamp_;	// ����ʱ��
	cv::Point trace_last_pos_;	// 

	ptz_t *ptz_;
	ZoomValueConvert zvc;

	DWORD tick_init_;
	DWORD curr() { return GetTickCount() - tick_init_; }

	cv::Point debug_trace_pos_;	// debug

	int test_ptz_last_dir_;		// FIXME: ò��Ƶ��������̨ת����������ͬ�ķ��򣬻᲻ͣ�ؿ��� ???? ������̨������ô��

	TeacherImplRunningState state_;

	// ���ڱ���״̬������������
	struct RunningStateContext
	{
		struct {
			int h, v;	// ϣ����̨��ָ��
		} to_close;
	};
	RunningStateContext state_ctx_;

	// ������̨��ǰ��������������ת��״̬�м� ....
	class TracingData
	{
	public:
		KVConfig *cfg_;

	public:
		int max_speed, min_speed;	// �����С�ٶ�
		int speed;	// ��ǰ��̨�ٶ�
		Dir dir;	// ��ǰ����
		int distance;	// �����ĵľ���

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

	TEACHERDETECTED teacher_detected_data_;	// ����һ��ʱ���ڵ�̽������
	ost::Mutex cs_teacher_detected_data_;
	TeacherTracingInfo trace_info_;

	int running_state_;	// 0: ������1: ��̨��ʼ��
			
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

	// ���յ� camshift �ɹ�ʱ��pt Ϊ camshift �����ĵ�
	// �� camshift ʧ��ʱ��������� ...
	void set_camshift_pt(const cv::Point &pt)
	{
		ost::MutexLock al(cs_teacher_detected_data_);
		TeacherDetected td;
		td.stamp = now();
		td.pt = pt;
		td.stamp_size = last_hog_size_stamp_;
		td.hog_size = last_hog_size_;

		teacher_detected_data_.push_back(td);

		ChkTimeoutTeacherDetected cttd(td.stamp - atof(cfg_->get_value("analyze_duration", "10.0")));	// �����Ҫ������
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

				if (datas.size() >= 2)	// FIXME: �������������� ...
					trytrace(datas, now());
			}

			Task *t = next();	// �¸��ȴ�������
			if (t) {
				// ��� should_do ��Ч��������Ϊ����ִ�У����ӹ�
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

	// ת��������̨��ָ���������ĵ�
	void set_ptz_abs_pos(ptz_t *ptz, const cv::Point &pt, int speed, int *hh = 0, int *vv = 0);
	int get_ptz_abs_pos(ptz_t *ptz, int *h, int *v);

	Rot_Angle get_trace_ptz_abs_pos();
	double get_trace_zoom_scale();

	// �����ȶ��������
	cv::Point get_stable_center() const	{ return stable_center_; }

	// ��̽������ľ��Σ�ת������������ľ���
	// WARNING: ������ν����ʺϱ���С��λ����Ϣ����Ե�
	void det2tra(const cv::Rect &rc, const cv::Point &lastpt, const Rot_Angle &ang, cv::Rect &trace_rc, cv::Point &trace_lastpt);

	// �� state_ ת���� state2
	void change_state(TeacherImplRunningState s2);
};
