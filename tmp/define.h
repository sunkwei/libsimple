#pragma once

// Ŀǰʹ�� RGB24����ʵʹ�� YUV �е� Y ��������� ...
#define PIXFMT PIX_FMT_RGB24

#define TEACHER_DETWINNAME	"teacher detecting window"
#define STUDENT_DETWINNAME	"student detecting window"
#define TEACHER_TACWINNAME	"teacher tracing window"
#define STUDENT_TACWINNAME	"student tracing window"

#define CENTER_X(r) ((r).x+(r).width/2)
#define CENTER_Y(r) ((r).y+(r).height/2)
#define PT_IN_RECT(r, pt) ((pt).x>(r).x && (pt).x<(r).x+(r).width && (pt).y>(r).y && (pt).y<(r).y+(r).height)

#include "version.h"
