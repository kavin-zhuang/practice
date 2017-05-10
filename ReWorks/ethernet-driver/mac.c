/*
 * mac.c
 * 
 * THIS is a demo of network driver for ipnet.
 *
 * Copyright (C) 2016, 上海华元创信软件有限公司
 *
 * 2017.1, jinfeng
 *
 */

#include <bsp.h>
#include <muxLib.h>
#include <drv/vxcompat/endLib.h>
#include <sys/errno.h>
#include <drv/netLib.h>

#include "net.h"

/* TODO: */
#define IOC_USER        0x8000 /* tunnellable from RTP space */
#define _IOU(g,n) (IOC_USER|_IO(g,n))
#define _IORU(g,n,t)  (IOC_USER|_IOR(g,n,t))
#define EIOCGFLAGS _IORU('e', 5, int) /* Get device flag */

static int cpswEndUnload(END_OBJ *pEnd)
{
  TRACE();
  
  return 0;
}

static int cpswEndIoctl(END_OBJ *pEnd, int cmd, caddr_t data)
{
  TRACE("cmd = %08X data = %08X", cmd, data);
  
  int error = 0;
  
  CPSW_DRV_CTRL * pDrvCtrl;
  VXB_DEVICE *pDev;
  END_RCVJOBQ_INFO * qinfo;
  UINT32 nQs;
  END_CAPABILITIES * hwCaps;
  
  pDrvCtrl = (CPSW_DRV_CTRL *) pEnd;
  pDev = pDrvCtrl->pDev;

  switch(cmd) {
  
  case EIOCGFLAGS:
    if (NULL == data)
      error = EINVAL;
    else
      *(long *) data = END_FLAGS_GET(pEnd);
    break;
    
  case EIOCGADDR:
    if (data == NULL)
      error = EINVAL;
    else
      bcopy((char *) pDrvCtrl->MacAddr, (char *)data, ETHER_ADDR_LEN);
    break;
    
  case EIOCGIFCAP:
    hwCaps = (END_CAPABILITIES *) data;
    if (NULL == hwCaps) {
      error = EINVAL;
    }
    else {
//      hwCaps->cap_available = pDrvCtrl->fecCaps.cap_available;
//      hwCaps->cap_enabled = pDrvCtrl->fecCaps.cap_enabled;
    }

    break;
    
  case EIOCGRCVJOBQ:
    
    if(NULL == data) {
      error = EINVAL;
      break;
    }

    qinfo = (END_RCVJOBQ_INFO *) data;
    nQs = qinfo->numRcvJobQs;
    qinfo->numRcvJobQs = 1;
    
    TRACE("nQs = %d", nQs);
    
    if (nQs < 1) {
      error = ENOSPC;
    }
    else {
      qinfo->qIds[0] = pDrvCtrl->JobQueue;
    }

    break;
      
  default:
    error = EINVAL;;
    break;
  }
  
  TRACE("error = %d", error);
  
  return error;
}

static int cpswEndMCastAddrAdd(END_OBJ *pEnd, char *addr)
{
  TRACE();
  
  return 0;
}

static int cpswEndMCastAddrDel(END_OBJ *pEnd, char *addr)
{
  TRACE();
  
  return 0;
}

static int cpswEndMCastAddrGet(END_OBJ *pEnd, MULTI_TABLE *tbl)
{
  TRACE();
  
  return 0;
}
static int cpswEndStart(END_OBJ *pEnd)
{
  TRACE();
  
  CPSW_DRV_CTRL *pCtrl = (CPSW_DRV_CTRL*)pEnd;
  
  pCtrl->JobQueue = netJobQueueId;
  
  return 0;
}

static int cpswEndStop(END_OBJ *pEnd)
{
  TRACE();
  
  return 0;
}

void mblk_dump(M_BLK_ID pMblk)
{
  printk("==============================================\r\n");
  printk("mBlkHdr:\r\n");
  printk("\t mNext = %x\r\n", pMblk->mBlkHdr.mNext);
  printk("\t mNextPkt = %x\r\n", pMblk->mBlkHdr.mNextPkt);
  printk("\t mData = %x\r\n", pMblk->mBlkHdr.mData);
  printk("\t mLen = %x\r\n", pMblk->mBlkHdr.mLen);
  printk("\t mType = %x\r\n", pMblk->mBlkHdr.mType);
  printk("\t mFlags = %x\r\n", pMblk->mBlkHdr.mFlags);
  
  printk("mBlkPktHdr:\r\n");
  printk("\t len = %x\r\n", pMblk->mBlkPktHdr.len);
  
  printk("pClBlk:\r\n");
  printk("\t clNode = %x\r\n", pMblk->pClBlk->clNode);
  printk("\t clSize = %x\r\n", pMblk->pClBlk->clSize);
  printk("\t clRefCnt = %x\r\n", pMblk->pClBlk->clRefCnt);
  printk("\t pNetPool = %x\r\n", pMblk->pClBlk->pNetPool);
}

static int cpswEndSend(END_OBJ *pEnd, M_BLK_ID pMblk)
{
  TRACE("pMblk = %x", pMblk);
  
  mblk_dump(pMblk);
  
  endPoolTupleFree(pMblk);
  
  return 0;
}

static int cpswEndPollSend(END_OBJ *pEnd, M_BLK_ID id)
{
  TRACE();
  
  return 0;
}

static int cpswEndPollReceive(END_OBJ *pEnd, M_BLK_ID id)
{
  TRACE();
  
  return 0;
}

NET_FUNCS cpswNetFuncs =
{
  cpswEndStart,  /* start func. */
  cpswEndStop,/* stop func. */
  cpswEndUnload, /* unload func. */
  cpswEndIoctl,  /* ioctl func. */
  cpswEndSend,/* send func. */
  cpswEndMCastAddrAdd, /* multicast add func. */
  cpswEndMCastAddrDel, /* multicast delete func. */
  cpswEndMCastAddrGet, /* multicast get fun. */
  cpswEndPollSend,  /* polling send func. */
  cpswEndPollReceive,  /* polling receive func. */
  endEtherAddressForm,  /* put address info into a NET_BUFFER */
  endEtherPacketDataGet,/* get pointer to data in NET_BUFFER */
  endEtherPacketAddrGet /* Get packet addresses */
};
