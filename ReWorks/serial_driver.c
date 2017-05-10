/******************************************************************************
 * 
 * ��Ȩ��
 * 		�й����ӿƼ����Ź�˾����ʮ���о���
 * ������
 * 		���ļ�ʵ����x86ƽ̨ns16550A��������
 * �޸ģ�
 * 		2012-05-10�������������޸�struct tty_driver���޸Ĵ�������
 * 		2012-05-05���������������Ӵ��ڽṹ��ļ������ԣ����ж�ʹ�ܷ���open��close��
 * 		2011-11-18�����������޸�ISR��ʽ����ISR��Ϊ�����׶�ʵ��
 * 		2011-10-22�����������޸Ĵ���������ӿ�����
 * 		2011-09-28����־�꣬�޸�poll��д�ӿ�����
 * 		2011-09-26��������������poll��ʽ���ʴ��ڣ���дISR
 * 		2011-09-16���������������Բ������޸�ioctl()�ӿڡ�tx_char()�ӿں�rx_char()
 * 							�ӿ�ioctl_define.h��regs.h�ļ�������set_lcr()�ӿ�
 * 		2011-08-30����������ͳһ�Ĵ�������
 * 		2011-08-27��������������
 */
#include <pthread.h>			/* �߳� */
#include <irq.h>				/* �жϹҽ� */
#include <string.h>				/* C */
#include <reworks/printk.h>		/* printk��ӡ */
#include <io_asm.h>				/* �˿ڷ��� */
#include <driver.h>
#include "serial_driver.h"		/* ��������ͷ�ļ� */
#include <mpc8377.h>
#include <openpic.h>
/*==============================================================================
 * 
 * ȫ�ֱ���
 * 
 */
/**
 * ���ڲ�������
 * 	
 * 	�˴�����Ĵ��ڲ����ڴ����豸��ʼ����ע��׶�ʹ��
 */
struct serial_params serial_param_define[SERIAL_NUMBER] =
{		/* ���������     ע����豸·��   �Ĵ����� �Ĵ������  �ж�����   ��׼������  ������ */
		{LINE_LDISC_NAME, "/dev/serial0", SERIAL_BASE(0), 1, OPENPIC_VEC_UART1, CSB_CLK_DEFAULT, 115200},
		{LINE_LDISC_NAME, "/dev/serial1", SERIAL_BASE(1), 1, OPENPIC_VEC_UART2, CSB_CLK_DEFAULT, 115200}
};

/* ���������ṹȫ�ֱ��� */
struct serial_device_struct_ex global_serial_struct[SERIAL_NUMBER];

#define readb(addr) 		(*(volatile unsigned char *) (addr))
#define writeb(b, addr) 	(*(volatile unsigned char *) (addr) = (b))

#define RBR				0x00		/* ���ջ���Ĵ��� */
#define THR				0x00		/* transmit holding register */
#define LSR				0x05		/* line ״̬�Ĵ��� */
#define LSR_THRE		0x20		/* transmit holding register empty */

/******************************************************************************
 * 
 * ��ȡ�����豸�Ĵ���ֵ
 * 
 * ���룺
 * 		driver�������豸�����ṹָ��
 * 		offset��ƫ��
 * �����
 * 		��
 * ���أ�
 * 		�Ĵ���ֵ
 * ��ע��
 * 		���ڲ�ͬӲ��ƽ̨���ʴ��ڼĴ����ķ�ʽ��ͬ���������������ʹ�ýӿ�from/to_serial
 * �Է��ʴ��ڵķ�ʽ���з�װ��������ݲ�ͬƽ̨���ص㣬�޸�from/to_serial��ʵ�֡�
 */
static inline unsigned int from_serial(struct serial_device_struct_ex *driver, 
									   int offset)
{

	/* �ڴ��ַ��ƫ�ƣ�ƫ��Ϊ���Ĵ���ƫ�� + �Ĵ����ֽڿ�� */	
	return readb((int)(driver->regs_base + (offset * driver->delta)));
}


/******************************************************************************
 * 
 * �򴮿��豸�Ĵ���д������
 * 
 * ���룺
 * 		driver�������豸�����ṹָ��
 * 		offset��ƫ��
 * 		value��д��Ĵ�����ֵ
 * �����
 * 		��
 * ���أ�
 * 		�Ĵ���ֵ
 */
static inline void to_serial(struct serial_device_struct_ex *driver, 
							 int offset, 
							 int value)
{
	/* �ڴ��ַ��ƫ�ƣ�ƫ��Ϊ���Ĵ���ƫ�� + �Ĵ����ֽڿ�� */
	writeb(value, (int)(driver->regs_base + (offset * driver->delta)));

	return;
}



/******************************************************************************
 * 
 * ���ò����ʽӿڣ�������Ϊ�ڲ��ӿ�
 * 
 * ���룺
 * 		driver��
 * �����
 * 		��
 * ���أ�
 * 		�Ĵ���ֵ
 */
static int set_baudrate(struct serial_device_struct_ex *driver, 
						unsigned int baudrate)
{
	int level;
	
	level = int_lock();	/* ���ж� */
	
	/* ����LCR��ʹ�õ�ǰ�ܹ����ò����� */
	to_serial(driver, SERIAL_LCR, driver->regs.LCR | SERIAL_LCR_DLAB);
	
	/* ���ò����ʵ͡��߷�Ƶϵ�� */
	to_serial(driver, SERIAL_DLL, (driver->base_baudrate / (16 * baudrate)));
	to_serial(driver, SERIAL_DLM, (driver->base_baudrate / (16 * baudrate)) >> 8);
	
	driver->baudrate = baudrate;	/* ���浱ǰ�Ĳ����� */
	
	/* ��ԭLCR */
	to_serial(driver, SERIAL_LCR, driver->regs.LCR);	/* ���ò����ʷ��� */
	
	int_unlock(level);	/* ���ж� */
	
	return 0;
}


/******************************************************************************
 * 
 * ���ô���LCR
 * 
 * ���룺
 * 		driver���������������ṹ
 * 		lcr��LCRֵ
 * �����
 * 		��
 * ���أ�
 * 		�ɹ�����0��ʧ�ܷ���-1 
 */
static int set_lcr(struct serial_device_struct_ex *driver, unsigned int lcr)
{
	int new_lcr = 0x0;
	int level;
	int cur_lcr;
	
	level = int_lock();
	/* ���ж� */
	to_serial(driver, SERIAL_IER, 0x0);
	
	/* ��������λ��Ĭ��8λ */
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
	else	/* �����������������ΪĬ��ֵCS8 */
	{
		new_lcr |= SERIAL_LCR_S8;
	}
	
	/* ֹͣλ��1��ֹͣλĬ�ϣ������ж� */
	cur_lcr = lcr & 0xf0;
	if (cur_lcr == STOP2)
	{
		new_lcr |= SERIAL_LCR_STOP2;
	}
	
	/* У�飻��У��Ĭ�ϣ������ж� */
	cur_lcr = lcr & 0xf00;
	if (cur_lcr == PARODD)
	{
		new_lcr |= SERIAL_LCR_PARITY;
	}
	else if (cur_lcr == PAREVEN)
	{
		new_lcr |= (SERIAL_LCR_PARITY | SERIAL_LCR_EVEN);
	}
	
	/* ���� */
	driver->regs.LCR = new_lcr;
	
	/* ���� */
	to_serial(driver, SERIAL_LCR, driver->regs.LCR);
	to_serial(driver, SERIAL_IER, driver->regs.IER);
	
	int_unlock(level);
	
	return 0;
}


/******************************************************************************
 * 
 * ���մ����ַ��ӿ�
 * 
 * 		������������while��ʽ���մ������ݣ���д������⻺������ֱ�������յ����ݡ�
 * 
 * ���룺
 * 		driver�����������ṹָ��
 * 		lsr_status��LSR�Ĵ���ֵ
 * �����
 * 		��
 * ���أ�
 * 		��
 */
inline int rx_in_isr(struct serial_device_struct_ex *driver, 
					 int *lsr_status)
{
	char c;
	do
	{
		/* Rx�쳣���ͳ�� */
		if (*lsr_status & (SERIAL_LSR_OE | 
						  SERIAL_LSR_PE | 
						  SERIAL_LSR_FE |
						  SERIAL_LSR_BI))
		{
			if (SERIAL_LSR_OE & *lsr_status)		/* ��� */
			{	
				driver->statistic.oe++;
			}
			else if (SERIAL_LSR_PE & *lsr_status)	/* У�� */
			{
				driver->statistic.pe++;
			}
			else if (SERIAL_LSR_FE & *lsr_status)	/* ֡���� */
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
			c = from_serial(driver, SERIAL_RBR);	/* ȡ���յ����� */			
			write_char_to_tty((struct tty_driver *)driver, c);/* д������� */	
			driver->statistic.rx++;					/* ͳ�����ַ� */
		}		
		
		*lsr_status = from_serial(driver, SERIAL_LSR);
	}
	while (SERIAL_LSR_RXV & *lsr_status);
	
	return 0;
}


/******************************************************************************
 * 
 * �򴮿ڷ����ַ��ӿ�
 * 
 * 		������������while��ʽ�򴮿ڷ������ݣ�ֱ������⻺����Ϊ�ա�
 * 
 * ���룺
 * 		driver�����������ṹָ��
 * 		lsr_status��LSR�Ĵ���ֵ
 * �����
 * 		��
 * ���أ�
 * 		��
 */
inline int tx_in_isr(struct serial_device_struct_ex *driver, 
					 int *lsr_status)
{
	char c;	
	do
	{
		/* ��ȡ�ַ� */
		if (-1 != read_char_from_tty((struct tty_driver *)driver, &c))
		{
			to_serial(driver, SERIAL_THR, c);	/* д�����ͼĴ��� */
			*lsr_status = from_serial(driver, SERIAL_LSR);	/* �ٴζ�LSR */
			driver->statistic.tx++;
		}
		else
		{
			driver->regs.IER &= ~SERIAL_IER_THEN;	/* ����Tx�ж�λ������������ */
			to_serial(driver, SERIAL_IER, driver->regs.IER);	/* ����IER�Ĵ��� */
			break;	/* �˳�whileѭ�� */
		}			
	}
	while (SERIAL_LSR_THRE & *lsr_status);

	return 0;
}


/******************************************************************************
 * 
 * �����жϷ������ISR
 * 
 * 		�������ڴ��ڳ�ʼ�������е��ã�ͨ��int_handle_install�ӿ�ʵ��ISR���жϵĹҽӡ�
 * ������ʵ����Rx��Tx�����жϵĴ�����δʵ��Modem�жϺʹ����жϵĴ���
 * 
 * ���룺
 * 		driver�����������ṹָ��
 * �����
 * 		��
 * ���أ�
 * 		��
 */
static void *serial_int_isr2(void *arg)
{  
	struct serial_device_struct_ex *driver = (struct serial_device_struct_ex *)arg;
	int lsr_status;
	int iir_status;
	
	while (1)
	{
		/* �ȴ�isr1�ͷ��ź��� */
		sem_wait2(&driver->isr_sem, WAIT, NO_TIMEOUT);		
		
		iir_status = from_serial(driver, SERIAL_IIR) &0xff;	/* ȡIIR */
		if (SERIAL_IIR_NO_INT & iir_status)		/* ���жϷ��� */
		{
			continue;
		}
		
		lsr_status = from_serial(driver, SERIAL_LSR);	/* ȡLSR */
		
		/* ����Rx */
		if (SERIAL_LSR_RXV & lsr_status)
		{
			rx_in_isr(driver, &lsr_status);	/* �ڲ��Ӻ������� */
		}
		
		/* Tx */
		if ((SERIAL_LSR_THRE & lsr_status) || ((iir_status & 0x06) == SERIAL_IIR_THRI))
		{
			tx_in_isr(driver, &lsr_status);	/* �ڲ��Ӻ������� */
		}			
	}

	return NULL;
}


/*******************************************************************************
 * 
 * �жϷ������׶�һ
 * 
 * 		�����������ͷ�ISR�ź�����������ISR2ִ�С����������жϿ�ܴ�����
 * 
 * ���룺
 * 		driver�������豸�����ṹָ��
 * �����
 * 		��
 * ���أ�
 * 		��
 */
void serial_int_isr1(struct serial_device_struct_ex *driver)
{
//	printk("==========================\n");
//	sem_post(&driver->isr_sem);
#if 1
	int lsr_status;
	int iir_status;
	
	
		
	iir_status = from_serial(driver, SERIAL_IIR) &0xff;	/* ȡIIR */
	if (SERIAL_IIR_NO_INT & iir_status)		/* ���жϷ��� */
	{
		return;
	}
	
	lsr_status = from_serial(driver, SERIAL_LSR);	/* ȡLSR */
	
	/* ����Rx */
	if (SERIAL_LSR_RXV & lsr_status)
	{
		rx_in_isr(driver, &lsr_status);	/* �ڲ��Ӻ������� */
	}
	
	/* Tx */
	if ((SERIAL_LSR_THRE & lsr_status) || ((iir_status & 0x06) == SERIAL_IIR_THRI))
	{
		tx_in_isr(driver, &lsr_status);	/* �ڲ��Ӻ������� */
	}			

#endif
	return;
}

/******************************************************************************
 * 
 * �����豸�򿪽ӿ�
 * 
 * ���룺
 * 		driver�������豸�����ṹָ��
 * �����
 * 		��
 * ���أ�
 * 		�ɹ�����0��ʧ�ܷ���-1
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
	
	/* �������Ϊ0����˵����һ�δ򿪴����豸�������ʹ���ж� */
	if (0 == serial_driver->count)
	{
		int_enable_pic(serial_driver->intvec);
	}
	
	serial_driver->count++;
	
	return 0;
}

/******************************************************************************
 * 
 * �����豸�رսӿ�
 * 
 * ���룺
 * 		driver�������豸�����ṹָ��
 * �����
 * 		��
 * ���أ�
 * 		�ɹ�����0��ʧ�ܷ���-1
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
 * ����һ�η��ͽӿ�
 * 
 * 		������������һ�η��ͣ����������ϲ�TTY������е��á���Ӧ��ͨ��IO��write�ӿ�
 * ���ɹ�����򴮿�д����ʱ���������ñ��ӿ�����һ�η��͡���������ͨ��Tx�ж�ʹ�ܡ�
 * ʵ��ISR�ĵ��á�
 * 
 * ���룺
 * 		driver�������豸�����ṹָ��
 * �����
 * 		��
 * ���أ�
 * 		�ɹ�����0��ʧ�ܷ���-1
 */
int serial_start_tx(struct tty_driver *driver)
{	
	struct serial_device_struct_ex *serial_driver = 
		(struct serial_device_struct_ex *)driver;
	
	/* Tx�ж�ʹ�ܣ������ݵ�IER */
	serial_driver->regs.IER |= SERIAL_IER_THEN;
	
	/* ʹ��Tx�ж� */
	to_serial(serial_driver, SERIAL_IER, serial_driver->regs.IER);
	
	return 0;
}


/******************************************************************************
 * 
 * ֹͣ���ͽӿ�
 * 
 * 		��������ֹͣ���ͣ�����������һ�������е��á���������õ�ʱ�����ϲ�����
 * ���޵ȴ�������ַ�����������ͨ��Tx����ֹͣ�ַ����͡�
 * 		���Ӧ�ó����������Զ����Ϳ���ѡ��IOS_LDISC_TX_AUTO����ô��������޴������
 * �ַ�ʱ�Զ����ñ��ӿڣ���Ȼ�����û�����ø�ѡ���ô������ISR��ֹͣ���͡����û��
 * �����ʵ�ֱ�������ô�Զ����Ϳ���ѡ������ã������Ƿ����ø�ѡ�
 * 
 * ���룺
 * 		driver�������豸�����ṹָ��
 * �����
 * 		��
 * ���أ�
 * 		�ɹ�����0��ʧ�ܷ���-1
 */
int serial_stop_tx(struct tty_driver *driver)
{	
	struct serial_device_struct_ex *serial_driver = 
		(struct serial_device_struct_ex *)driver;
	
	/* Tx�жϷ��ܣ������ݵ�IER */
	serial_driver->regs.IER |= ~SERIAL_IER_THEN;
	
	/* Tx�жϷ��� */
	to_serial(serial_driver, SERIAL_IER, serial_driver->regs.IER);
	
	return 0;
}


/******************************************************************************
 * 
 * �������սӿ�
 * 
 * 		���������������գ�����������һ�������е��á�
 * 
 * ���룺
 * 		driver�������豸�����ṹָ��
 * �����
 * 		��
 * ���أ�
 * 		�ɹ�����0��ʧ�ܷ���-1
 * ��ע��
 * 		��������δʵ�ֵ��á�
 */
int serial_start_rx(struct tty_driver *driver)
{	
	struct serial_device_struct_ex *serial_driver = 
		(struct serial_device_struct_ex *)driver;
	
	/* Rxʹ�ܣ������ݵ�IER */
	serial_driver->regs.IER |= (SERIAL_IER_RBEN | SERIAL_IER_RLSEN);
	
	/* ʹ��Rx�ж� */
	to_serial(serial_driver, SERIAL_IER, serial_driver->regs.IER);
	
	return 0;
}


/******************************************************************************
 * 
 * ֹͣ���սӿ�
 * 
 * 		��������ֹͣ���գ�����������һ�������е��á�
 * 
 * ���룺
 * 		driver�������豸�����ṹָ��
 * �����
 * 		��
 * ���أ�
 * 		�ɹ�����0��ʧ�ܷ���-1
 * ��ע��
 * 		��������δʵ�ֵ��á�
 */
int serial_stop_rx(struct tty_driver *driver)
{	
	struct serial_device_struct_ex *serial_driver = 
		(struct serial_device_struct_ex *)driver;
	
	/* Rx���ܣ������ݵ�IER */
	serial_driver->regs.IER |= ~(SERIAL_IER_RBEN | SERIAL_IER_RLSEN);
	
	/* Rx���� */
	to_serial(serial_driver, SERIAL_IER, serial_driver->regs.IER);
	
	return 0;
}


/******************************************************************************
 * 
 * �����ַ��ӿ�
 * 
 * 		������ͨ��ֱ��дTRH�Ĵ����򴮿����һ���ַ���
 * 
 * ���룺
 * 		driver�������豸�����ṹָ��
 * 		c�������͵��ַ�
 * �����
 * 		��
 * ���أ�
 * 		�ɹ�����0��ʧ�ܷ���-1
 */
int serial_tx_char(struct tty_driver *driver, char c)
{
	struct serial_device_struct_ex *serial_driver = 
		(struct serial_device_struct_ex *)driver;
	
	/* ����ַ�THR */
	if (SERIAL_LSR_THRE & from_serial(serial_driver, SERIAL_LSR))
	{
		to_serial(serial_driver, SERIAL_THR, c);		
		return 0;
	}
	
	return -1;
}


/******************************************************************************
 * 
 * �����ַ��ӿ�
 * 
 * 		������ͨ��ֱ�Ӷ�RDH�Ĵ�������������һ���ַ���
 * 
 * ���룺
 * 		driver�������豸�����ṹָ��
 * �����
 * 		c����Ž��յ��ַ����ֽ�����ָ��
 * ���أ�
 * 		�ɹ�����0��ʧ�ܷ���-1
 */
int serial_rx_char(struct tty_driver *driver, char *c)
{
	struct serial_device_struct_ex *serial_driver = 
		(struct serial_device_struct_ex *)driver;
	
	/* ��ȡRDH�Ĵ��������Ĵ�������д��c */
	if (SERIAL_LSR_RXV & from_serial(serial_driver, SERIAL_LSR))
	{
		*c = from_serial(serial_driver, SERIAL_RBR);
		return 0;
	}	
	
	return -1;	
}


/******************************************************************************
 * 
 * �����豸���ƽӿ�
 * 
 * ���룺
 * 		driver�������豸�����ṹָ��
 * 		u32����������
 * 		arg���������
 * �����
 * ���أ�
 * 		�ɹ�����0��ʧ�ܷ���-1
 */
int serial_ioctl(struct tty_driver *driver, u32 cmd, void *arg)
{
	struct serial_device_struct_ex *serial_driver = 
		(struct serial_device_struct_ex *)driver;
	
	int status = 0;
	int tmp;	/* ��ʱ���� */
	
	switch (cmd)
	{
		case TTY_SET_BAUDRATE :	/* ���ò����� */
		{
			tmp = (int)arg;
			/* �Բ����ʽ��м�� */
			if ((50 > tmp) || (115200 < tmp))
			{
				return -1;
			}
			status = set_baudrate(serial_driver, tmp);
			break;
		}
		case TTY_GET_BAUDRATE :	/* ��ȡ������ */
		{
			if(!arg)
			{
				return -1;
			}
			*(int *)arg = serial_driver->baudrate;
			break;
		}
		case TTY_SET_MODE :		/* ���÷���ģʽ */
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
		case TTY_GET_MODE :		/* ��ȡ����ģʽ */
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
		case TTY_SERIAL_LCR :		/* ����LCR�Ĵ��� */
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
 * ����ѯ�ķ�ʽ�Ӵ����豸��ȡ����
 * 
 * ���룺
 * 		serial_num�����ʵĴ��ں�
 * 		count������ȡ�����ݴ�С
 * �����
 * 		buffer����Ŵ���ȡ���ݵĻ�����
 * ���أ�
 * 		�ɹ����ض�ȡ���ַ�������ʧ�ܷ���-1
 */
#if 0
int tx_in_poll(int serial_num, const char *buffer, int count)
{
	int writed = 0;		/* ��д����ַ����� */
	
	/* ������� */
	if ((0 > serial_num) || (SERIAL_NUMBER <= serial_num))
	{
		return -1;
	}
	
	/* Tx���� */
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
	int writed = 0;		/* ��д����ַ�����  Afrit modified 20151202*/
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
 * ����ѯ�ķ�ʽ�Ӵ����豸��ȡ����
 * 
 * ���룺
 * 		serial_num�����ʵĴ��ں�
 * 		count������ȡ�����ݴ�С
 * �����
 * 		buffer����Ŵ���ȡ���ݵĻ�����
 * ���أ�
 * 		�ɹ����ض�ȡ���ַ�������ʧ�ܷ���-1
 */
#if 0
int rx_in_poll(int serial_num, char *buffer, int count)
{	
	int readed = 0;		/* �Ѷ�ȡ���ַ����� */
	
	/* ������� */
	if ((0 > serial_num) || (SERIAL_NUMBER <= serial_num) ||
		(NULL == buffer))
	{
		return -1;
	}
	
	/* �ر�Rx�ж� */
	
	
	/* Rx���� */
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
 * ��ʼ������Ӳ���豸
 * 
 * ���룺
 * 		driver�������豸�����ṹָ��
 * �����
 * ���أ�
 * 		�ɹ�����0��ʧ�ܷ���-1
 */
int serial_hw_init(struct serial_device_struct_ex *driver)
{	
	int level;
	
	level = int_lock();
	
	/* IER, LCR, FCR��� */
	to_serial(driver, SERIAL_LCR, 0);		
	to_serial(driver, SERIAL_FCR, 0);			
	to_serial(driver, SERIAL_IER, 0);	
	
	/* IER */
	to_serial(driver, SERIAL_IER, 0);	/* ���ж� */
	driver->regs.IER = 0;				/* ��ʼ״̬�������жϷ���ģʽ */
	
	/* modem���ƼĴ������Ƿ���Ҫ??? */
	driver->regs.MCR = 0;
	driver->regs.MCR |= SERIAL_MCR_OUT2;
	driver->regs.MCR &= (~(SERIAL_MCR_DTR | SERIAL_MCR_RTS));
	to_serial(driver, SERIAL_MCR, driver->regs.MCR);
	
	/* ����FIFO��ʹ�ܡ����塢���� */
	driver->regs.FCR = SERIAL_FCR_EN | 
					   SERIAL_FCR_RXCLR | 
					   SERIAL_FCR_TXCLR;
	
	/* LCR */
	set_lcr(driver, (CS8 | STOP1 | DENBPAR));
	
	/* FCR */
	to_serial(driver, SERIAL_FCR, driver->regs.FCR); 
	
	set_baudrate(driver, driver->baudrate);
	
	/* IER�ж�ʹ�� */
	to_serial(driver, SERIAL_IER, driver->regs.IER);
	
	int_unlock(level);
	
	return 0;
}



/******************************************************************************
 * 
 * ��ʼ�����ڽӿ�
 * 
 * 		������ ��Ҫ��ɴ����豸Ӳ����ʼ������������ɵĹ�����Ҫ��Ϊ�������棺������չ
 * �Ĵ����豸�����ṹ�������ʼ��Ӳ����ص���ṹ���Խ��и�ֵ��
 * 
 * ���룺
 * 		��
 * �����
 * 		��
 * ���أ�
 * 		�ɹ�����0��ʧ�ܷ���-1
 * ��ע��
 * 		���ӿ�ִ����Ϻ󣬿ɱ�֤��ѯ���ʴ��ڵĽӿ�rx/tx_in_poll���á�
 */
int serial_device_init(void)
{
	int i;	/* ѭ������ */
	
	/* ��ʼ����ʶ */
	static int serial_device_init_flag = 0;
	if (serial_device_init_flag)
	{
		return 0;
	}
	
	for (i = 0; i < SERIAL_NUMBER; i++)	
	{
		struct serial_device_struct_ex *driver = &global_serial_struct[i];
		memset(driver, 0, sizeof(struct serial_device_struct_ex));
		
		/* ��չ�ṹ���������� */
		driver->base_baudrate			= serial_param_define[i].base_baudrate;
		driver->baudrate				= serial_param_define[i].baudrate;
		driver->regs_base				= (u8 *)serial_param_define[i].regs_base;
		driver->delta					= serial_param_define[i].delta;		
		driver->count					= 0;
		driver->intvec					= serial_param_define[i].intvec;
		
		/* ��ʼ������Ӳ�� */
		serial_hw_init(driver);
	}
	
	/* ��ʼ����ʶ */
	serial_device_init_flag = 1;
	
	return 0;
}


/*******************************************************************************
 * 
 * ע�ᴮ���豸
 * 
 * 		������������TTY��ע�ᴮ���豸�����������Ҫ����Ϊ����struct tty_driver�Ĳ���
 * ��ֵ�����뱾�豸���õĴ��豸�š��ҽ��жϡ�ע�ᴮ���豸��
 * 		
 * 
 * ���룺
 * 		��
 * �����
 * 		��
 * ���أ�
 * 		�ɹ�����0��ʧ�ܷ���-1
 * ��ע��
 * 		���ӿ�ִ����Ϻ󣬿��ù���׼I/O�ӿڷ��ʴ����豸��
 */
int serial_device_register(void)
{
	int i;			/* ѭ������ */
	int retn;		/* ��ʱ����ֵ */
	dev_t 	minor;
	
	/* ��ʼ����ʶ */
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
		
		/* ������ֵ */
		base_driver->open				= (TTY_OPEN)serial_open;
		base_driver->close				= (TTY_CLOSE)serial_close;
		base_driver->start_tx 			= (TTY_START_TX)serial_start_tx;
		base_driver->stop_tx 			= (TTY_STOP_TX)serial_stop_tx;
		base_driver->start_rx			= (TTY_START_RX)serial_start_rx;
		base_driver->stop_rx			= (TTY_STOP_RX)serial_stop_rx;
		base_driver->tx_char 			= (TTY_TX_CHAR)serial_tx_char;
		base_driver->rx_char 			= (TTY_RX_CHAR)serial_rx_char;
		base_driver->ioctl 				= (TTY_IOCTL)serial_ioctl;	
		
		/* ������豸�� */
		if (0 != chrdev_minor_request(TTY_MAJOR, TTY_SERIAL_START, 1, TTY_TERM_START -1 , &minor))
		{
			printk("[ERROR] serial driver cannot request minor\n");
			return -1;
		}		
		base_driver->dev	 			= MKDEV(TTY_MAJOR, minor);		/* �豸�� */
		
		/* ע���豸 */
		register_tty_device(serial_param_define[i].ldisc_name, 
							serial_param_define[i].registered_path, 
							base_driver);	
		
//		printk("%s %d Register serial isr handler:serial_int_isr1|%d(DUART%d) \n",__func__,__LINE__,serial_param_define[i].intvec,i);
		/* �ҽ��ж� */	
		/* ISR���ơ��ж����������ȼ���ISR��ISR���� */
		int_install_handler(serial_param_define[i].intvec, 
							(void (*)(void *))serial_int_isr1, 
							(void *)driver);
		
		/* �����ж�ģʽ */
		driver->regs.IER = (SERIAL_IER_RBEN | SERIAL_IER_RLSEN);/* ����״̬�жϡ����ж� */
		driver->regs.IER &= ~SERIAL_IER_MSEN;	/* ����modem�жϣ��˴��������ã���ɾ */
		to_serial(driver, SERIAL_IER, driver->regs.IER);
		
		/* ΪISR�����������ź�������ʼֵΪ0�� */
		sem_init2(&driver->isr_sem, 0, SEM_BINARY, PTHREAD_WAITQ_FIFO, 0);
		
		/* ����ISR2�߳� */
		retn = pthread_create2(&driver->isr_thread, 					/* ID */
							   strrchr(serial_param_define[i].registered_path, '/'),	/* ���� */
							   220,										/* ���ȼ� */
							   RE_UNBREAKABLE | RE_FP_TASK,				/* ѡ�� */
							   0x8000,									/* ջ */
							   (void*(*)(void *))serial_int_isr2,		/* ��� */
							   (void *)driver);							/* ���� */
		if (0 != retn)
		{
			printk("[ERROR] cannot create thread\n");
	
			return -1;
		}
	}
	
	/* ��ʼ����ʶ */
	serial_device_register_flag = 1;
	
	return 0;
}


/*******************************************************************************
 * 
 * ����ģ���ʼ���ӿ�
 * 
 * ���룺
 * 		��
 * �����
 * 		��
 * ���أ�
 * 		��
 * �޸ģ�
 * 		2011-11-11��ɾ��������ʼ������ģ����ļ������ú����ϲ�����
 */
int serial_module_init(void)
{
	
	/* ��ʼ��������������Ĺ���� */
	line_ldisc_init();
	
	/* ��ʼ�������豸 */
	serial_device_init();
	
	/* ע�ᴮ���豸 */
	serial_device_register();
	
	/* ������崮����Ϊ��׼I/O */
	int fd = open("/dev/serial0", O_RDWR, 0777);
	
	ioctl(fd, TTY_SET_OPTIONS, TTY_OUT_CR2NL | TTY_IN_ECHO);
	
	global_std_set(0, fd);
	global_std_set(1, fd);
	global_std_set(2, fd);
	
	return 0;
}

