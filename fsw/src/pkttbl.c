/*
** Purpose: Implement KIT_TO's Message Table management functions.
**
** Notes:
**   None
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
** Include Files:
*/

#include <string.h>
#include "pkttbl.h"

/***********************/
/** Macro Definitions **/
/***********************/

#define JSON_DATA_WORD_STR_MAX 32


/**********************/
/** Type Definitions **/
/**********************/

/* See LoadJsonData()prologue for details */

typedef CJSON_IntObj_t JsonDecId_t;
typedef CJSON_IntObj_t JsonPriority_t;
typedef CJSON_IntObj_t JsonReliability_t;
typedef CJSON_IntObj_t JsonBufLimit_t;
typedef CJSON_IntObj_t JsonFilterType_t;
typedef CJSON_IntObj_t JsonFilterX_t;
typedef CJSON_IntObj_t JsonFilterN_t;
typedef CJSON_IntObj_t JsonFilterO_t;

typedef struct
{
   JsonDecId_t        DecId;
   JsonPriority_t     Priority;
   JsonReliability_t  Reliability;
   JsonBufLimit_t     BufLimit;
   JsonFilterType_t   FilterType;
   JsonFilterX_t      FilterX;
   JsonFilterN_t      FilterN;
   JsonFilterO_t      FilterO;
     
} JsonPacket_t;

/**********************/
/** Global File Data **/
/**********************/

static PKTTBL_Class_t* PktTbl = NULL;
static PKTTBL_Data_t   TblData;        /* Working buffer for loads */


/******************************/
/** File Function Prototypes **/
/******************************/

static void ConstructJsonPacket(JsonPacket_t* JsonPacket, uint16 PktArrayIdx);
static boolean LoadJsonData(size_t JsonFileLen);
static boolean WriteJsonPkt(int32 FileHandle, const PKTTBL_Pkt_t* Pkt, boolean FirstPktWritten);


/******************************************************************************
** Function: PKTTBL_Constructor
**
** Notes:
**    1. This must be called prior to any other functions
**
*/
void PKTTBL_Constructor(PKTTBL_Class_t*  ObjPtr, const char* AppName,
                        PKTTBL_LoadNewTbl_t LoadNewTbl)
{
   
   PktTbl = ObjPtr;

   CFE_PSP_MemSet(PktTbl, 0, sizeof(PKTTBL_Class_t));
   PKTTBL_SetTblToUnused(&(PktTbl->Data));

   PktTbl->AppName        = AppName;
   PktTbl->LoadNewTbl     = LoadNewTbl;
   PktTbl->LastLoadStatus = TBLMGR_STATUS_UNDEF;
   
} /* End PKTTBL_Constructor() */


/******************************************************************************
** Function: PKTTBL_SetPacketToUnused
**
**
*/
void PKTTBL_SetPacketToUnused(PKTTBL_Pkt_t* PktPtr)
{
   
   CFE_PSP_MemSet(PktPtr, 0, sizeof(PKTTBL_Pkt_t));

   PktPtr->StreamId    = PKTTBL_UNUSED_MSG_ID;
   PktPtr->Filter.Type = PKTUTIL_FILTER_ALWAYS;
   
} /* End PKTTBL_SetPacketToUnused() */


/******************************************************************************
** Function: PKTTBL_SetTblToUnused
**
**
*/
void PKTTBL_SetTblToUnused(PKTTBL_Data_t* TblPtr)
{
  
   uint16 AppId;
   
   CFE_PSP_MemSet(TblPtr, 0, sizeof(PKTTBL_Data_t));

   for (AppId=0; AppId < PKTUTIL_MAX_APP_ID; AppId++)
   {
      
      TblPtr->Pkt[AppId].StreamId    = PKTTBL_UNUSED_MSG_ID;
      TblPtr->Pkt[AppId].Filter.Type = PKTUTIL_FILTER_ALWAYS;
   
   }
   
} /* End PKTTBL_SetTblToUnused() */


/******************************************************************************
** Function: PKTTBL_ResetStatus
**
*/
void PKTTBL_ResetStatus(void)
{
   
   PktTbl->LastLoadStatus = TBLMGR_STATUS_UNDEF;
   PktTbl->LastLoadCnt = 0;
    
} /* End PKTTBL_ResetStatus() */



/******************************************************************************
** Function: PKTTBL_LoadCmd
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager that has verified the file.
*/
boolean PKTTBL_LoadCmd(TBLMGR_Tbl_t* Tbl, uint8 LoadType, const char* Filename)
{

   boolean
   RetStatus = FALSE;

   if (CJSON_ProcessFile(Filename, PktTbl->JsonBuf, PKTTBL_JSON_FILE_MAX_CHAR, LoadJsonData))
   {
      PktTbl->Loaded = TRUE;
      PktTbl->LastLoadStatus = TBLMGR_STATUS_VALID;
      RetStatus = TRUE;
   }
   else
   {
      PktTbl->LastLoadStatus = TBLMGR_STATUS_INVALID;
   }

   return RetStatus;
   
} /* End PKTTBL_LoadCmd() */


/******************************************************************************
** Function: PKTTBL_DumpCmd
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager that has verified the file.
**  3. DumpType is unused.
**  4. File is formatted so it can be used as a load file. It does not follow
**     the cFE table file format. 
**  5. Creates a new dump file, overwriting anything that may have existed
**     previously
*/

boolean PKTTBL_DumpCmd(TBLMGR_Tbl_t* Tbl, uint8 DumpType, const char* Filename)
{

   boolean  RetStatus = FALSE;
   boolean  FirstPktWritten = FALSE;
   int32    FileHandle;
   uint16   AppId;
   char     DumpRecord[256];
   char     SysTimeStr[256];
   
   FileHandle = OS_creat(Filename, OS_WRITE_ONLY);

   if (FileHandle >= OS_FS_SUCCESS)
   {

      sprintf(DumpRecord,"\n{\n\"name\": \"Kit Telemetry Output (KIT_TO) Packet Table\",\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      CFE_TIME_Print(SysTimeStr, CFE_TIME_GetTime());
      
      sprintf(DumpRecord,"\"description\": \"KIT_TO dumped at %s\",\n",SysTimeStr);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));


      /* 
      ** Packet Array 
      **
      ** - Not all fields in ground table are saved in FSW so they are not
      **   populated in the dump file. However, the dump file can still
      **   be loaded.
      */
      
      sprintf(DumpRecord,"\"packet-array\": [\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      
      for (AppId=0; AppId < PKTUTIL_MAX_APP_ID; AppId++)
      {
               
         if (WriteJsonPkt(FileHandle, &(PktTbl->Data.Pkt[AppId]), FirstPktWritten)) FirstPktWritten = TRUE;
              
      } /* End packet loop */

      sprintf(DumpRecord,"\n]}\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      RetStatus = TRUE;

      OS_close(FileHandle);

   } /* End if file create */
   else
   {
   
      CFE_EVS_SendEvent(PKTTBL_CREATE_FILE_ERR_EID, CFE_EVS_ERROR,
                        "Error creating dump file '%s', Status=0x%08X", Filename, FileHandle);
   
   } /* End if file create error */

   return RetStatus;
   
} /* End of PKTTBL_DumpCmd() */


/******************************************************************************
** Function: ConstructJsonPacket
**
*/
static void ConstructJsonPacket(JsonPacket_t* JsonPacket, uint16 PktArrayIdx)
{

   char KeyStr[64];
   
   sprintf(KeyStr,"packet-array[%d].packet.dec-id", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->DecId.Obj, KeyStr, JSONNumber, &JsonPacket->DecId.Value, 4);

   sprintf(KeyStr,"packet-array[%d].packet.priority", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->Priority.Obj, KeyStr, JSONNumber, &JsonPacket->Priority.Value, 4);

   sprintf(KeyStr,"packet-array[%d].packet.reliability", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->Reliability.Obj, KeyStr, JSONNumber, &JsonPacket->Reliability.Value, 4);

   sprintf(KeyStr,"packet-array[%d].packet.buf-limit", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->BufLimit.Obj, KeyStr, JSONNumber, &JsonPacket->BufLimit.Value, 4);

   sprintf(KeyStr,"packet-array[%d].packet.filter.type", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->FilterType.Obj, KeyStr, JSONNumber, &JsonPacket->FilterType.Value, 4);

   sprintf(KeyStr,"packet-array[%d].packet.filter.X", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->FilterX.Obj, KeyStr, JSONNumber, &JsonPacket->FilterX.Value, 4);

   sprintf(KeyStr,"packet-array[%d].packet.filter.N", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->FilterN.Obj, KeyStr, JSONNumber, &JsonPacket->FilterN.Value, 4);

   sprintf(KeyStr,"packet-array[%d].packet.filter.O", PktArrayIdx);
   CJSON_ObjConstructor(&JsonPacket->FilterO.Obj, KeyStr, JSONNumber, &JsonPacket->FilterO.Value, 4);

} /* ConstructJsonPacket() */


/******************************************************************************
** Function: LoadJsonData
**
** Notes:
**  1. The JSON file can contain 1 to PKTUTIL_MAX_APP_ID entries. The table can
**     be sparsely populated. 
**  2. JSON "packet-array" contains the following "packet" object entries
**       {"packet": {
**          "name": "CFE_ES_APP_TLM_MID",  # Not saved
**          "stream-id": "\u080B",         # Not saved
**          "dec-id": 2059,
**          "priority": 0,
**          "reliability": 0,
**          "buf-limit": 4,
**          "filter": { "type": 2, "X": 1, "N": 1, "O": 0}
**       }},
**
*/
static boolean LoadJsonData(size_t JsonFileLen)
{

   boolean  RetStatus = TRUE;
   boolean  ReadPkt = TRUE;
   uint16   AttributeCnt;
   uint16   PktArrayIdx;
   uint16   AppIdIdx;
   
   JsonPacket_t  JsonPacket;
   PKTTBL_Pkt_t  Pkt;


   PktTbl->JsonFileLen = JsonFileLen;

   /* 
   ** 1. Copy table owner data into local table buffer
   ** 2. Process JSON file which updates local table buffer with JSON supplied values
   ** 3. If valid, copy local buffer over owner's data 
   */
   
   memcpy(&TblData, &PktTbl->Data, sizeof(PKTTBL_Data_t));

   PktArrayIdx = 0;
   while (ReadPkt)
   {

      ConstructJsonPacket(&JsonPacket, PktArrayIdx);
      memset((void*)&Pkt,0,sizeof(PKTTBL_Pkt_t));

      /*
      ** Use 'dec-id' field to determine whether processing the file
      ** is complete. A missing or malformed 'dec-id' field error will
      ** not be caught or reported.
      ** The 'dec-id' field is required but CJSON_LoadObjOptional() is
      ** used so the 'object not found' event will be suppressed 
      */      
      
      if (CJSON_LoadObjOptional(&JsonPacket.DecId.Obj, PktTbl->JsonBuf, PktTbl->JsonFileLen))
      {
         
         AppIdIdx = JsonPacket.DecId.Value & PKTTBL_APP_ID_MASK;
         
         if (AppIdIdx < PKTUTIL_MAX_APP_ID)
         {
         
            AttributeCnt = 0;
            if (CJSON_LoadObj(&JsonPacket.Priority.Obj,    PktTbl->JsonBuf, PktTbl->JsonFileLen)) AttributeCnt++;
            if (CJSON_LoadObj(&JsonPacket.Reliability.Obj, PktTbl->JsonBuf, PktTbl->JsonFileLen)) AttributeCnt++;
            if (CJSON_LoadObj(&JsonPacket.BufLimit.Obj,    PktTbl->JsonBuf, PktTbl->JsonFileLen)) AttributeCnt++;
            if (CJSON_LoadObj(&JsonPacket.FilterType.Obj,  PktTbl->JsonBuf, PktTbl->JsonFileLen)) AttributeCnt++;
            if (CJSON_LoadObj(&JsonPacket.FilterX.Obj,     PktTbl->JsonBuf, PktTbl->JsonFileLen)) AttributeCnt++;
            if (CJSON_LoadObj(&JsonPacket.FilterN.Obj,     PktTbl->JsonBuf, PktTbl->JsonFileLen)) AttributeCnt++;
            if (CJSON_LoadObj(&JsonPacket.FilterO.Obj,     PktTbl->JsonBuf, PktTbl->JsonFileLen)) AttributeCnt++;
            
            if (AttributeCnt == 7)
            {
               
               Pkt.StreamId        = (CFE_SB_MsgId_t) JsonPacket.DecId.Value;
               Pkt.Qos.Priority    = JsonPacket.Priority.Value;
               Pkt.Qos.Reliability = JsonPacket.Reliability.Value;
               Pkt.BufLim          = JsonPacket.BufLimit.Value;
               Pkt.Filter.Type     = JsonPacket.FilterType.Value;
               Pkt.Filter.Param.X  = JsonPacket.FilterX.Value; 
               Pkt.Filter.Param.N  = JsonPacket.FilterN.Value; 
               Pkt.Filter.Param.O  = JsonPacket.FilterO.Value; 
                              
               memcpy(&TblData.Pkt[AppIdIdx],&Pkt,sizeof(PKTTBL_Pkt_t));
               
            } /* End if valid attributes */
            else
            {
               CFE_EVS_SendEvent(PKTTBL_LOAD_ERR_EID, CFE_EVS_ERROR,
                                 "Packet[%d] has mising attributes, only %d of 7 defined",
                                 PktArrayIdx, AttributeCnt);
               ReadPkt = FALSE;
               RetStatus = FALSE;
            }
         } /* End if valid ID */
         else
         {
            CFE_EVS_SendEvent(PKTTBL_LOAD_ERR_EID, CFE_EVS_ERROR,
                              "Packet[%d]'s dec-id %d has an invlaid app-id value of %d. Valid range is 0 to %d",
                              PktArrayIdx, JsonPacket.DecId.Value, AppIdIdx, (PKTUTIL_MAX_APP_ID-1));
         }

         PktArrayIdx++;
         
      } /* End if 'dec-id' */
      else
      {
         ReadPkt = FALSE;
      }
      
   } /* End ReadPkt */
   
   if (PktArrayIdx == 0)
   {
      CFE_EVS_SendEvent(PKTTBL_LOAD_ERR_EID, CFE_EVS_ERROR,
                        "JSON table file has no message entries");
   }
   else
   {
      if (RetStatus == TRUE)
      {
         PktTbl->LoadNewTbl(&TblData);
         PktTbl->LastLoadCnt = PktArrayIdx;
         CFE_EVS_SendEvent(PKTTBL_LOAD_EID, CFE_EVS_INFORMATION,
                           "Packet Table load updated %d entries", PktArrayIdx);
      }
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */


/******************************************************************************
** Function: WriteJsonPkt
**
** Notes:
**   1. Can't end last record with a comma so logic checks that commas only
**      start to be written after the first packet has been written
*/
static boolean WriteJsonPkt(int32 FileHandle, const PKTTBL_Pkt_t* Pkt, boolean FirstPktWritten)
{
   
   boolean PktWritten = FALSE;
   char DumpRecord[256];

   if (CFE_SB_MsgIdToValue(Pkt->StreamId) != CFE_SB_MsgIdToValue(PKTTBL_UNUSED_MSG_ID))
   {
      
      if (FirstPktWritten)
      {
         sprintf(DumpRecord,",\n");
         OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      }
      
      sprintf(DumpRecord,"\"packet\": {\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      sprintf(DumpRecord,"   \"dec-id\": %d,\n   \"priority\": %d,\n   \"reliability\": %d,\n   \"buf-limit\": %d,\n",
              CFE_SB_MsgIdToValue(Pkt->StreamId), Pkt->Qos.Priority, Pkt->Qos.Reliability, Pkt->BufLim);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      
      sprintf(DumpRecord,"   \"filter\": { \"type\": %d, \"X\": %d, \"N\": %d, \"O\": %d}\n}",
              Pkt->Filter.Type, Pkt->Filter.Param.X, Pkt->Filter.Param.N, Pkt->Filter.Param.O);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
   
      PktWritten = TRUE;
      
   } /* End if StreamId record has been loaded */
   
   return PktWritten;
   
} /* End WriteJsonPkt() */


