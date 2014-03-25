#pragma once

#include "ptz_oper.h"
#include "../libzqpkt_img_fifo/simple_student_trace/KVConfig.h"
#include <cc++/thread.h>
#include "log.h"
#include "../teacher_detect_trace/ZoomValueConvert.h"

class Ptz : ost::Thread
{
	KVConfig *cfg_;
	ptz_t *ptz_;
	
	bool quit_;
	ost::Semaphore sem_;
	ost::Mutex cs_;
	std::deque<Task*> tasks_;

public:
	Ptz(KVConfig *cfg);
	~Ptz(void);

	ptz_t *ptz() { return ptz_; }
	void add(Task *task);
	void reset_ptz();

	double current_scale();

private:
	void run();
	Task *next_task();
	void exec_task(Task *t);
	void load_ptz_params();

protected:
	virtual void one_loop() {}

	struct PtzParam
	{
		double focus;
		double view_angle_hori, view_angle_verb;
		double ccd_hori, ccd_verb;
		double min_angle_hori, min_angle_verb;

		ZoomValueConvert *zvc;
	};
	PtzParam ptz_params_;
};
