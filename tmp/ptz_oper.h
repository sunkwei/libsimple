#pragma once

#include "tdt.h"
#include "../libzqpkt_img_fifo/simple_student_trace/KVConfig.h"
#include "../simple_teacher_trace/fullimage_recognition.h"
#include "../../libptz/ptz.h"
#include "TracePolicy.h"
#include <algorithm>
#include "../teacher_detect_trace/ZoomValueConvert.h"

// 描述方向，其中 MIDDLE 认为是 stop 状态.
enum Dir
{
	LEFT, RIGHT, UP, DOWN, UPLEFT, UPRIGHT, DOWNLEFT, DOWNRIGHT, MIDDLE,
};

typedef int(*pfn_ptz_move)(ptz_t *ptz, int speed);

// 为了与 ptz_left, right, ... 的原型一致 ...
int my_ptz_stop(ptz_t *ptz, int notused);

/** 云台操作，往往比较耗时，所以放到工作线程中执行
*/
struct Task
{
	bool (*should_do)(Task *t, time_t now);
	void (*handle)(Task *t);
	void (*free)(Task *t);

	ptz_t *ptz;
	void *opaque;
};

struct Opaque_ptz_move
{
	int speed, timeout, dir;
};

static void task_ptz_move_free(Task *t)
{
	Opaque_ptz_move *m = (Opaque_ptz_move*)t->opaque;
	delete m;
	delete t;
}

static void task_ptz_move_handle(Task *t)
{
	Opaque_ptz_move *m = (Opaque_ptz_move*)t->opaque;
	switch (m->dir) {
	case 1: // left
		ptz_left(t->ptz, m->speed);
		break;
	case 2: // right
		ptz_right(t->ptz, m->speed);
		break;
	case 3: // up
		ptz_up(t->ptz, m->speed);
		break;
	case 4: // down
		ptz_down(t->ptz, m->speed);
		break;
	}

	ost::Thread::sleep(m->timeout);
	ptz_stop(t->ptz);
}

static Task *task_ptz_move(ptz_t *ptz, int dir, int speed, int timeout)
{
	if (ptz == 0) return 0;

	Task *t = new Task;
	t->opaque = new Opaque_ptz_move;
	t->ptz = ptz;

	((Opaque_ptz_move*)t->opaque)->dir = dir;
	((Opaque_ptz_move*)t->opaque)->speed = speed;
	((Opaque_ptz_move*)t->opaque)->timeout = timeout;

	t->handle = task_ptz_move_handle;
	t->should_do = 0;
	t->free = task_ptz_move_free;

	return t;
}

struct Opaque_ptz_zoom
{
	bool tele;
	int speed;
	int timeout;
};

static void task_ptz_zoom_free(Task *t)
{
	Opaque_ptz_zoom *z = (Opaque_ptz_zoom*)t->opaque;
	delete z;
	delete t;
}

static void task_ptz_zoom_handle(Task *t)
{
	Opaque_ptz_zoom *z = (Opaque_ptz_zoom*)t->opaque;
	if (z->tele)
		ptz_zoom_tele(t->ptz, z->speed);
	else
		ptz_zoom_wide(t->ptz, z->speed);
		
	ost::Thread::sleep(z->timeout);

	ptz_zoom_stop(t->ptz);
}

static Task *task_ptz_zoom(ptz_t *ptz, bool tele, int speed, int timeout)
{
	if (ptz == 0) return 0;

	Task *t = new Task;
	t->opaque = new Opaque_ptz_zoom;
	t->ptz = ptz;

	((Opaque_ptz_zoom*)t->opaque)->tele = tele;
	((Opaque_ptz_zoom*)t->opaque)->speed = speed;
	((Opaque_ptz_zoom*)t->opaque)->timeout = timeout;

	t->handle = task_ptz_zoom_handle;
	t->should_do = 0;
	t->free = task_ptz_zoom_free;

	return t;
}

struct Opaque_ptz_center
{
	double hr, vr;
	int speed;
};

static void task_ptz_center_free(Task *t)
{
	Opaque_ptz_center *c = (Opaque_ptz_center*)t->opaque;
	delete c;
	delete t;
}

static void task_ptz_center_handle(Task *t)
{
	Opaque_ptz_center *c = (Opaque_ptz_center*)t->opaque;
	ptz_ext_location_center(t->ptz, c->hr, c->vr, c->speed, 0);
}

static Task *task_ptz_center(ptz_t *ptz, double hr, double vr, int speed)
{
	if (ptz == 0) return 0;
	Task *t = new Task;
	t->opaque = new Opaque_ptz_center;
	t->ptz = ptz;

	((Opaque_ptz_center*)t->opaque)->hr = hr;
	((Opaque_ptz_center*)t->opaque)->vr = vr;
	((Opaque_ptz_center*)t->opaque)->speed = speed;

	t->handle = task_ptz_center_handle;
	t->should_do = 0;
	t->free = task_ptz_center_free;

	return t;
}

struct Opaque_ptz_reset
{
	int h, v, z;
};

static void task_ptz_reset_free(Task *t)
{
	Opaque_ptz_reset *c = (Opaque_ptz_reset*)t->opaque;
	delete c;
	delete t;
}

static void task_ptz_reset_handle(Task *t)
{
	Opaque_ptz_reset *c = (Opaque_ptz_reset*)t->opaque;
	ptz_zoom_set(t->ptz, c->z);
	ptz_set_absolute_position(t->ptz, c->h, c->v, 30);
}

static Task *task_ptz_reset(ptz_t *ptz, int h, int v, int z)
{
	if (!ptz) return 0;
	Task *t = new Task;
	t->opaque = new Opaque_ptz_reset;
	Opaque_ptz_reset *p = (Opaque_ptz_reset*)t->opaque;
	p->h = h, p->v = v, p->z = z;
	t->ptz = ptz;
	t->handle = task_ptz_reset_handle;
	t->free = task_ptz_reset_free;
	t->should_do = 0;

	return t;
}

/** 异步转动到指定的绝对坐标 */
static Task *task_ptz_set_position(ptz_t *ptz, int h, int v, int z)
{
	if (!ptz) return 0;
	return task_ptz_reset(ptz, h, v, z);
}

/** 根据 a,b 的位置，计算云台转动方向
 */
Dir ptz_get_dir(const cv::Point &a, const cv::Point &b, pfn_ptz_move *func = 0, const char **desc = 0);
