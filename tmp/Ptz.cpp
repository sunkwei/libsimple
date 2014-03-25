#include "Ptz.h"

Ptz::Ptz(KVConfig *cfg)
	: cfg_(cfg)
{
	ptz_ = 0;

	const char *name = cfg_->get_value("ptz_serial_name", "COMX");
	if (strcmp(name, "COMX")) {
		ptz_ = ptz_open(name, atoi(cfg_->get_value("ptz_addr", "1")));

		if (!ptz_)
			mylog("WARNING: %s: to open ptz of '%s'ERR!, cfg='%s' \n", __FUNCTION__, name, cfg_->file_name());
		else
			mylog("INFO: %s: en open ptz of '%s' OK! cfg='%s'\n", __FUNCTION__, name, cfg_->file_name());
	}
	else {
		mylog("WARNING: %s: en, NO ptz config, cfg='%s'!!!\n", __FUNCTION__, cfg_->file_name());
	}

	load_ptz_params();

	quit_ = false;
	start();
}

Ptz::~Ptz(void)
{
	quit_ = true;
	join();

	delete ptz_params_.zvc;
}

void Ptz::load_ptz_params()
{
	ptz_params_.focus = atof(cfg_->get_value("cam_trace_f", "3.5"));
	ptz_params_.ccd_hori = atof(cfg_->get_value("cam_trace_ccd_w", "5.1326"));
	ptz_params_.ccd_verb = atof(cfg_->get_value("cam_trace_ccd_h", "2.8852"));
	ptz_params_.min_angle_hori = atof(cfg_->get_value("cam_trace_min_hangle", "0.075"));
	ptz_params_.min_angle_verb = atof(cfg_->get_value("cam_trace_min_vangle", "0.075"));
	ptz_params_.view_angle_hori = atof(cfg_->get_value("cam_trace_view_hangle", "72.5"));
	ptz_params_.view_angle_verb = atof(cfg_->get_value("cam_trace_view_vangle", "44.8"));

	ptz_params_.zvc = new ZoomValueConvert(cfg_);
}

Task *Ptz::next_task()
{
	ost::MutexLock al(cs_);
	Task *t = 0;
	if (!tasks_.empty()) {
		t = tasks_.front();
		tasks_.pop_front();
	}

	return t;
}

void Ptz::exec_task(Task *t)
{
	if (t->should_do && !t->should_do(t, now())) return;
	if (t->handle) t->handle(t);
	if (t->free) t->free(t);
	else delete t;
}

void Ptz::add(Task *task)
{
	if (task) {
		ost::MutexLock al(cs_);
		tasks_.push_back(task);
	}

	sem_.post();
}

void Ptz::reset_ptz()
{
	if (ptz_) {
		int h = atoi(cfg_->get_value("ptz_init_x", "0"));
		int v = atoi(cfg_->get_value("ptz_init_y", "0"));
		int z = atoi(cfg_->get_value("ptz_init_z", "5000"));

		add(task_ptz_reset(ptz_, h, v, z));
	}
}

void Ptz::run()
{
	reset_ptz();

	while (!quit_) {
		sem_.wait();

		one_loop();

		while (Task *t = next_task())
			exec_task(t);
	}
}

double Ptz::current_scale()
{
	if (!ptz_) return 1.0;
	int z = ptz_zoom_get(ptz_);
	return ptz_params_.zvc->mp_zoom(z);
}
