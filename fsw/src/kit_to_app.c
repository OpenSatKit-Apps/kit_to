/*
** Purpose: Define the OpenSatKit Telemetry Output application. This app
**          receives telemetry packets from the software bus and uses its
**          packet table to determine whether packets should be sent over
**          a UDP socket.
**
** Notes:
**   1. This is non-flight code so an attempt has been made to balance keeping
**      it simple while making it robust. Limiting the number of configuration
**      parameters and integration items (message IDs, perf IDs, etc) was
**      also taken into consideration.
**   2. Performance traces are not included.
**   3. Most functions are global to assist in unit testing
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide.
**   2. cFS Application Developer's Guide.
**
**   Written by David McComas, licensed under the Apache License, Version 2.0
**   (the "License"); you may not use this file except in compliance with the
**   License. You may obtain a copy of the License at
**
**      http://www.apache.org/licenses/LICENSE-2.0
**
**   Unless required by applicable law or agreed to in writing, software
**   distributed under the License is distributed on an "AS IS" BASIS,
**   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**   See the License for the specific language governing permissions and
**   limitations under the License.
*/

/*
** Includes
*/

#include <string.h>
#include <math.h>
#include "kit_to_app.h"


/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macros */
#define  INITBL_OBJ   (&(KitTo.IniTbl))
#define  CMDMGR_OBJ   (&(KitTo.CmdMgr))
#define  TBLMGR_OBJ   (&(KitTo.TblMgr))
#define  PKTMGR_OBJ   (&(KitTo.PktMgr))
#define  EVTPLBK_OBJ  (&(KitTo.EvtPlbk))


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static int32 InitApp(void);
static void InitDataTypePkt(KIT_TO_DataTypePkt_t *DataTypePkt);
static void ProcessCommands(void);
static void SendHousekeepingPkt(KIT_TO_HkPkt_t *HkPkt);


/**********************/
/** File Global Data **/
/**********************/

/* 
** Must match DECLARE ENUM() declaration in app_cfg.h
** Defines "static INILIB_CfgEnum_t IniCfgEnum"
*/
DEFINE_ENUM(Config,APP_CONFIG)


/*****************/
/** Global Data **/
/*****************/

KIT_TO_Class_t  KitTo;


/******************************************************************************
** Function: KIT_TO_AppMain
**
*/
void KIT_TO_AppMain(void)
{

   int32  Status    = CFE_SEVERITY_ERROR;
   uint32 RunStatus = CFE_ES_APP_ERROR;
   uint32 StartupCnt;
   uint16 NumPktsOutput;

   Status = CFE_ES_RegisterApp();
   CFE_EVS_Register(NULL,0,0);

   /*
   ** Perform application specific initialization
   */
   if (Status == CFE_SUCCESS)
   {
      
      Status = InitApp();
   
   }

   if (Status == CFE_SUCCESS) RunStatus = CFE_ES_APP_RUN;
   
   
   /*
   ** Main process loop
   */
   
   CFE_EVS_SendEvent(KIT_TO_INIT_DEBUG_EID, KIT_TO_INIT_EVS_TYPE, "KIT_TO: About to enter loop\n");
   StartupCnt = 0;
   while (CFE_ES_RunLoop(&RunStatus))
   {
   
      /* Use a short delay during startup to avoid event message pipe overflow */
      if (StartupCnt < 200)
      { 
         OS_TaskDelay(20);
         ++StartupCnt;
      }
      else
      {
         OS_TaskDelay(KitTo.RunLoopDelay);
      }

      NumPktsOutput = PKTMGR_OutputTelemetry();
      
      CFE_EVS_SendEvent(KIT_TO_DEMO_EID, CFE_EVS_DEBUG, "Output %d telemetry packets", NumPktsOutput);

      ProcessCommands();

   } /* End CFE_ES_RunLoop */


   /* Write to system log in case events not working */

   CFE_ES_WriteToSysLog("KIT_TO App terminating, err = 0x%08X\n", Status);

   CFE_EVS_SendEvent(KIT_TO_APP_EXIT_EID, CFE_EVS_CRITICAL, "KIT_TO App: terminating, err = 0x%08X", Status);

   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of KIT_TO_AppMain() */


/******************************************************************************
** Function: KIT_TO_NoOpCmd
**
*/

boolean KIT_TO_NoOpCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   CFE_EVS_SendEvent (KIT_TO_APP_NOOP_EID, CFE_EVS_INFORMATION,
                      "Kit Telemetry Output (KIT_TO) version %d.%d.%d received a no operation command",
                      KIT_TO_MAJOR_VER,KIT_TO_MINOR_VER,KIT_TO_PLATFORM_REV);

   return TRUE;


} /* End KIT_TO_NoOpCmd() */


/******************************************************************************
** Function: KIT_TO_ResetAppCmd
**
*/

boolean KIT_TO_ResetAppCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   TBLMGR_ResetStatus(TBLMGR_OBJ);

   PKTMGR_ResetStatus();
   EVT_PLBK_ResetStatus();
   
   return TRUE;

} /* End KIT_TO_ResetAppCmd() */


/******************************************************************************
** Function: KIT_TO_SendDataTypeTlmCmd
**
*/

boolean KIT_TO_SendDataTypeTlmCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   int32 Status;

   CFE_SB_TimeStampMsg((CFE_SB_MsgPtr_t) &KitTo.DataTypePkt);
   Status = CFE_SB_SendMsg((CFE_SB_Msg_t *)&KitTo.DataTypePkt);

   return (Status == CFE_SUCCESS);

} /* End KIT_TO_SendDataTypeTlmCmd() */


/******************************************************************************
** Function: KIT_TO_SetRunLoopDelayCmd
**
*/
boolean KIT_TO_SetRunLoopDelayCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   const KIT_TO_SetRunLoopDelayCmdMsg_t *CmdMsg = (const KIT_TO_SetRunLoopDelayCmdMsg_t *) MsgPtr;
   KIT_TO_Class_t *KitToPtr = (KIT_TO_Class_t *)ObjDataPtr;
   boolean RetStatus = FALSE;
   
   if ((CmdMsg->RunLoopDelay >= KitTo.RunLoopDelayMin) &&
       (CmdMsg->RunLoopDelay <= KitTo.RunLoopDelayMax))
   {
   
      CFE_EVS_SendEvent(KIT_TO_SET_RUN_LOOP_DELAY_EID, CFE_EVS_INFORMATION,
                        "Run loop delay changed from %d to %d", 
                        KitToPtr->RunLoopDelay, CmdMsg->RunLoopDelay);
   
      KitToPtr->RunLoopDelay = CmdMsg->RunLoopDelay;
      
      PKTMGR_InitStats(KitToPtr->RunLoopDelay,INITBL_GetIntConfig(INITBL_OBJ, CFG_PKTMGR_STATS_CONFIG_DELAY));

      RetStatus = TRUE;
   
   }   
   else
   {
      
      CFE_EVS_SendEvent(KIT_TO_INVALID_RUN_LOOP_DELAY_EID, CFE_EVS_ERROR,
                        "Invalid commanded run loop delay of %d ms. Valid inclusive range: [%d,%d] ms", 
                        CmdMsg->RunLoopDelay,KitTo.RunLoopDelayMin,KitTo.RunLoopDelayMax);
      
   }
   
   return RetStatus;
   
} /* End KIT_TO_SetRunLoopDelayCmd() */


/******************************************************************************
** Function: KIT_TO_TestFilterCmd
**
*/
boolean KIT_TO_TestFilterCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   const KIT_TO_TestFilterCmdMsg_t *CmdPtr = (const KIT_TO_TestFilterCmdMsg_t *) MsgPtr;
      
   uint16 SeqCnt;
   uint16 SecHdrTimeLen = sizeof(CCSDS_TlmSecHdr_t);
   uint32 Seconds, Subseconds;
   uint32 SubSecDelta;
   char   FormatStr[132];
   CCSDS_TelemetryPacket_t TestPkt;
   CFE_TIME_SysTime_t      PktTime;
   PktUtil_Filter_t        Filter;
   
   Filter.Type  = PKTUTIL_FILTER_BY_SEQ_CNT;
   Filter.Param = CmdPtr->FilterParam;

   CFE_EVS_SendEvent(KIT_TO_TEST_FILTER_EID, CFE_EVS_INFORMATION,
                     "Filter by sequence counter: N=%d, X=%d, O=%d",
                     Filter.Param.N, Filter.Param.X, Filter.Param.O);

   for (SeqCnt=0; SeqCnt < 20; SeqCnt++)
   {
      CCSDS_WR_SEQ(TestPkt.SpacePacket.Hdr, SeqCnt);
      CFE_EVS_SendEvent(KIT_TO_TEST_FILTER_EID, CFE_EVS_INFORMATION,
                        ">>>SeqCnt=%2d: Filtered=%d\n", 
                        SeqCnt, PktUtil_IsPacketFiltered((const CFE_SB_MsgPtr_t)&TestPkt, &Filter));
   }


   Filter.Type  = PKTUTIL_FILTER_BY_TIME;

   CFE_EVS_SendEvent(KIT_TO_TEST_FILTER_EID, CFE_EVS_INFORMATION,
                     "Filter by time: N=%d, X=%d, O=%d. CCSDS_TIME_SIZE=%d bytes",
                     Filter.Param.N, Filter.Param.X, Filter.Param.O, SecHdrTimeLen); 

   if (SecHdrTimeLen == 6)
   {
      SubSecDelta = 0x0100;
      /* TODO cfe6.8 strcpy(FormatStr,">>>Time=0x%08X:%06X OSK Filtered=%d, cFS Filtered=%d\n"); */
      strcpy(FormatStr,">>>Time=0x%08X:%06X OSK Filtered=%d\n");
      CFE_SB_InitMsg(&TestPkt, CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_HK_TLM_MID)), 12, TRUE);
   }
   else
   {
      SubSecDelta = 0x01000000;
      /* TODO strcpy(FormatStr,">>>Time=0x%08X:%08X OSK Filtered=%d, cFS Filtered=%d\n"); */
      strcpy(FormatStr,">>>Time=0x%08X:%08X OSK Filtered=%d\n");
      CFE_SB_InitMsg(&TestPkt, CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_HK_TLM_MID)), 14, TRUE);
   }
   
   Seconds = 0;
   Subseconds = 0;
   for (SeqCnt=0; SeqCnt < 20; SeqCnt++)
   {
      /* TODO - Temp cfe6.8 Bootes workaround that iis fixed in cFE 7.0 (Caelum) */
      
      CCSDS_WR_SEC_HDR_SEC(TestPkt.Sec,Seconds);         /* TODO - EDS-Bootes: TestPkt.Sec.Seconds    = Seconds;    */
      CCSDS_WR_SEC_HDR_SUBSEC(TestPkt.Sec, Subseconds);  /* TODO - EDS-Bootes: TestPkt.Sec.Subseconds = Subseconds; */
      PktTime = CFE_SB_GetMsgTime((CFE_SB_MsgPtr_t)&TestPkt);

      /* TODO - cfe6.8 Wait for cfs_utils update
      CFE_EVS_SendEvent(KIT_TO_TEST_FILTER_EID, CFE_EVS_INFORMATION, FormatStr,
                        PktTime.Seconds, PktTime.Subseconds, 
                        PktUtil_IsPacketFiltered((const CFE_SB_MsgPtr_t)&TestPkt, &Filter),
                        CFS_IsPacketFiltered((CFE_SB_MsgPtr_t)&TestPkt,2,Filter.Param.N,Filter.Param.X,Filter.Param.O));
      */
      CFE_EVS_SendEvent(KIT_TO_TEST_FILTER_EID, CFE_EVS_INFORMATION, FormatStr,
                        PktTime.Seconds, PktTime.Subseconds, 
                        PktUtil_IsPacketFiltered((const CFE_SB_MsgPtr_t)&TestPkt, &Filter));
      
      Subseconds += SubSecDelta;
      ++Seconds;
   }

   return TRUE;

} /* End KIT_TO_TestFilterCmd() */


/******************************************************************************
** Function: SendHousekeepingPkt
**
*/
static void SendHousekeepingPkt(KIT_TO_HkPkt_t *HkPkt)
{

   /*
   ** KIT_TO Data
   */

   HkPkt->ValidCmdCnt   = KitTo.CmdMgr.ValidCmdCnt;
   HkPkt->InvalidCmdCnt = KitTo.CmdMgr.InvalidCmdCnt;

   HkPkt->RunLoopDelay  = KitTo.RunLoopDelay;

   /*
   ** PKTTBL Data
   */

   HkPkt->PktTblLastLoadStatus  = KitTo.PktTbl.LastLoadStatus;
   HkPkt->PktTblAttrErrCnt      = KitTo.PktTbl.LastLoadCnt;

   /*
   ** PKTMGR Data
   ** - At a minimum all pktmgr variables effected by a reset must be included
   ** - Some of these may be more diagnostic but not enough to warrant a
   **   separate diagnostic. Also easier for the user not to have to command it.
   */

   HkPkt->StatsValid  = (KitTo.PktMgr.Stats.State == PKTMGR_STATS_VALID);
   HkPkt->PktsPerSec  = round(KitTo.PktMgr.Stats.AvgPktsPerSec);
   HkPkt->BytesPerSec = round(KitTo.PktMgr.Stats.AvgBytesPerSec);

   HkPkt->TlmSockId = (uint16)KitTo.PktMgr.TlmSockId;
   strncpy(HkPkt->TlmDestIp, KitTo.PktMgr.TlmDestIp, PKTMGR_IP_STR_LEN);

   HkPkt->EvtPlbkEna      = KitTo.EvtPlbk.Enabled;
   HkPkt->EvtPlbkHkPeriod = (uint8)KitTo.EvtPlbk.HkCyclePeriod;
   
   CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) HkPkt);
   CFE_SB_SendMsg((CFE_SB_Msg_t *) HkPkt);

} /* End SendHousekeepingPkt() */


/******************************************************************************
** Function: InitApp
**
*/
static int32 InitApp(void)
{

   int32 Status = CFE_SEVERITY_ERROR;

   /*
   ** Initialize contained objects
   */
   
   if (INITBL_Constructor(INITBL_OBJ, KIT_TO_INI_FILENAME, &IniCfgEnum))
   {
      
      KitTo.CmdMidValue     = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_CMD_MID);
      KitTo.SendHkMidValue  = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_SEND_HK_MID);

      KitTo.RunLoopDelay    = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_RUN_LOOP_DELAY);
      KitTo.RunLoopDelayMin = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_RUN_LOOP_DELAY_MIN);
      KitTo.RunLoopDelayMax = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_RUN_LOOP_DELAY_MAX);
   
      PKTMGR_Constructor(PKTMGR_OBJ, INITBL_OBJ);
      
      EVT_PLBK_Constructor(EVTPLBK_OBJ, INITBL_OBJ);
      
      Status = CFE_SUCCESS;
   
   } /* End if INITBL Constructed */

      
   /*
   ** Initialize application managers
   */

   if (Status == CFE_SUCCESS)
   {

      Status = CFE_SB_CreatePipe(&KitTo.CmdPipe,
                                 INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_CMD_PIPE_DEPTH),
                                 INITBL_GetStrConfig(INITBL_OBJ, CFG_APP_CMD_PIPE_NAME)); 

      if (Status == CFE_SUCCESS) 
      {

         CFE_SB_Subscribe(CFE_SB_ValueToMsgId(KitTo.CmdMidValue),    KitTo.CmdPipe);
         CFE_SB_Subscribe(CFE_SB_ValueToMsgId(KitTo.SendHkMidValue), KitTo.CmdPipe);

      }
      else
      {

         CFE_EVS_SendEvent(KIT_TO_APP_INIT_ERR_EID, CFE_EVS_ERROR,
                           "Create SB Command Pipe %s with depth %d failed. SB Status = 0x%04X",
                           INITBL_GetStrConfig(INITBL_OBJ, CFG_APP_CMD_PIPE_NAME), 
                           INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_CMD_PIPE_DEPTH), Status);
      }
   
      CMDMGR_Constructor(CMDMGR_OBJ);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_NOOP_CMD_FC,            NULL,       KIT_TO_NoOpCmd,            0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_RESET_CMD_FC,           NULL,       KIT_TO_ResetAppCmd,        0);

      CMDMGR_RegisterFunc(CMDMGR_OBJ, KIT_TO_PKT_TBL_LOAD_CMD_FC,    TBLMGR_OBJ, TBLMGR_LoadTblCmd,         TBLMGR_LOAD_TBL_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, KIT_TO_PKT_TBL_DUMP_CMD_FC,    TBLMGR_OBJ, TBLMGR_DumpTblCmd,         TBLMGR_DUMP_TBL_CMD_DATA_LEN);

      CMDMGR_RegisterFunc(CMDMGR_OBJ, KIT_TO_ADD_PKT_CMD_FC,          PKTMGR_OBJ, PKTMGR_AddPktCmd,          PKKTMGR_ADD_PKT_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, KIT_TO_ENABLE_OUTPUT_CMD_FC,    PKTMGR_OBJ, PKTMGR_EnableOutputCmd,    PKKTMGR_ENABLE_OUTPUT_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, KIT_TO_REMOVE_ALL_PKTS_CMD_FC,  PKTMGR_OBJ, PKTMGR_RemoveAllPktsCmd,   0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, KIT_TO_REMOVE_PKT_CMD_FC,       PKTMGR_OBJ, PKTMGR_RemovePktCmd,       PKKTMGR_REMOVE_PKT_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, KIT_TO_SEND_PKT_TBL_TLM_CMD_FC, PKTMGR_OBJ, PKTMGR_SendPktTblTlmCmd,   PKKTMGR_SEND_PKT_TBL_TLM_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, KIT_TO_UPDATE_FILTER_CMD_FC,    PKTMGR_OBJ, PKTMGR_UpdateFilterCmd,    PKKTMGR_UPDATE_FILTER_CMD_DATA_LEN);
      
      CMDMGR_RegisterFunc(CMDMGR_OBJ, KIT_TO_SEND_DATA_TYPES_CMD_FC,    &KitTo, KIT_TO_SendDataTypeTlmCmd, 0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, KIT_TO_SET_RUN_LOOP_DELAY_CMD_FC, &KitTo, KIT_TO_SetRunLoopDelayCmd, KIT_TO_SET_RUN_LOOP_DELAY_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, KIT_TO_TEST_FILTER_CMD_FC,        &KitTo, KIT_TO_TestFilterCmd,      KIT_TO_TEST_FILTER_CMD_DATA_LEN);

      CMDMGR_RegisterFunc(CMDMGR_OBJ, KIT_TO_EVT_PLBK_CONFIG_CMD_FC,  EVTPLBK_OBJ, EVT_PLBK_ConfigCmd, EVT_PLBK_CONFIG_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, KIT_TO_EVT_PLBK_START_CMD_FC,   EVTPLBK_OBJ, EVT_PLBK_StartCmd,  EVT_PLBK_START_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, KIT_TO_EVT_PLBK_STOP_CMD_FC,    EVTPLBK_OBJ, EVT_PLBK_StopCmd,   EVT_PLBK_STOP_CMD_DATA_LEN);

      CFE_EVS_SendEvent(KIT_TO_INIT_DEBUG_EID, KIT_TO_INIT_EVS_TYPE, "KIT_TO_InitApp() Before TBLMGR calls\n");
      TBLMGR_Constructor(TBLMGR_OBJ);
      TBLMGR_RegisterTblWithDef(TBLMGR_OBJ, PKTTBL_LoadCmd, PKTTBL_DumpCmd, INITBL_GetStrConfig(INITBL_OBJ, CFG_PKTTBL_LOAD_FILE));

      CFE_SB_InitMsg(&KitTo.HkPkt, CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_HK_TLM_MID)), KIT_TO_TLM_HK_LEN, TRUE);
      InitDataTypePkt(&KitTo.DataTypePkt);

      /*
      ** Application startup event message
      */

      CFE_EVS_SendEvent(KIT_TO_APP_INIT_EID, CFE_EVS_INFORMATION,
                        "KIT_TO Initialized. Version %d.%d.%d",
                        KIT_TO_MAJOR_VER, KIT_TO_MINOR_VER, KIT_TO_PLATFORM_REV);

   } /* End if init success */
   
   return(Status);

} /* End of InitApp() */


/******************************************************************************
** Function: InitDataTypePkt
**
*/
static void InitDataTypePkt(KIT_TO_DataTypePkt_t *DataTypePkt)
{

   int16  i;
   char   StringVariable[10] = "ABCDEFGHIJ";

   CFE_SB_InitMsg(DataTypePkt, 
                  CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_DATA_TYPE_TLM_MID)),
                  KIT_TO_TLM_DATA_TYPE_LEN, TRUE);

   DataTypePkt->synch = 0x6969;
   #if 0
      DataTypePkt->bit1    = 1;
      DataTypePkt->bit2    = 0;
      DataTypePkt->bit34   = 2;
      DataTypePkt->bit56   = 3;
      DataTypePkt->bit78   = 1;
      DataTypePkt->nibble1 = 0xA;
      DataTypePkt->nibble2 = 0x4;
   #endif
      DataTypePkt->bl1 = FALSE;
      DataTypePkt->bl2 = TRUE;
      DataTypePkt->b1  = 16;
      DataTypePkt->b2  = 127;
      DataTypePkt->b3  = 0x7F;
      DataTypePkt->b4  = 0x45;
      DataTypePkt->w1  = 0x2468;
      DataTypePkt->w2  = 0x7FFF;
      DataTypePkt->dw1 = 0x12345678;
      DataTypePkt->dw2 = 0x87654321;
      DataTypePkt->f1  = 90.01;
      DataTypePkt->f2  = .0000045;
      DataTypePkt->df1 = 99.9;
      DataTypePkt->df2 = .4444;

   for (i=0; i < 10; i++) DataTypePkt->str[i] = StringVariable[i];

} /* End InitDataTypePkt() */

/******************************************************************************
** Function: ProcessCommands
**
*/
static void ProcessCommands(void)
{

   int32           Status = CFE_SEVERITY_ERROR;
   CFE_SB_Msg_t*   CmdMsgPtr;
   int             MsgId;
   
   Status = CFE_SB_RcvMsg(&CmdMsgPtr, KitTo.CmdPipe, CFE_SB_POLL);
        
   if (Status == CFE_SUCCESS)
   {

      MsgId = CFE_SB_MsgIdToValue(CFE_SB_GetMsgId(CmdMsgPtr));

      if (MsgId == KitTo.CmdMidValue) 
      {
         CMDMGR_DispatchFunc(CMDMGR_OBJ, CmdMsgPtr);
      }
      else if (MsgId == KitTo.SendHkMidValue)
      {
         EVT_PLBK_Execute();
         SendHousekeepingPkt(&(KitTo.HkPkt));
      }
      else
      {
         CFE_EVS_SendEvent(KIT_TO_APP_INVALID_MID_EID, CFE_EVS_ERROR,
                           "Received invalid command packet,MID = 0x%04X",MsgId);
      }

   } /* End if SB received a packet */

} /* End ProcessCommands() */


