#include "StudentRunningImpl.h"

StudentRunningImpl::StudentRunningImpl(KVConfig *cfg, ptz_t *ptz)
	: cfg_(cfg)
	, ptz_(ptz)
{
	init_h_ = atoi(cfg_->get_value("ptz_init_x", "0"));
	init_v_ = atoi(cfg_->get_value("ptz_init_y", "0"));
	init_z_ = atoi(cfg_->get_value("ptz_init_z", "0"));

	quit_ = false;
	start();
}

StudentRunningImpl::~StudentRunningImpl(void)
{
	quit_ = true;
	sem_.post();
	join();
}

void StudentRunningImpl::run()
{
	while (!quit_) {
		sem_.wait();
		if (quit_) break;

		Task *t = next_task();
		if (t) {
			if (t->should_do(t, time(0))) {
				t->handle(t);
			}

			if (t->free)
				t->free(t);
			else
				delete t;
		}
	}
}
