/*
** Purpose: Define configurations for the OpenSatKit Telemetry Output app. 
**
** Notes:
**   1. These configurations should have an application scope and define
**      parameters that shouldn't need to change across deployments. If
**      a change is made to this file or any other app source file during
**      a deployment then the definition of the KIT_TO_PLATFORM_REV
**      macro in kit_to_demo_platform_cfg.h should be updated.
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide and the
**      osk_c_demo app that illustrates best practices with comments.  
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

#ifndef _app_cfg_
#define _app_cfg_

/*
** Includes
*/

#include "cfe_platform_cfg.h"
#include "kit_to_platform_cfg.h"
#include "osk_c_fw.h"


/******************************************************************************
** Versions
**
** 1.1 - Refactored for OSK 2.2 framework changes
** 1.2 - Add statistics, app delay cmd, debug events for SB training, June 2020
** 2.0 - Added packet filtering and dimensioned packet table to accomodate full
**       11-bit AppId range. Reorder command function codes to group pktmgr and
**       app level commands
** 2.1 - Added event log playback
** 3.0 - New baseline for separate OSK app repo compatible with cFE Bootes
*/

#define  KIT_TO_MAJOR_VER     3
#define  KIT_TO_MINOR_VER     0


/******************************************************************************
** JSON init file definitions/declarations.
**    
*/

#define CFG_APP_CFE_NAME           APP_CFE_NAME
#define CFG_APP_PERF_ID            APP_PERF_ID


#define CFG_APP_CMD_PIPE_NAME      APP_CMD_PIPE_NAME
#define CFG_APP_CMD_PIPE_DEPTH     APP_CMD_PIPE_DEPTH

#define CFG_APP_RUN_LOOP_DELAY     APP_RUN_LOOP_DELAY       /* Delay in milliseconds for main loop */
#define CFG_APP_RUN_LOOP_DELAY_MIN APP_RUN_LOOP_DELAY_MIN   /* Minimum command value to set delay  */
#define CFG_APP_RUN_LOOP_DELAY_MAX APP_RUN_LOOP_DELAY_MAX   /* Maximum command value to set delay  */

#define CFG_APP_CMD_MID            APP_CMD_MID
#define CFG_APP_SEND_HK_MID        APP_SEND_HK_MID
#define CFG_APP_HK_TLM_MID         APP_HK_TLM_MID
#define CFG_APP_DATA_TYPE_TLM_MID  APP_DATA_TYPE_TLM_MID
#define CFG_PKTMGR_TLM_MID         PKTMGR_TLM_MID
#define CFG_EVT_PLBK_TLM_MID       EVT_PLBK_TLM_MID

#define CFG_PKTMGR_PIPE_NAME    PKTMGR_PIPE_NAME
#define CFG_PKTMGR_PIPE_DEPTH   PKTMGR_PIPE_DEPTH
#define CFG_PKTMGR_UDP_TLM_PORT PKTMGR_UDP_TLM_PORT

#define CFG_PKTMGR_STATS_INIT_DELAY    PKTMGR_STATS_INIT_DELAY   /* ms after app initialized to start stats computations   */
#define CFG_PKTMGR_STATS_CONFIG_DELAY  PKTMGR_STATS_CONFIG_DELAY /* ms after a reconfiguration to start stats computations */

#define CFG_PKTTBL_LOAD_FILE    PKTTBL_LOAD_FILE
#define CFG_PKTTBL_DUMP_FILE    PKTTBL_DUMP_FILE

#define CFG_EVT_PLBK_HK_PERIOD  EVT_PLBK_HK_PERIOD  /* Number of HK request cycles between event tlm messages */
#define CFG_EVT_PLBK_LOG_FILE   EVT_PLBK_LOG_FILE 

#define CFG_EVT_PLBK_EVS_CMD_MID       EVT_PLBK_EVS_CMD_MID       /* cFE EVS command message ID */
#define CFG_EVT_PLBK_EVS_WRITE_LOG_FC  EVT_PLBK_EVS_WRITE_LOG_FC  /* cFE EVS 'Write Log to File' command function code */ 


#define APP_CONFIG(XX) \
   XX(APP_CFE_NAME,char*) \
   XX(APP_PERF_ID,uint32) \
   XX(APP_CMD_PIPE_NAME,char*) \
   XX(APP_CMD_PIPE_DEPTH,uint32) \
   XX(APP_RUN_LOOP_DELAY,uint32) \
   XX(APP_RUN_LOOP_DELAY_MIN,uint32) \
   XX(APP_RUN_LOOP_DELAY_MAX,uint32) \
   XX(APP_CMD_MID,uint32) \
   XX(APP_SEND_HK_MID,uint32) \
   XX(APP_HK_TLM_MID,uint32) \
   XX(APP_DATA_TYPE_TLM_MID,uint32) \
   XX(PKTMGR_TLM_MID,uint32) \
   XX(EVT_PLBK_TLM_MID,uint32) \
   XX(PKTMGR_PIPE_NAME,char*) \
   XX(PKTMGR_PIPE_DEPTH,uint32) \
   XX(PKTMGR_UDP_TLM_PORT,uint32) \
   XX(PKTMGR_STATS_INIT_DELAY,uint32) \
   XX(PKTMGR_STATS_CONFIG_DELAY,uint32) \
   XX(PKTTBL_LOAD_FILE,char*) \
   XX(PKTTBL_DUMP_FILE,char*) \
   XX(EVT_PLBK_HK_PERIOD,uint32) \
   XX(EVT_PLBK_LOG_FILE,char*) \
   XX(EVT_PLBK_EVS_CMD_MID,uint32) \
   XX(EVT_PLBK_EVS_WRITE_LOG_FC,uint32) \

DECLARE_ENUM(Config,APP_CONFIG)


/******************************************************************************
** Command Macros
*/

#define KIT_TO_PKT_TBL_LOAD_CMD_FC       (CMDMGR_APP_START_FC +  0)
#define KIT_TO_PKT_TBL_DUMP_CMD_FC       (CMDMGR_APP_START_FC +  1)

#define KIT_TO_ADD_PKT_CMD_FC            (CMDMGR_APP_START_FC +  2)
#define KIT_TO_ENABLE_OUTPUT_CMD_FC      (CMDMGR_APP_START_FC +  3)
#define KIT_TO_REMOVE_ALL_PKTS_CMD_FC    (CMDMGR_APP_START_FC +  4)
#define KIT_TO_REMOVE_PKT_CMD_FC         (CMDMGR_APP_START_FC +  5)
#define KIT_TO_SEND_PKT_TBL_TLM_CMD_FC   (CMDMGR_APP_START_FC +  6)
#define KIT_TO_UPDATE_FILTER_CMD_FC      (CMDMGR_APP_START_FC +  7)

#define KIT_TO_SEND_DATA_TYPES_CMD_FC    (CMDMGR_APP_START_FC +  8)
#define KIT_TO_SET_RUN_LOOP_DELAY_CMD_FC (CMDMGR_APP_START_FC +  9)
#define KIT_TO_TEST_FILTER_CMD_FC        (CMDMGR_APP_START_FC + 10)

#define KIT_TO_EVT_PLBK_CONFIG_CMD_FC    (CMDMGR_APP_START_FC + 11)
#define KIT_TO_EVT_PLBK_START_CMD_FC     (CMDMGR_APP_START_FC + 12)
#define KIT_TO_EVT_PLBK_STOP_CMD_FC      (CMDMGR_APP_START_FC + 13)


/******************************************************************************
** Event Macros
**
** Define the base event message IDs used by each object/component used by the
** application. There are no automated checks to ensure an ID range is not
** exceeded so it is the developer's responsibility to verify the ranges. 
*/

#define KIT_TO_APP_BASE_EID  (OSK_C_FW_APP_BASE_EID +   0)
#define PKTTBL_BASE_EID      (OSK_C_FW_APP_BASE_EID + 100)
#define PKTMGR_BASE_EID      (OSK_C_FW_APP_BASE_EID + 200)
#define EVT_PLBK_BASE_EID    (OSK_C_FW_APP_BASE_EID + 300)

/*
** One event ID is used for all initialization debug messages. Uncomment one of
** the KIT_TO_INIT_EVS_TYPE definitions. Set it to INFORMATION if you want to
** see the events during initialization. This is opposite to what you'd expect 
** because INFORMATION messages are enabled by default when an app is loaded.
*/

#define KIT_TO_INIT_DEBUG_EID 999
#define KIT_TO_INIT_EVS_TYPE CFE_EVS_DEBUG
//#define KIT_TO_INIT_EVS_TYPE CFE_EVS_INFORMATION


/******************************************************************************
** evt_plbk.h Configurations
**
** - EVT_PLBK_EVENTS_PER_TLM_MSG is defined here because it impacts the size of 
**   data structures and most deployments should be able to work with this
**   value. Defining it elsewhere could encourage needless changes.
*/

#define EVT_PLBK_EVENTS_PER_TLM_MSG   4  


#endif /* _app_cfg_ */
