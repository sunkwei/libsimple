#pragma once

#include "ptz_oper.h"

class StudentRunningImpl : ost::Thread
{
	KVConfig *cfg_;
	ptz_t *ptz_;

	bool quit_;
	ost::Semaphore sem_;
	ost::Mutex cs_;
	std::deque<Task*> tasks_;

	Camera_parameter cp_trace, cp_detect;
	Coordinate_Conversion cc;
	TracePolicy *policy_;

	double init_v_, init_h_, init_z_;	// 云台初始夹角，初始zoom value


public:
	StudentRunningImpl(KVConfig *cfg, ptz_t *ptz);
	~StudentRunningImpl(void);

private:
	void run();	// 线程函数

	Task *next_task()
	{
		ost::MutexLock al(cs_);
		if (!tasks_.empty()) {
			Task *t = tasks_.front();
			tasks_.pop_front();
			return t;
		}
		return 0;
	}
};
