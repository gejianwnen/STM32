/** @file
  Platform Stage1 header file

  @copyright
  INTEL CONFIDENTIAL
  Copyright 1999 - 2019 Intel Corporation. <BR>

  The source code contained or described herein and all documents related to the
  source code ("Material") are owned by Intel Corporation or its suppliers or
  licensors. Title to the Material remains with Intel Corporation or its suppliers
  and licensors. The Material may contain trade secrets and proprietary    and
  confidential information of Intel Corporation and its suppliers and licensors,
  and is protected by worldwide copyright and trade secret laws and treaty
  provisions. No part of the Material may be used, copied, reproduced, modified,
  published, uploaded, posted, transmitted, distributed, or disclosed in any way
  without Intel's prior express written permission.

  No license under any patent, copyright, trade secret or other intellectual
  property right is granted to or conferred upon you by disclosure or delivery
  of the Materials, either expressly, by implication, inducement, estoppel or
  otherwise. Any license under such intellectual property rights must be
  express and approved by Intel in writing.

  Unless otherwise agreed by Intel in writing, you may not remove or alter
  this notice or any other notice embedded in Materials by Intel or
  Intel's suppliers or licensors in any way.
**/

#ifndef _EFI_PLATFORM_EARLY_INIT_H_
#define _EFI_PLATFORM_EARLY_INIT_H_

#include <Base.h>
#include <PiPei.h>
#include <Uefi.h>
#include <Ppi/BaseMemoryTest.h>
#include <Ppi/Stall.h>
#include <Ppi/FlashMap.h>
#include <Ppi/PlatformMemorySize.h>
#include <Ppi/BootInRecoveryMode.h>
#include <Ppi/PchPolicy.h>
#include <Ppi/Spi.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/FirmwareVolumeInfo.h>
#include <Ppi/Reset.h>
#include <Ppi/Smbus2.h>
#include <GpioInitData.h>

#include <Guid/PlatformInfo.h>
#include <Guid/SetupVariable.h>
#include <PchSetupVariable.h>
#include <Guid/MeRcVariable.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/PlatformTxt.h>
#include <Guid/BiosId.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PciLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PlatformHooksLib.h>
#include <Library/PreSiliconEnvDetectLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugPrintErrorLevelLib.h>
#include <Library/PrintLib.h>
#include <Library/BiosIdLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PlatformStatusCodes.h>
#include <Library/PchCycleDecodingLib.h>
#include <Library/SetupLib.h>

#include <Cpu/CpuCoreRegs.h>
#include <CpuDataStruct.h>
#include <Platform.h>
#include <Register/PchRegsPcr.h>
#include <Register/PchRegsLpc.h>
#include <Register/PchRegsSpi.h>
#include <PchAccess.h>
#include <Library/MmPciBaseLib.h>
#include <IndustryStandard/Pci22.h>

#include <SioRegs.h>
#include <FlashMap.h>

#include <Protocol/IioUds.h>
#include <Library/UbaBoardSioInfoLib.h>

//
// Used in PEI memory test routines
//
#define MEMORY_TEST_COVER_SPAN  0x40000
#define MEMORY_TEST_PATTERN     0x5A5A5A5A
typedef struct _PEI_FV_INFO_PPI_PRIVATE {
  EFI_PEI_PPI_DESCRIPTOR                  PpiList;
  EFI_PEI_FIRMWARE_VOLUME_INFO_PPI        FvInfoPpi;
} PEI_FV_INFO_PPI_PRIVATE;
//   EDK2_TODO
#define EFI_MAX_ADDRESS   0xFFFFFFFF
#ifndef EFI_DEADLOOP
 #define EFI_DEADLOOP()    { volatile int __iii; __iii = 1; while (__iii); }
#endif

//
// Set The State To Go After G3 defines
//
#define LEAVE_POWER_STATE_UNCHANGED 0xFF

//
// Advanced DFX Jumper defines
//
#define ADV_DEBUG_AUTO 2
#define ADV_DEBUG_ENABLE 1
#define ADV_DEBUG_DISABLE 0

#define DFX_DISABLE  0

//
// Function prototypes for breakpoint callbacks
//
EFI_STATUS
EFIAPI
AfterMrcBreakpoint (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor,
  IN  VOID                        *Ppi
  );

EFI_STATUS
EFIAPI
AfterKtiRcBreakpoint (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor,
  IN  VOID                        *Ppi
  );

EFI_STATUS
EFIAPI
ReadyForIbistBreakpoint (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor,
  IN  VOID                        *Ppi
  );

EFI_STATUS
EFIAPI
AfterFullSpeedSetupBreakpoint (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor,
  IN  VOID                        *Ppi
  );

//
// Function Prototypes
//
EFI_STATUS
PeiPrintPlatformInfo (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR    *NotifyDescriptor,
  IN VOID                         *Ppi
  );

EFI_STATUS
PeiGetBiosId (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN OUT  BIOS_ID_IMAGE           *BiosIdImage
  );

VOID
EFIAPI
InstallFvInfoPpi (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_FV_HANDLE           VolumeHandle
  );

/**

  GC_TODO: Add function description

  @param PeiServices   - GC_TODO: add argument description
  @param This          - GC_TODO: add argument description
  @param BeginAddress  - GC_TODO: add argument description
  @param MemoryLength  - GC_TODO: add argument description
  @param Operation     - GC_TODO: add argument description
  @param ErrorAddress  - GC_TODO: add argument description

  @retval GC_TODO: add return values

**/
EFI_STATUS
EFIAPI
BaseMemoryTest (
  IN  EFI_PEI_SERVICES          **PeiServices,
  IN  PEI_BASE_MEMORY_TEST_PPI  *This,
  IN  EFI_PHYSICAL_ADDRESS      BeginAddress,
  IN  UINT64                    MemoryLength,
  IN  PEI_MEMORY_TEST_OP        Operation,
  OUT EFI_PHYSICAL_ADDRESS      *ErrorAddress
  )
;

VOID
EarlyPlatformPchInit (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN SYSTEM_CONFIGURATION        *SystemConfiguration,
  //
  // PCH_VARIABLE
  // Add the PCH Variable till we move to PCH pkg
  //
  IN PCH_SETUP                   *PchSetup
  );

VOID
Test (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN SYSTEM_CONFIGURATION        *SystemConfiguration,
  IN PCH_SETUP                   *PchSetup
  );

EFI_STATUS
PeimInitializeFlashMap (
  IN EFI_PEI_FILE_HANDLE       *FileHandle,
  IN CONST EFI_PEI_SERVICES           **PeiServices
  );

EFI_STATUS
PeimInstallFlashMapPpi (
  IN EFI_PEI_FILE_HANDLE       *FileHandle,
  IN CONST EFI_PEI_SERVICES           **PeiServices
  );

/**

  GC_TODO: Add function description

  @param PeiServices   - GC_TODO: add argument description
  @param AreaType      - GC_TODO: add argument description
  @param AreaTypeGuid  - GC_TODO: add argument description
  @param NumEntries    - GC_TODO: add argument description
  @param Entries       - GC_TODO: add argument description

  @retval GC_TODO: add return values

**/
EFI_STATUS
EFIAPI
GetAreaInfo (
  IN  EFI_PEI_SERVICES          **PeiServices,
  IN  EFI_FLASH_AREA_TYPE       AreaType,
  IN  EFI_GUID                  *AreaTypeGuid,
  OUT UINT32                    *NumEntries,
  OUT EFI_FLASH_SUBAREA_ENTRY   **Entries
  )
;

/**

  GC_TODO: Add function description

  @param PeiServices       - GC_TODO: add argument description
  @param NotifyDescriptor  - GC_TODO: add argument description
  @param Ppi               - GC_TODO: add argument description

  @retval GC_TODO: add return values

**/
EFI_STATUS
EFIAPI
SiliconRcHobsReadyPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
;

/**
  This function will be called when gReferenceCodePolicyPpiGuid is installed or reinstalled.

  @param  PeiServices General purpose services available to every PEIM.

  @param  NotifyDescriptor Information about the notify event..

  @param  Ppi The notify context.

  @retval EFI_SUCCESS If the function completed successfully.
**/
EFI_STATUS
EFIAPI
ReferenceCodePolicyNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

/**

  GC_TODO: Add function description

  @param PeiServices - GC_TODO: add argument description
  @param This        - GC_TODO: add argument description
  @param MemorySize  - GC_TODO: add argument description

  @retval GC_TODO: add return values

**/
EFI_STATUS
EFIAPI
GetPlatformMemorySize (
  IN      EFI_PEI_SERVICES                       **PeiServices,
  IN      PEI_PLATFORM_MEMORY_SIZE_PPI           *This,
  IN OUT  UINT64                                 *MemorySize
  )
;

EFI_STATUS
EndOfPeiPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

/**

  GC_TODO: Add function description

  @param PeiServices - GC_TODO: add argument description

  @retval GC_TODO: add return values

**/
EFI_STATUS
EFIAPI
PeimInitializeRecovery (
  IN EFI_PEI_SERVICES     **PeiServices
  )
;

VOID
LpcSioEarlyInit (
  IN     PEI_BOARD_SIO_INFO             *BoardSioInfo
  );

EFI_STATUS
ConfigurePlatformClocks (
  IN EFI_PEI_SERVICES                   **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR          *NotifyDescriptor,
  IN VOID                               *SmbusPpi
  );

VOID
GetIioUdsHob (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN IIO_UDS                  **UdsHobPtr
  );

/**
  SpiFlashDescriptorUpdate - Overrides SPI Flash Descriptor content

  @param[in] PeiServices                Pointer to the PeiServices

  @retval EFI_SUCCESS                   FD updated
  @retval EFI_ABORTED                   FD update not needed or not possible due to FD access denied
**/
EFI_STATUS
EFIAPI
SpiFlashDescriptorUpdate(
  IN EFI_PEI_SERVICES            **PeiServices
  );

#endif //_EFI_PLATFORM_EARLY_INIT_H_
