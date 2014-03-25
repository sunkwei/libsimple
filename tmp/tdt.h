#ifndef _zonekey_teacher_detact_trace__HH
#define _zonekey_teacher_detact_trace__HH

/** ��ʦͼ��̽����٣�
		����ԭ��
			����̽���������ͼ�񣬾����������ҵ���ʦ��λ�ã����Ƹ�����������и��١�ʹ�����õĲ��ԣ�֪ͨ���е�Ӱģʽ���л�

		�л�����:
			��֤��Ӱģʽ�У���������̨ת���;�ͷ����ľ�ͷ��
			1. ��Ӱ���������ʦ��������Χ��С��ʱ����ʦ����������ʦ�߶�ʱ����ʦȫ����
			2. ��ȫ��ʱ������������ٽ�ʦ��
 */

#ifdef __cplusplus
extern "C" {
#endif // c++

typedef struct tdt_t tdt_t;

typedef enum tdt_actions
{
	/** TODO: ���¶���֪ͨ���룬 ....
	 */

	/// �������� :(
	TDT_ACTION_SWITCH_TO_FULLVIEW = 1,	// ��ӰӦ���л���ȫ��ģʽ��info Ϊ""
	TDT_ACTION_SWITCH_TO_CLOSEUP = 2,	// ��ӰӦ���л�������ģʽ��info Ϊ""

	TDT_TARGET_FOUND = 101,		// ���ָ���Ŀ�꣬info ��Ϊ "%d,%d" ָ��Ŀ�������
	TDT_TARGET_SIZE = 102,		// ����Ŀ��ͷ���С��info Ϊ "%d,%d" ָ��Ŀ���С
} tdt_actions;

/** �ص�֪ͨ��
 */
typedef void(*pfn_tdt_notify)(void *opaque, tdt_actions action_code, const char *info);

/** ����ʵ�� 
	@param cfg_name: �����ļ�����
	@param notify: �ص�����
	@param opaque: �ص������ĵ�һ������
	@return dtdʵ����0 ʧ��
 */
tdt_t *tdt_open(const char *cfg_name, pfn_tdt_notify notify, void *opaque);
void tdt_close(tdt_t *ctx);

/** �Ƿ����� */
void tdt_enable(tdt_t *ctx, int enable);
int tdt_is_enable(tdt_t *ctx);

/** д���ʦ/ѧ��̽�������yv12ͼ�����ݣ����ݴ�С���Ա仯 ...
	
	@param ctx: tdt_open() ���صľ��
	@param width��ͼ����
	@param height: ͼ��߶�
	@param fmt: ͼ���ʽ��Ŀǰ���� 0���൱�� yv12
	@param data: ͼ������
	@param stride: ����ÿ�е��ֽ���
	@param stamp�� ʱ���������Ϊ ���롱..
	@return 0 �ɹ�������ʧ��
 */
int tdt_write_teacher_pic(tdt_t *ctx, int width, int height, int fmt, unsigned char *data[4], int stride[4], double stamp);
int tdt_write_student_pic(tdt_t *ctx, int width, int height, int fmt, unsigned char *data[4], int stride[4], double stamp);

/** д���ʦ/ѧ�����������yv12ͼ������ */
int tdt_write_teacher_pic_tracing(tdt_t *ctx, int width, int height, int fmt, unsigned char *data[4], int stride[4], double stamp);
int tdt_write_student_pic_tracing(tdt_t *ctx, int width, int height, int fmt, unsigned char *data[4], int stride[4], double stamp);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif // c++

#endif // _zonekey_teacher_detact_trace__HH
