#ifndef BSP_NET_H
#define BSP_NET_H

#include <net/ethernet.h>

#define VXB_MAXBARS   1

typedef struct vxbDev 
{
  char *      pName;
  UINT32      unitNumber;

  void *      pRegBase[VXB_MAXBARS];
  int         phyAddr;

  void *      pDrvCtrl;
  FUNCPTR     clkFreq;
  int         mediaIfMode;
  int         combinBuf;
  int         irq;
} VXB_DEVICE, *VXB_DEVICE_ID;

typedef struct {
  END_OBJ         End;
  VXB_DEVICE_ID   pDev;
  JOB_QUEUE_ID    JobQueue;
  UINT8           MacAddr[ETHER_ADDR_LEN];
  M_BLK_ID        PollBuf;
} CPSW_DRV_CTRL;

#endif
