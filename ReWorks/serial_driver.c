/******************************************************************************
 * 
 * 版权：
 * 		中国电子科技集团公司第三十二研究所
 * 描述：
 * 		本文件实现了x86平台ns16550A串口驱动
 * 修改：
 * 		2012-05-10，唐立三，因修改struct tty_driver而修改串口驱动
 * 		2012-05-05，唐立三，将增加串口结构体的计数属性，将中断使能放在open、close。
 * 		2011-11-18，唐立三，修改ISR方式，将ISR分为两个阶段实现
 * 		2011-10-22，唐立三，修改串口驱动层接口名称
 * 		2011-09-28，王志宏，修改poll读写接口名称
 * 		2011-09-26，唐立三，增加poll方式访问串口，重写ISR
 * 		2011-09-16，唐立三，根据自测结果，修改ioctl()接口、tx_char()接口和rx_char()
 * 							接口ioctl_define.h和regs.h文件，增加set_lcr()接口
 * 		2011-08-30，唐立三，统一寄存器定义
 * 		2011-08-27，唐立三，建立
 */
#include <pthread.h>			/* 线程 */
#include <irq.h>				/* 中断挂接 */
#include <string.h>				/* C */
#include <reworks/printk.h>		/* printk打印 */
#include <io_asm.h>				/* 端口访问 */
#include <driver.h>
#include "serial_driver.h"		/* 串口驱动头文件 */
#include <mpc8377.h>
#include <openpic.h>
/*==============================================================================
 * 
 * 全局变量
 * 
 */
/**
 * 串口参数定义
 * 	
 * 	此处定义的串口参数在串口设备初始化和注册阶段使用
 */
struct serial_params serial_param_define[SERIAL_NUMBER] =
{		/* 规则库名称     注册的设备路径   寄存器基 寄存器宽度  中断向量   基准波特率  波特率 */
		{LINE_LDISC_NAME, "/dev/serial0", SERIAL_BASE(0), 1, OPENPIC_VEC_UART1, CSB_CLK_DEFAULT, 115200},
		{LINE_LDISC_NAME, "/dev/serial1", SERIAL_BASE(1), 1, OPENPIC_VEC_UART2, CSB_CLK_DEFAULT, 115200}
};

/* 串口描述结构全局变量 */
struct serial_device_struct_ex global_serial_struct[SERIAL_NUMBER];

#define readb(addr) 		(*(volatile unsigned char *) (addr))
#define writeb(b, addr) 	(*(volatile unsigned char *) (addr) = (b))

#define RBR				0x00		/* 接收缓存寄存器 */
#define THR				0x00		/* transmit holding register */
#define LSR				0x05		/* line 状态寄存器 */
#define LSR_THRE		0x20		/* transmit holding register empty */

/******************************************************************************
 * 
 * 读取串口设备寄存器值
 * 
 * 输入：
 * 		driver：串口设备描述结构指针
 * 		offset：偏移
 * 输出：
 * 		无
 * 返回：
 * 		寄存器值
 * 备注：
 * 		由于不同硬件平台访问串口寄存器的方式不同，因而串口驱动中使用接口from/to_serial
 * 对访问串口的方式进行封装。建议根据不同平台的特点，修改from/to_serial的实现。
 */
static inline unsigned int from_serial(struct serial_device_struct_ex *driver, 
									   int offset)
{

	/* 内存基址加偏移，偏移为：寄存器偏移 + 寄存器字节宽度 */	
	return readb((int)(driver->regs_base + (offset * driver->delta)));
}


/******************************************************************************
 * 
 * 向串口设备寄存器写入数据
 * 
 * 输入：
 * 		driver：串口设备描述结构指针
 * 		offset：偏移
 * 		value：写入寄存器的值
 * 输出：
 * 		无
 * 返回：
 * 		寄存器值
 */
static inline void to_serial(struct serial_device_struct_ex *driver, 
							 int offset, 
							 int value)
{
	/* 内存基址加偏移，偏移为：寄存器偏移 + 寄存器字节宽度 */
	writeb(value, (int)(driver->regs_base + (offset * driver->delta)));

	return;
}



/******************************************************************************
 * 
 * 设置波特率接口；本程序为内部接口
 * 
 * 输入：
 * 		driver：
 * 输出：
 * 		无
 * 返回：
 * 		寄存器值
 */
static int set_baudrate(struct serial_device_struct_ex *driver, 
						unsigned int baudrate)
{
	int level;
	
	level = int_lock();	/* 锁中断 */
	
	/* 设置LCR，使得当前能够设置波特率 */
	to_serial(driver, SERIAL_LCR, driver->regs.LCR | SERIAL_LCR_DLAB);
	
	/* 设置波特率低、高分频系数 */
	to_serial(driver, SERIAL_DLL, (driver->base_baudrate / (16 * baudrate)));
	to_serial(driver, SERIAL_DLM, (driver->base_baudrate / (16 * baudrate)) >> 8);
	
	driver->baudrate = baudrate;	/* 保存当前的波特率 */
	
	/* 还原LCR */
	to_serial(driver, SERIAL_LCR, driver->regs.LCR);	/* 设置波特率非能 */
	
	int_unlock(level);	/* 开中断 */
	
	return 0;
}


/******************************************************************************
 * 
 * 设置串口LCR
 * 
 * 输入：
 * 		driver：串口驱动描述结构
 * 		lcr：LCR值
 * 输出：
 * 		无
 * 返回：
 * 		成功返回0；失败返回-1 
 */
static int set_lcr(struct serial_device_struct_ex *driver, unsigned int lcr)
{
	int new_lcr = 0x0;
	int level;
	int cur_lcr;
	
	level = int_lock();
	/* 关中断 */
	to_serial(driver, SERIAL_IER, 0x0);
	
	/* 设置数据位，默认8位 */
	cur_lcr = lcr & 0xf;
	if (cur_lcr == CS5)
	{
		new_lcr |= SERIAL_LCR_S5;
	}
	else if (cur_lcr == CS6)
	{
		new_lcr |= SERIAL_LCR_S6;
	}
	else if (cur_lcr == CS7)
	{
		new_lcr |= SERIAL_LCR_S7;
	}
	else	/* 如果参数错误，则设置为默认值CS8 */
	{
		new_lcr |= SERIAL_LCR_S8;
	}
	
	/* 停止位；1个停止位默认，不需判断 */
	cur_lcr = lcr & 0xf0;
	if (cur_lcr == STOP2)
	{
		new_lcr |= SERIAL_LCR_STOP2;
	}
	
	/* 校验；无校验默认，不需判断 */
	cur_lcr = lcr & 0xf00;
	if (cur_lcr == PARODD)
	{
		new_lcr |= SERIAL_LCR_PARITY;
	}
	else if (cur_lcr == PAREVEN)
	{
		new_lcr |= (SERIAL_LCR_PARITY | SERIAL_LCR_EVEN);
	}
	
	/* 备份 */
	driver->regs.LCR = new_lcr;
	
	/* 设置 */
	to_serial(driver, SERIAL_LCR, driver->regs.LCR);
	to_serial(driver, SERIAL_IER, driver->regs.IER);
	
	int_unlock(level);
	
	return 0;
}


/******************************************************************************
 * 
 * 接收串口字符接口
 * 
 * 		本程序用于以while方式接收串口数据，并写至规则库缓冲区，直至不再收到数据。
 * 
 * 输入：
 * 		driver：串口描述结构指针
 * 		lsr_status：LSR寄存器值
 * 输出：
 * 		无
 * 返回：
 * 		无
 */
inline int rx_in_isr(struct serial_device_struct_ex *driver, 
					 int *lsr_status)
{
	char c;
	do
	{
		/* Rx异常情况统计 */
		if (*lsr_status & (SERIAL_LSR_OE | 
						  SERIAL_LSR_PE | 
						  SERIAL_LSR_FE |
						  SERIAL_LSR_BI))
		{
			if (SERIAL_LSR_OE & *lsr_status)		/* 溢出 */
			{	
				driver->statistic.oe++;
			}
			else if (SERIAL_LSR_PE & *lsr_status)	/* 校验 */
			{
				driver->statistic.pe++;
			}
			else if (SERIAL_LSR_FE & *lsr_status)	/* 帧错误 */
			{
				driver->statistic.fe++;
			}
			else if (SERIAL_LSR_BI & *lsr_status)	/* break */
			{
				driver->statistic.bi++;
			}
		}
		else
		{
			c = from_serial(driver, SERIAL_RBR);	/* 取接收的数据 */			
			write_char_to_tty((struct tty_driver *)driver, c);/* 写至规则库 */	
			driver->statistic.rx++;					/* 统计收字符 */
		}		
		
		*lsr_status = from_serial(driver, SERIAL_LSR);
	}
	while (SERIAL_LSR_RXV & *lsr_status);
	
	return 0;
}


/******************************************************************************
 * 
 * 向串口发送字符接口
 * 
 * 		本程序用于以while方式向串口发送数据，直至规则库缓冲区为空。
 * 
 * 输入：
 * 		driver：串口描述结构指针
 * 		lsr_status：LSR寄存器值
 * 输出：
 * 		无
 * 返回：
 * 		无
 */
inline int tx_in_isr(struct serial_device_struct_ex *driver, 
					 int *lsr_status)
{
	char c;	
	do
	{
		/* 读取字符 */
		if (-1 != read_char_from_tty((struct tty_driver *)driver, &c))
		{
			to_serial(driver, SERIAL_THR, c);	/* 写至发送寄存器 */
			*lsr_status = from_serial(driver, SERIAL_LSR);	/* 再次读LSR */
			driver->statistic.tx++;
		}
		else
		{
			driver->regs.IER &= ~SERIAL_IER_THEN;	/* 屏蔽Tx中断位，保存至备份 */
			to_serial(driver, SERIAL_IER, driver->regs.IER);	/* 设置IER寄存器 */
			break;	/* 退出while循环 */
		}			
	}
	while (SERIAL_LSR_THRE & *lsr_status);

	return 0;
}


/******************************************************************************
 * 
 * 串口中断服务程序ISR
 * 
 * 		本程序在串口初始化代码中调用，通过int_handle_install接口实现ISR与中断的挂接。
 * 本程序实现了Rx和Tx两类中断的处理，尚未实现Modem中断和错误中断的处理。
 * 
 * 输入：
 * 		driver：串口描述结构指针
 * 输出：
 * 		无
 * 返回：
 * 		无
 */
static void *serial_int_isr2(void *arg)
{  
	struct serial_device_struct_ex *driver = (struct serial_device_struct_ex *)arg;
	int lsr_status;
	int iir_status;
	
	while (1)
	{
		/* 等待isr1释放信号量 */
		sem_wait2(&driver->isr_sem, WAIT, NO_TIMEOUT);		
		
		iir_status = from_serial(driver, SERIAL_IIR) &0xff;	/* 取IIR */
		if (SERIAL_IIR_NO_INT & iir_status)		/* 无中断发生 */
		{
			continue;
		}
		
		lsr_status = from_serial(driver, SERIAL_LSR);	/* 取LSR */
		
		/* 接收Rx */
		if (SERIAL_LSR_RXV & lsr_status)
		{
			rx_in_isr(driver, &lsr_status);	/* 内部子函数调用 */
		}
		
		/* Tx */
		if ((SERIAL_LSR_THRE & lsr_status) || ((iir_status & 0x06) == SERIAL_IIR_THRI))
		{
			tx_in_isr(driver, &lsr_status);	/* 内部子函数调用 */
		}			
	}

	return NULL;
}


/*******************************************************************************
 * 
 * 中断服务程序阶段一
 * 
 * 		本程序用于释放ISR信号量，以启动ISR2执行。本程序由中断框架触发。
 * 
 * 输入：
 * 		driver：串口设备描述结构指针
 * 输出：
 * 		无
 * 返回：
 * 		无
 */
void serial_int_isr1(struct serial_device_struct_ex *driver)
{
//	printk("==========================\n");
//	sem_post(&driver->isr_sem);
#if 1
	int lsr_status;
	int iir_status;
	
	
		
	iir_status = from_serial(driver, SERIAL_IIR) &0xff;	/* 取IIR */
	if (SERIAL_IIR_NO_INT & iir_status)		/* 无中断发生 */
	{
		return;
	}
	
	lsr_status = from_serial(driver, SERIAL_LSR);	/* 取LSR */
	
	/* 接收Rx */
	if (SERIAL_LSR_RXV & lsr_status)
	{
		rx_in_isr(driver, &lsr_status);	/* 内部子函数调用 */
	}
	
	/* Tx */
	if ((SERIAL_LSR_THRE & lsr_status) || ((iir_status & 0x06) == SERIAL_IIR_THRI))
	{
		tx_in_isr(driver, &lsr_status);	/* 内部子函数调用 */
	}			

#endif
	return;
}

/******************************************************************************
 * 
 * 串口设备打开接口
 * 
 * 输入：
 * 		driver：串口设备描述结构指针
 * 输出：
 * 		无
 * 返回：
 * 		成功返回0；失败返回-1
 * 
 */
int serial_open(struct tty_driver *driver)
{
	struct serial_device_struct_ex *serial_driver;
	
	if (NULL == driver)
	{
		return -1;
	}
	serial_driver = (struct serial_device_struct_ex *)driver;
	
	/* 如果计数为0，则说明第一次打开串口设备，因而需使能中断 */
	if (0 == serial_driver->count)
	{
		int_enable_pic(serial_driver->intvec);
	}
	
	serial_driver->count++;
	
	return 0;
}

/******************************************************************************
 * 
 * 串口设备关闭接口
 * 
 * 输入：
 * 		driver：串口设备描述结构指针
 * 输出：
 * 		无
 * 返回：
 * 		成功返回0；失败返回-1
 * 
 */
int serial_close(struct tty_driver *driver)
{
	int level;
	struct serial_device_struct_ex *serial_driver = 
		(struct serial_device_struct_ex *)driver;	
	
	level = int_lock();
	
	serial_driver->count--;	
	if (0 > serial_driver->count)
	{
		serial_driver->count = 0;
		int_unlock(level);
		return 0;
	}
	
	if (0 == serial_driver->count)
	{
		int_disable_pic(serial_driver->intvec);
	}
	
	int_unlock(level);
	
	return 0;
}



/******************************************************************************
 * 
 * 启动一次发送接口
 * 
 * 		本程序负责启动一次发送，本程序在上层TTY规则库中调用。当应用通过IO的write接口
 * 经由规则库向串口写数据时，规则库调用本接口启动一次发送。本程序负责通过Tx中断使能。
 * 实现ISR的调用。
 * 
 * 输入：
 * 		driver：串口设备描述结构指针
 * 输出：
 * 		无
 * 返回：
 * 		成功返回0；失败返回-1
 */
int serial_start_tx(struct tty_driver *driver)
{	
	struct serial_device_struct_ex *serial_driver = 
		(struct serial_device_struct_ex *)driver;
	
	/* Tx中断使能，并备份到IER */
	serial_driver->regs.IER |= SERIAL_IER_THEN;
	
	/* 使能Tx中断 */
	to_serial(serial_driver, SERIAL_IER, serial_driver->regs.IER);
	
	return 0;
}


/******************************************************************************
 * 
 * 停止发送接口
 * 
 * 		本程序负责停止发送，本程序在上一层规则库中调用。本程序调用的时机是上层规则库
 * 中无等待传输的字符。本程序负责通过Tx非能停止字符发送。
 * 		如果应用程序配置了自动发送控制选项IOS_LDISC_TX_AUTO，那么规则库在无待传输的
 * 字符时自动调用本接口，当然，如果没有配置该选项，那么可以在ISR中停止发送。如果没有
 * 定义或实现本程序，那么自动发送控制选项不起作用，无论是否配置该选项。
 * 
 * 输入：
 * 		driver：串口设备描述结构指针
 * 输出：
 * 		无
 * 返回：
 * 		成功返回0；失败返回-1
 */
int serial_stop_tx(struct tty_driver *driver)
{	
	struct serial_device_struct_ex *serial_driver = 
		(struct serial_device_struct_ex *)driver;
	
	/* Tx中断非能，并备份到IER */
	serial_driver->regs.IER |= ~SERIAL_IER_THEN;
	
	/* Tx中断非能 */
	to_serial(serial_driver, SERIAL_IER, serial_driver->regs.IER);
	
	return 0;
}


/******************************************************************************
 * 
 * 启动接收接口
 * 
 * 		本程序负责启动接收，本程序在上一层规则库中调用。
 * 
 * 输入：
 * 		driver：串口设备描述结构指针
 * 输出：
 * 		无
 * 返回：
 * 		成功返回0；失败返回-1
 * 备注：
 * 		本程序尚未实现调用。
 */
int serial_start_rx(struct tty_driver *driver)
{	
	struct serial_device_struct_ex *serial_driver = 
		(struct serial_device_struct_ex *)driver;
	
	/* Rx使能，并备份到IER */
	serial_driver->regs.IER |= (SERIAL_IER_RBEN | SERIAL_IER_RLSEN);
	
	/* 使能Rx中断 */
	to_serial(serial_driver, SERIAL_IER, serial_driver->regs.IER);
	
	return 0;
}


/******************************************************************************
 * 
 * 停止接收接口
 * 
 * 		本程序负责停止接收，本程序在上一层规则库中调用。
 * 
 * 输入：
 * 		driver：串口设备描述结构指针
 * 输出：
 * 		无
 * 返回：
 * 		成功返回0；失败返回-1
 * 备注：
 * 		本程序尚未实现调用。
 */
int serial_stop_rx(struct tty_driver *driver)
{	
	struct serial_device_struct_ex *serial_driver = 
		(struct serial_device_struct_ex *)driver;
	
	/* Rx非能，并备份到IER */
	serial_driver->regs.IER |= ~(SERIAL_IER_RBEN | SERIAL_IER_RLSEN);
	
	/* Rx非能 */
	to_serial(serial_driver, SERIAL_IER, serial_driver->regs.IER);
	
	return 0;
}


/******************************************************************************
 * 
 * 发送字符接口
 * 
 * 		本程序通过直接写TRH寄存器向串口输出一个字符。
 * 
 * 输入：
 * 		driver：串口设备描述结构指针
 * 		c：待发送的字符
 * 输出：
 * 		无
 * 返回：
 * 		成功返回0；失败返回-1
 */
int serial_tx_char(struct tty_driver *driver, char c)
{
	struct serial_device_struct_ex *serial_driver = 
		(struct serial_device_struct_ex *)driver;
	
	/* 输出字符THR */
	if (SERIAL_LSR_THRE & from_serial(serial_driver, SERIAL_LSR))
	{
		to_serial(serial_driver, SERIAL_THR, c);		
		return 0;
	}
	
	return -1;
}


/******************************************************************************
 * 
 * 接收字符接口
 * 
 * 		本程序通过直接读RDH寄存器向调用者输出一个字符。
 * 
 * 输入：
 * 		driver：串口设备描述结构指针
 * 输出：
 * 		c：存放接收到字符的字节类型指针
 * 返回：
 * 		成功返回0；失败返回-1
 */
int serial_rx_char(struct tty_driver *driver, char *c)
{
	struct serial_device_struct_ex *serial_driver = 
		(struct serial_device_struct_ex *)driver;
	
	/* 读取RDH寄存器，将寄存器内容写至c */
	if (SERIAL_LSR_RXV & from_serial(serial_driver, SERIAL_LSR))
	{
		*c = from_serial(serial_driver, SERIAL_RBR);
		return 0;
	}	
	
	return -1;	
}


/******************************************************************************
 * 
 * 串口设备控制接口
 * 
 * 输入：
 * 		driver：串口设备描述结构指针
 * 		u32：控制命令
 * 		arg：命令参数
 * 输出：
 * 返回：
 * 		成功返回0；失败返回-1
 */
int serial_ioctl(struct tty_driver *driver, u32 cmd, void *arg)
{
	struct serial_device_struct_ex *serial_driver = 
		(struct serial_device_struct_ex *)driver;
	
	int status = 0;
	int tmp;	/* 临时变量 */
	
	switch (cmd)
	{
		case TTY_SET_BAUDRATE :	/* 设置波特率 */
		{
			tmp = (int)arg;
			/* 对波特率进行检查 */
			if ((50 > tmp) || (115200 < tmp))
			{
				return -1;
			}
			status = set_baudrate(serial_driver, tmp);
			break;
		}
		case TTY_GET_BAUDRATE :	/* 获取波特率 */
		{
			if(!arg)
			{
				return -1;
			}
			*(int *)arg = serial_driver->baudrate;
			break;
		}
		case TTY_SET_MODE :		/* 设置访问模式 */
		{
			if (INT_MODE == (int)arg)
			{
				serial_driver->regs.IER = (SERIAL_IER_RBEN | SERIAL_IER_RLSEN);
				to_serial(serial_driver, SERIAL_IER, serial_driver->regs.IER);
				return 0;			
			}
			else if (POLL_MODE == (int)arg)
			{
				serial_driver->regs.IER = 0x0;
				to_serial(serial_driver, SERIAL_IER, serial_driver->regs.IER);
				return 0;
			}
			else
			{
				return -1;
			}
			break;
		}
		case TTY_GET_MODE :		/* 获取访问模式 */
		{
			if (serial_driver->regs.IER & (SERIAL_IER_RBEN | SERIAL_IER_RLSEN))
			{
				*(int *)arg = INT_MODE;
				return 0;
			}
			else
			{
				*(int *)arg = POLL_MODE;
				return 0;
			}
			
			break;
		}
		case TTY_SERIAL_LCR :		/* 设置LCR寄存器 */
		{
			tmp = (int)arg;
			set_lcr(serial_driver, tmp);
			break;
		}
		default :
		{
			status = -1;
			break;
		}
	}
	
	return status;
}

/*******************************************************************************
 * 
 * 以轮询的方式从串口设备读取数据
 * 
 * 输入：
 * 		serial_num：访问的串口号
 * 		count：待读取的数据大小
 * 输出：
 * 		buffer：存放待读取数据的缓冲区
 * 返回：
 * 		成功返回读取的字符个数；失败返回-1
 */
#if 0
int tx_in_poll(int serial_num, const char *buffer, int count)
{
	int writed = 0;		/* 已写入的字符计数 */
	
	/* 参数检查 */
	if ((0 > serial_num) || (SERIAL_NUMBER <= serial_num))
	{
		return -1;
	}
	
	/* Tx数据 */
	while (count)
	{
		while (-1 == serial_tx_char(
				(struct tty_driver *)&global_serial_struct[serial_num],
				buffer[writed]))
		{
			;
		}		
		count--;
		writed++;
	}
	
	(0 == writed) ? ({return -1;}) : ({return writed;});
}
#else
int tx_in_poll(int num, const char *buf, int count)
{
	int writed = 0;		/* 已写入的字符计数  Afrit modified 20151202*/
	char *p = (char *)buf;
	while(count--)
	{
		while (!(readb(UART_REG (LSR, num)) & LSR_THRE));
		writeb(*p, UART_REG (THR, num));
		p++;
		writed++;
	}
//	return 0;
	(0 == writed) ? ({return -1;}) : ({return writed;});
}
#endif

/*******************************************************************************
 * 
 * 以轮询的方式从串口设备读取数据
 * 
 * 输入：
 * 		serial_num：访问的串口号
 * 		count：待读取的数据大小
 * 输出：
 * 		buffer：存放待读取数据的缓冲区
 * 返回：
 * 		成功返回读取的字符个数；失败返回-1
 */
#if 0
int rx_in_poll(int serial_num, char *buffer, int count)
{	
	int readed = 0;		/* 已读取的字符个数 */
	
	/* 参数检查 */
	if ((0 > serial_num) || (SERIAL_NUMBER <= serial_num) ||
		(NULL == buffer))
	{
		return -1;
	}
	
	/* 关闭Rx中断 */
	
	
	/* Rx数据 */
	while (count)
	{
		if (-1 == serial_rx_char(
				(struct tty_driver *)&global_serial_struct[serial_num], 
				&buffer[readed]))
		{
			if (readed)
			{
				goto result;
			}
			else
			{
				continue;
			}
		}		
		count--;
		readed++;
	}
	
result:
	(0 == readed) ? ({return -1;}) : ({return readed;});
}
#else

int rx_in_poll(int num, char *buf, int count)
{
	int read_cnt = 0;
	
	while(1) 
	{
		int timeout = 10000;
		while( !(readb(UART_REG(LSR, num)) & SERIAL_LSR_RXV) )
		{
			if (0 == timeout--)
			{
				goto cannot_rx;
			}
		}
		
		*buf++ = readb(UART_REG(RBR, num));
		read_cnt++;
		if (read_cnt == count) 
		{
			break;
		}
		
		continue;
		
cannot_rx:
		if (read_cnt) 
		{
			break;
		}
		
	}
	return read_cnt;
}

#endif

/******************************************************************************
 * 
 * 初始化串口硬件设备
 * 
 * 输入：
 * 		driver：串口设备描述结构指针
 * 输出：
 * 返回：
 * 		成功返回0；失败返回-1
 */
int serial_hw_init(struct serial_device_struct_ex *driver)
{	
	int level;
	
	level = int_lock();
	
	/* IER, LCR, FCR清除 */
	to_serial(driver, SERIAL_LCR, 0);		
	to_serial(driver, SERIAL_FCR, 0);			
	to_serial(driver, SERIAL_IER, 0);	
	
	/* IER */
	to_serial(driver, SERIAL_IER, 0);	/* 清中断 */
	driver->regs.IER = 0;				/* 初始状态不采用中断访问模式 */
	
	/* modem控制寄存器，是否需要??? */
	driver->regs.MCR = 0;
	driver->regs.MCR |= SERIAL_MCR_OUT2;
	driver->regs.MCR &= (~(SERIAL_MCR_DTR | SERIAL_MCR_RTS));
	to_serial(driver, SERIAL_MCR, driver->regs.MCR);
	
	/* 设置FIFO，使能、收清、读清 */
	driver->regs.FCR = SERIAL_FCR_EN | 
					   SERIAL_FCR_RXCLR | 
					   SERIAL_FCR_TXCLR;
	
	/* LCR */
	set_lcr(driver, (CS8 | STOP1 | DENBPAR));
	
	/* FCR */
	to_serial(driver, SERIAL_FCR, driver->regs.FCR); 
	
	set_baudrate(driver, driver->baudrate);
	
	/* IER中断使能 */
	to_serial(driver, SERIAL_IER, driver->regs.IER);
	
	int_unlock(level);
	
	return 0;
}



/******************************************************************************
 * 
 * 初始化串口接口
 * 
 * 		本程序 主要完成串口设备硬件初始化。本程序完成的功能主要分为两个方面：分配扩展
 * 的串口设备描述结构，对与初始化硬件相关的描结构属性进行赋值。
 * 
 * 输入：
 * 		无
 * 输出：
 * 		无
 * 返回：
 * 		成功返回0；失败返回-1
 * 备注：
 * 		本接口执行完毕后，可保证轮询访问串口的接口rx/tx_in_poll可用。
 */
int serial_device_init(void)
{
	int i;	/* 循环变量 */
	
	/* 初始化标识 */
	static int serial_device_init_flag = 0;
	if (serial_device_init_flag)
	{
		return 0;
	}
	
	for (i = 0; i < SERIAL_NUMBER; i++)	
	{
		struct serial_device_struct_ex *driver = &global_serial_struct[i];
		memset(driver, 0, sizeof(struct serial_device_struct_ex));
		
		/* 扩展结构体属性设置 */
		driver->base_baudrate			= serial_param_define[i].base_baudrate;
		driver->baudrate				= serial_param_define[i].baudrate;
		driver->regs_base				= (u8 *)serial_param_define[i].regs_base;
		driver->delta					= serial_param_define[i].delta;		
		driver->count					= 0;
		driver->intvec					= serial_param_define[i].intvec;
		
		/* 初始化串口硬件 */
		serial_hw_init(driver);
	}
	
	/* 初始化标识 */
	serial_device_init_flag = 1;
	
	return 0;
}


/*******************************************************************************
 * 
 * 注册串口设备
 * 
 * 		本程序用于向TTY层注册串口设备。本程序的主要工作为：对struct tty_driver的操作
 * 赋值、申请本设备所用的次设备号、挂接中断、注册串口设备。
 * 		
 * 
 * 输入：
 * 		无
 * 输出：
 * 		无
 * 返回：
 * 		成功返回0；失败返回-1
 * 备注：
 * 		本接口执行完毕后，可用过标准I/O接口访问串口设备。
 */
int serial_device_register(void)
{
	int i;			/* 循环变量 */
	int retn;		/* 临时返回值 */
	dev_t 	minor;
	
	/* 初始化标识 */
	static int serial_device_register_flag = 0;
	if (serial_device_register_flag)
	{
		return 0;
	}
	
	for (i = 0; i < SERIAL_NUMBER; i++)
	{	
		struct serial_device_struct_ex *driver = &global_serial_struct[i];
		
		struct tty_driver *base_driver = 
			(struct tty_driver *)&(driver->driver);
		
		/* 操作赋值 */
		base_driver->open				= (TTY_OPEN)serial_open;
		base_driver->close				= (TTY_CLOSE)serial_close;
		base_driver->start_tx 			= (TTY_START_TX)serial_start_tx;
		base_driver->stop_tx 			= (TTY_STOP_TX)serial_stop_tx;
		base_driver->start_rx			= (TTY_START_RX)serial_start_rx;
		base_driver->stop_rx			= (TTY_STOP_RX)serial_stop_rx;
		base_driver->tx_char 			= (TTY_TX_CHAR)serial_tx_char;
		base_driver->rx_char 			= (TTY_RX_CHAR)serial_rx_char;
		base_driver->ioctl 				= (TTY_IOCTL)serial_ioctl;	
		
		/* 申请次设备号 */
		if (0 != chrdev_minor_request(TTY_MAJOR, TTY_SERIAL_START, 1, TTY_TERM_START -1 , &minor))
		{
			printk("[ERROR] serial driver cannot request minor\n");
			return -1;
		}		
		base_driver->dev	 			= MKDEV(TTY_MAJOR, minor);		/* 设备号 */
		
		/* 注册设备 */
		register_tty_device(serial_param_define[i].ldisc_name, 
							serial_param_define[i].registered_path, 
							base_driver);	
		
//		printk("%s %d Register serial isr handler:serial_int_isr1|%d(DUART%d) \n",__func__,__LINE__,serial_param_define[i].intvec,i);
		/* 挂接中断 */	
		/* ISR名称、中断向量、优先级、ISR、ISR参数 */
		int_install_handler(serial_param_define[i].intvec, 
							(void (*)(void *))serial_int_isr1, 
							(void *)driver);
		
		/* 设置中断模式 */
		driver->regs.IER = (SERIAL_IER_RBEN | SERIAL_IER_RLSEN);/* 收行状态中断、收中断 */
		driver->regs.IER &= ~SERIAL_IER_MSEN;	/* 屏蔽modem中断，此处设置无用，后删 */
		to_serial(driver, SERIAL_IER, driver->regs.IER);
		
		/* 为ISR创建二进制信号量（初始值为0） */
		sem_init2(&driver->isr_sem, 0, SEM_BINARY, PTHREAD_WAITQ_FIFO, 0);
		
		/* 创建ISR2线程 */
		retn = pthread_create2(&driver->isr_thread, 					/* ID */
							   strrchr(serial_param_define[i].registered_path, '/'),	/* 名称 */
							   220,										/* 优先级 */
							   RE_UNBREAKABLE | RE_FP_TASK,				/* 选项 */
							   0x8000,									/* 栈 */
							   (void*(*)(void *))serial_int_isr2,		/* 入口 */
							   (void *)driver);							/* 参数 */
		if (0 != retn)
		{
			printk("[ERROR] cannot create thread\n");
	
			return -1;
		}
	}
	
	/* 初始化标识 */
	serial_device_register_flag = 1;
	
	return 0;
}


/*******************************************************************************
 * 
 * 串口模块初始化接口
 * 
 * 输入：
 * 		无
 * 输出：
 * 		无
 * 返回：
 * 		无
 * 修改：
 * 		2011-11-11，删掉单独初始化串口模块的文件，将该函数合并至此
 */
int serial_module_init(void)
{
	
	/* 初始化串口驱动所需的规则库 */
	line_ldisc_init();
	
	/* 初始化串口设备 */
	serial_device_init();
	
	/* 注册串口设备 */
	serial_device_register();
	
	/* 如果定义串口作为标准I/O */
	int fd = open("/dev/serial0", O_RDWR, 0777);
	
	ioctl(fd, TTY_SET_OPTIONS, TTY_OUT_CR2NL | TTY_IN_ECHO);
	
	global_std_set(0, fd);
	global_std_set(1, fd);
	global_std_set(2, fd);
	
	return 0;
}

