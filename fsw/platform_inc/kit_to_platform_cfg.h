/*
** Purpose: Define platform configurations for the OpenSatKit Telemetry Output
**          application
**
** Notes:
**   1. These definitions should be minimal and only contain parameters that
**      need to be configurable and that must be defined at compile time.  
**      Use app_cfg.h for compile-time parameters that don't need to be
**      configured when an app is deployed and the JSON initialization
**      file for parameters that can be defined at runtime.
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

#ifndef _kit_to_platform_cfg_
#define _kit_to_platform_cfg_

/*
** Includes
*/

#include "kit_to_mission_cfg.h"


/******************************************************************************
** Platform Deployment Configurations
*/

#define KIT_TO_PLATFORM_REV   0
#define KIT_TO_INI_FILENAME   "/cf/kit_to_ini.json"

/******************************************************************************
** Packet Table Configurations
*/

#define PKTTBL_JSON_FILE_MAX_CHAR  16384


#endif /* _kit_to_platform_cfg_ */