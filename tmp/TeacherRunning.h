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

/** ��ʦ̽�⣺����̽������ͷ�е����ݣ����ڸ� Impl ���з�������
 */
class TeacherRunning : public Running
{
	pfn_tdt_notify notify_;
	void *opaque_;
	int &enabled_;
	TeacherDetecting *det_;
	std::vector<cv::Point> roi_;

	enum State {
		RUNNING,		// һ������
		CALIBRATING,	// ����궨
		PTZINITING,		// ���ڽ���
		CALIBRATING2,	// �궨�ڰ����أ����ڲɡ�֡�/������������
		CALIBRATING3,	// ��Ӧ�ڶ��α궨

		PTZINIT_LEFT,	// ���궨
		PTZINIT_RIGHT,	// �Ҳ�궨
	};
	State state_;		// 

	// �л�
	enum SwitchTo {
		FULLVIEW,		// ȫ��ģʽ
		CLOSEVIEW,		// ����ģʽ
	};
	SwitchTo switcher_;	// 

	std::string show_state_;	// ������ʾ��״̬��Ϣ

	struct CalibrationPoint
	{
		cv::Point pt;	// �궨���λ��
	};
	typedef std::vector<CalibrationPoint> CALIBRATIONS;
	CALIBRATIONS calibrations_;		// �궨����У��ɴ�����Ϊ��տ�
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
