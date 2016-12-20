/*
 * ���ߣ�ׯ���
 * ˵����
 *   ��IMX6�ϣ���ʼ��˳���� CLOCK->IOMUX->THIS CONTROLLER
 * ��Ȩ���Ϻ���Ԫ����������޹�˾
 */

#include <driver.h>
#include <semaphore.h>

// �豸����
#define XXX_DEV_NUM 	1

// ���豸��
#define XXX_MAJOR 		180

// ���豸��
#define XXX_MIN_START	0
#define XXX_MIN_END		10

// �Ĵ�������ַ
#define XXX_BASE        0x00000000

// IRQ�жϺ�
#define XXX_IRQ         0

// �Ĵ����ṹ��
struct xxx_regs {
    unsigned int ctrl;
    unsigned int status;
};

typedef struct {
	char path[64];
	unsigned int dev;
} xxx_dev_t;

static xxx_dev_t xxx_devs[XXX_DEV_NUM];

// �Ĵ������ʸ�������
static unsigned int inline xxx_readb()
{
}

// ������������

// �����ӿ�

static void *xxx_open(void *registered, const char *pathname, int flags, mode_t mode)
{
    xxx_dev_t *dev = (xxx_dev_t *)registered;
    
    return dev;
}

static int xxx_close(void *opened)
{
    xxx_dev_t *dev = (xxx_dev_t *)opened;
    
    return 0;
}

static int xxx_read(void *opened, uint8_t *buffer, size_t count)
{
    xxx_dev_t *dev = (xxx_dev_t *)opened;
    
    return 0;
}

static int xxx_write(void *opened, uint8_t *buffer, size_t count)
{
    xxx_dev_t *dev = (xxx_dev_t *)opened;
    
    return 0;
}

static void xxx_ioctl(void *opened, int cmd, void *arg)
{
    xxx_dev_t *dev = (xxx_dev_t *)opened;
    
    return 0;
}

// �����ӿڶ���

struct file_operations xxx_ops = {
		open:	xxx_open,
		close:	xxx_close,
		read:	xxx_read,
		write:	xxx_write,
		ioctl:	xxx_ioctl,
};

// �жϴ���ӿڣ�һ��Ҫ������������͵��ж��ź�

static void xxx_isr(void *arg)
{
	
}

// ��������ʼ���ӿ�

void xxx_init(void)
{
    
}

// ģ��ע��ӿ�

int xxx_module_init(void)
{
    int i;
    
    char path[32];
    
    dev_t major, minor;
    
    // ��ʼ�����������Դ
    
//    sem_init2(&priv->sem_data_arrive,0,SEM_BINARY,PTHREAD_WAITQ_PRIO,1);
    
    // ����������ʱ��

    // IO���ÿ���������
    
    // ��������ʼ��
    xxx_init();
    
    // ��������ʼ���������Ӧ�ж��ˣ���ϵͳ��ע���жϲ�ʹ��
    int_install_handler(XXX_IRQ, xxx_isr, NULL);
    int_enable_pic(XXX_IRQ);
    
    // ��ʱ���Ա��û�ʹ���ˣ����û���ע������ӿ�
    register_driver(XXX_MAJOR, &xxx_ops, &major);
    for(i=0; i<XXX_DEV_NUM; i++) {
    	chrdev_minor_request(XXX_MAJOR, XXX_MIN_START, 1, XXX_MIN_END, &minor);
    	xxx_devs[i].dev = MKDEV(major, minor);
        sprintf(xxx_devs[i].path, "/dev/xxx%d", i);
        register_chrdev(xxx_devs[i].path, xxx_devs[i].dev, (void*)&xxx_devs[i]);
    }
    
    return 0;
}
