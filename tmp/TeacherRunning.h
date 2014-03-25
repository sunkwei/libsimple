#pragma once

#include <opencv2/opencv.hpp>
#include "tdt.h"
#include "../libzqpkt_img_fifo/simple_student_trace/KVConfig.h"
#include "Running.h"
#include "../../libptz/ptz.h"
#include "../teacher_detect_trace/ZoomValueConvert.h"
#include "TracePolicy.h"
#include <algorithm>
#include "ptz_oper.h"
#include "TeacherDetecting.h"
#include "TeacherPtz.h"

/** 教师探测：保存探测摄像头中的数据，用于给 Impl 进行分析处理
 */
class TeacherRunning : public Running
{
	pfn_tdt_notify notify_;
	void *opaque_;
	int &enabled_;
	TeacherDetecting *det_;
	std::vector<cv::Point> roi_;

	enum State {
		RUNNING,		// 一般运行
		CALIBRATING,	// 区域标定
		PTZINITING,		// 正在进行
		CALIBRATING2,	// 标定黑板下沿，用于采“帧差法/背景剪除”法
		CALIBRATING3,	// 对应第二段标定

		PTZINIT_LEFT,	// 左侧标定
		PTZINIT_RIGHT,	// 右侧标定
	};
	State state_;		// 

	// 切换
	enum SwitchTo {
		FULLVIEW,		// 全景模式
		CLOSEVIEW,		// 近景模式
	};
	SwitchTo switcher_;	// 

	std::string show_state_;	// 用于显示的状态信息

	struct CalibrationPoint
	{
		cv::Point pt;	// 标定点的位置
	};
	typedef std::vector<CalibrationPoint> CALIBRATIONS;
	CALIBRATIONS calibrations_;		// 标定点队列，由此连接为封闭框
	ost::Mutex cs_calibrations_;

//	bool reset_want_, exec_test_want_;

	TeacherPtz *ptz_;

public:
	TeacherRunning(WindowThread *th, KVConfig *cfg, pfn_tdt_notify notify, void *opaque, int &enabled);
	~TeacherRunning(void);

private:
	virtual void one_image(IplImage *raw_img, IplImage *masked_img, double stamp);
	virtual bool is_teacher() const { return true; }
	virtual void mouse_detected(int ev, int x, int y, int flags);
	virtual void mouse_traced(int ev, int x, int y, int flags);
	
	virtual int key_handle(int key)
	{
		switch (state_) {
		case RUNNING:
			key_running_handle(key);
			break;

		case CALIBRATING:
		case CALIBRATING2:
		case CALIBRATING3:
			key_calibrating_handle(key);
			break;

		case PTZINIT_LEFT:
		case PTZINIT_RIGHT:
			key_ptzinit_left_right_handle(key);
			break;

		case PTZINITING:
			key_ptziniting_handle(key);
			break;
		}

		return 0;
	}

	virtual void before_show_tracing_image(IplImage *img);

	void key_running_handle(int key);
	void key_calibrating_handle(int key);
	void key_ptziniting_handle(int key);
	void key_ptzinit_left_right_handle(int key);

	void show_info(IplImage *img);

	std::vector<cv::Point> load_roi(KVConfig *cfg);
};
