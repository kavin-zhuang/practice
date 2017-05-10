/*
 * net.c
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
#include <arpa/inet.h>
#include <net/if.h>
#include <drv/netLib.h>
#include <netInit.h>
#include <string.h>

#include "net.h"

#define CPSW_IRQ  0

/* ??? */
#define PHY_ADDR 0

#define UNIT_NUM 0

#define CPSW_TUPLE_CNT       384

extern NET_FUNCS cpswNetFuncs;

static int flag_module_inited = 0;

static unsigned char macaddr = {0xaa,0xbb,0xcc,0xdd,0xee,0xff};

int sysEnetRefClkGet(void)
{
  TRACE();
  
  return 0;
}

VXB_DEVICE pDevResource[] =
{
  {
    (char*)"cpsw",
    UNIT_NUM,
    {(void*)0x4A100000},
    PHY_ADDR,
    NULL,
    sysEnetRefClkGet,
    2,          /*RGMII*/
    1,          /* combin buf */
    72          /* irq */
  },
  {NULL,0,{NULL},0,NULL,NULL,0,0,0}
};

static void * miiBusMonitorTask(void *arg)
{
  while(1) {
    pthread_delay(20000);
  }
}

CPSW_DRV_CTRL *gpDrvCtrl = NULL;

void phy_up_test(void)
{
  jobQueueStdPost(gpDrvCtrl->JobQueue, 
                  NET_TASK_QJOB_PRI,
                  (RE_VOIDFUNCPTR)muxLinkUpNotify, 
                  &gpDrvCtrl->End,
                  NULL, NULL, NULL, NULL);
}

void send_complete(void) {
  TRACE();
}

void mac_recv_test(UINT32 addr) {
  M_BLK_ID pMblk = addr;
  
  END_RCV_RTN_CALL(&gpDrvCtrl->End, pMblk);
}

int mac_send_test(void)
{
  QJOB TxJob;
  
  memset(&TxJob, 0, sizeof(TxJob));
  
  /* BUG ? */
  QJOB_SET_PRI(&TxJob, NET_TASK_QJOB_PRI);

  TxJob.func = send_complete;
  
  jobQueuePost(gpDrvCtrl->JobQueue, &TxJob);
  
  return 0;
}

int init_end_struture(void)
{
  return 0;
}

static void int_handler(void *arg)
{
  CPSW_DRV_CTRL *pDrvCtrl = (CPSW_DRV_CTRL*)arg;
}

/*
 * cpswEndLoad:
 * 
 * 1. initialise MAC and PHY
 * 2. malloc space for descriptors
 * 3. malloc space for holding all information
 * 4. malloc space for data (pool)
 * 5. install interrupt handler
 * 6. create a thread to monitor phy link status (up/down)
 */

/* TODO: what 'loadStr' means */

END_OBJ* cpswEndLoad(char *loadStr, void *arg)
{
  TRACE();
  
  pthread_t pid;
  int ret;
  
  VXB_DEVICE *pDev = &pDevResource[0];
  
  if(NULL == loadStr) {
    return NULL;
  }
  
  if(0 == loadStr[0]) {
    TRACE("request device name");
    snprintf(loadStr, 16, "cpsw");
    return NULL;
  }
  
  TRACE("request object, init string is \"%s\"", loadStr);
  
  /* 
   * BELOW malloc cpsw instance for all information, and reset cpsw
   */
  
  CPSW_DRV_CTRL *pDrvCtrl = (CPSW_DRV_CTRL*)malloc(sizeof(CPSW_DRV_CTRL));
  if(NULL == pDrvCtrl) {
    BSP_ERROR();
    goto failed;
  }
  
  pDev->pDrvCtrl = pDrvCtrl;
  
  memset(pDrvCtrl, 0, sizeof(CPSW_DRV_CTRL));
  
  memcpy(pDrvCtrl->MacAddr, macaddr, ETHER_ADDR_LEN);
  
  pDrvCtrl->pDev = pDev;

  /* 
   * malloc space for NetCard DMA engine's descriptions 
   * MUST BE no-cached
   */
  
//  pDrvCtrl->RxDescMem = malloc_no_cache_align(sizeof(FEC_DESC) * FEC_RX_DESC_CNT, 4);
//  memset(pDrvCtrl->RxDescMem, 0, sizeof(FEC_DESC) * FEC_RX_DESC_CNT);
//  
//  pDrvCtrl->fecTxDescMem = malloc_no_cache_align(sizeof(FEC_DESC) * FEC_TX_DESC_CNT, 4);
//  memset(pDrvCtrl->fecTxDescMem, 0, sizeof(FEC_DESC) * FEC_TX_DESC_CNT);
//  
  
  
  /* export for test */
  gpDrvCtrl = pDrvCtrl;
  
  /* 
   * install interrupt handler
   */
  
  ret = int_install_handler(CPSW_IRQ, int_handler, pDrvCtrl);
  if(-1 == ret) {
    BSP_ERROR();
    goto failed;
  }
  
  /*
   * monitor thread check phy status and send up/down event to MUX layer 
   */
  
  pthread_create2(&pid, "miiBusMonitor", 1, 0, 4096, miiBusMonitorTask, NULL);
  
  /* init for phy part of cpsw instance, and really init phy */
  
  /* finally fill pEnd structure, and register end ops to MUX */
    
  ret = END_OBJ_INIT(&pDrvCtrl->End, NULL, "cpsw", UNIT_NUM, &cpswNetFuncs, "cpsw end driver");
  if(-1 == ret) {
    BSP_ERROR();
    goto failed;
  }
  
  /* Initialize MIB-II agnostically */
  endM2Init(&pDrvCtrl->End, 
          M2_ifType_ethernet_csmacd,
          pDrvCtrl->MacAddr, 
          ETHER_ADDR_LEN,
          ETHERMTU,
          100000000,
          IFF_NOTRAILERS | IFF_SIMPLEX | IFF_MULTICAST | IFF_BROADCAST);
  
  /* 
   * create mBlks, Clblks & Clusters, which is equal to CPSW_TUPLE_CNT
   * as they are all linked, data will not be copied, just change the link
   * link header is holded in End.pNetPool
   */
  ret = endPoolCreate(CPSW_TUPLE_CNT, &pDrvCtrl->End.pNetPool);
  if(-1 == ret) {
    BSP_ERROR("failed to create pool");
  }
  
  /*
   * OK, here, show me the pool
   */
  
  /*
   * What they do ?
   */
  pDrvCtrl->PollBuf = endPoolTupleGet(pDrvCtrl->End.pNetPool);
  
  TRACE("PollBuf = %x\n", pDrvCtrl->PollBuf);
  
  return (&pDrvCtrl->End);
  
failed:
  return NULL;
}

int network_driver_init(int unit, char *ip, char *mask)
{
  TRACE();
  
  int ret;
  DEV_COOKIE *pCookie = NULL;
  char ifname[8];
  
  if(flag_module_inited) {
    return 0;
  }
  
  endLibInit();
  
  pCookie = muxDevLoad(UNIT_NUM, cpswEndLoad, "cpsw", FALSE, NULL);
  if(NULL == pCookie) {
    BSP_ERROR();
    return -1;
  }
  
  ret = muxDevStart(pCookie);
  if(-1 == ret) {
    BSP_ERROR("failed to start device");
    return -1;
  }
  
  /*
   * If attach successful, "ifconfig" will show you this interface 
   */
  
  ret = ip_attach(unit,"cpsw");
  if(-1 == ret) {
    BSP_ERROR("failed to create net interface");
    return -1;
  }
  
  /*
   * The interface is a combination: "[name][num]"
   */

  snprintf(ifname, 8, "%s%d", "cpsw", 0);
  
  /*
   * After set IP and Mask, the protocol can route the package to netcard
   * correctly 
   */
  
  ret = if_addr_set(ifname, ip);
  if(-1 == ret) {
    BSP_ERROR("failed to set interface address");
    return -1;
  }
  
  ret = if_mask_set(ifname, htonl(inet_addr(mask)));
  if(-1 == ret) {
    BSP_ERROR("failed to set interface mask");
    return -1;
  }
  
  /*
   * This will cause protocol calls start routine
   */
  ret = if_flag_change(ifname, IFF_UP, TRUE);
  if(-1 == ret) {
    BSP_ERROR("failed to set interface up");
    return -1;
  }
  
  flag_module_inited = 1;
  
  return 0;
}
