/*
 * 作者：庄金峰
 * 说明：
 *   在IMX6上，初始化顺序是 CLOCK->IOMUX->THIS CONTROLLER
 * 版权：上海华元创信软件有限公司
 */

#include <driver.h>
#include <semaphore.h>

// 设备数量
#define XXX_DEV_NUM 	1

// 主设备号
#define XXX_MAJOR 		180

// 次设备号
#define XXX_MIN_START	0
#define XXX_MIN_END		10

// 寄存器基地址
#define XXX_BASE        0x00000000

// IRQ中断号
#define XXX_IRQ         0

// 寄存器结构体
struct xxx_regs {
    unsigned int ctrl;
    unsigned int status;
};

typedef struct {
	char path[64];
	unsigned int dev;
} xxx_dev_t;

static xxx_dev_t xxx_devs[XXX_DEV_NUM];

// 寄存器访问辅助工具
static unsigned int inline xxx_readb()
{
}

// 其他辅助工具

// 操作接口

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

// 操作接口定义

struct file_operations xxx_ops = {
		open:	xxx_open,
		close:	xxx_close,
		read:	xxx_read,
		write:	xxx_write,
		ioctl:	xxx_ioctl,
};

// 中断处理接口，一定要清除控制器发送的中断信号

static void xxx_isr(void *arg)
{
	
}

// 控制器初始化接口

void xxx_init(void)
{
    
}

// 模块注册接口

int xxx_module_init(void)
{
    int i;
    
    char path[32];
    
    dev_t major, minor;
    
    // 初始化驱动软件资源
    
//    sem_init2(&priv->sem_data_arrive,0,SEM_BINARY,PTHREAD_WAITQ_PRIO,1);
    
    // 开启控制器时钟

    // IO复用控制器配置
    
    // 控制器初始化
    xxx_init();
    
    // 控制器初始化后可以响应中断了，向系统层注册中断并使能
    int_install_handler(XXX_IRQ, xxx_isr, NULL);
    int_enable_pic(XXX_IRQ);
    
    // 此时可以被用户使用了，向用户层注册操作接口
    register_driver(XXX_MAJOR, &xxx_ops, &major);
    for(i=0; i<XXX_DEV_NUM; i++) {
    	chrdev_minor_request(XXX_MAJOR, XXX_MIN_START, 1, XXX_MIN_END, &minor);
    	xxx_devs[i].dev = MKDEV(major, minor);
        sprintf(xxx_devs[i].path, "/dev/xxx%d", i);
        register_chrdev(xxx_devs[i].path, xxx_devs[i].dev, (void*)&xxx_devs[i]);
    }
    
    return 0;
}
