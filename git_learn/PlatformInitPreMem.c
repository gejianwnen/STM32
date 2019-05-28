/** @file
  Do platform specific stage1 initializations.

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

#include "PlatformInitPreMem.h"

#include <Ppi/BaseMemoryTest.h>
#include <Ppi/FlashMap.h>
#include <Ppi/SmbusPolicy.h>
#include <Ppi/MasterBootMode.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Library/CmosAccessLib.h>
#include <Library/PeiPolicyInitLib.h>
#include <Library/PeiPlatformHooklib.h>
#include <FirmwareConfigurations.h>
#include <Guid/GlobalVariable.h>
#include <Library/TimerLib.h>
#include <Register/PchRegsPmc.h>
#include <Library/PchCycleDecodingLib.h>
#include <Register/PchRegsLpc.h>
#include <Ppi/PchReset.h>
#include <Ppi/Spi.h>
#include <Guid/TcgDataHob.h>
#include <Guid/AdminPasswordGuid.h>
#include <Guid/SetupBreakpointGuid.h>
#include <Guid/SetupVariable.h>
#include <PchSetupVariable.h>
#include <Library/PmcLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <PchResetPlatformSpecific.h>
#include <Guid/SocketVariable.h>
#if defined(ME_WS_SUPPORT) && ME_WS_SUPPORT
#include "Library/MeTypeLib.h"
#endif //ME_WS_SUPPORT
#include <Library/PeiIioSiPolicyLib.h>
#include <Library/PeiIioSiPolicyUpdateLib.h>
#include <Library/UbaGpioPlatformConfig.h>
#include <Library/TestPointCheckLib.h>

#include <ReferenceCodePolicy.h>
#include <Library/UbaPlatLib.h>
#include <Library/UbaClocksConfigLib.h>

//
// RestrictedBegin
//
#include <IndustryStandard/I440FxPiix4.h>
#if defined(SV_HOOKS)
#include "SvConfiguration.h"
#include "Guid\SvOtherVariable.h"
#include "MerlinxPlatformApi.h"
#endif
//
// RestrictedEnd
//

extern EFI_GUID gEfiBiosIdGuid;
extern EFI_GUID gEfiSiliconRcHobsReadyPpi;
EFI_GUID  gEfiAfterMrcGuid            = EFI_AFTER_MRC_GUID;
EFI_GUID  gEfiAfterFullSpeedSetupGuid = EFI_AFTER_FULL_SPEED_SETUP_GUID;
EFI_GUID  gEfiReadyForIbistGuid       = EFI_READY_FOR_IBIST_GUID;

EFI_STATUS
EFIAPI
PlatformInitPreMem (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor,
  IN VOID                        *Ppi
  );

EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor,
  IN VOID                        *Ppi
  );

static PEI_BASE_MEMORY_TEST_PPI     mPeiBaseMemoryTestPpi = { BaseMemoryTest };

static PEI_PLATFORM_MEMORY_SIZE_PPI mMemoryMemorySizePpi  = { GetPlatformMemorySize };

static EFI_PEI_PPI_DESCRIPTOR       mPpiList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gPeiBaseMemoryTestPpiGuid,
    &mPeiBaseMemoryTestPpi
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gPeiPlatformMemorySizePpiGuid,
    &mMemoryMemorySizePpi
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMasterBootModePpiGuid,
    NULL
  }
};

static EFI_PEI_NOTIFY_DESCRIPTOR mPreMemNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiReadOnlyVariable2PpiGuid,
  (EFI_PEIM_NOTIFY_ENTRY_POINT) PlatformInitPreMem
};

static EFI_PEI_NOTIFY_DESCRIPTOR mMemDiscoveredNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMemoryDiscoveredPpiGuid,
  (EFI_PEIM_NOTIFY_ENTRY_POINT) MemoryDiscoveredPpiNotifyCallback
};

static EFI_PEI_NOTIFY_DESCRIPTOR    mNotifyList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiEndOfPeiSignalPpiGuid,
    EndOfPeiPpiNotifyCallback
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiPeiSmbus2PpiGuid,
    ConfigurePlatformClocks
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiPeiSmbus2PpiGuid,
    PeiPrintPlatformInfo
  },
  { EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiAfterMrcGuid,
    AfterMrcBreakpoint
  },
  { EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiAfterKtiRcGuid,
    AfterKtiRcBreakpoint
  },
  { EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiAfterFullSpeedSetupGuid,
    AfterFullSpeedSetupBreakpoint
  },
  { EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiReadyForIbistGuid,
    ReadyForIbistBreakpoint
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiSiliconRcHobsReadyPpi,
    SiliconRcHobsReadyPpiNotifyCallback
  }
};

CONST EFI_PEI_NOTIFY_DESCRIPTOR    ReferenceCodePolicyNotifyDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gReferenceCodePolicyPpiGuid,
  ReferenceCodePolicyNotifyCallback
};

//
// PPV:InternalOnlyBegin
//
#ifdef SV_HOOKS
EFI_GUID gMegaBlockGuid                 = { 0x7318402F, 0xECAF, 0x4B25, { 0x8C, 0x11, 0xB7, 0xA0, 0xC9, 0x8A, 0x01, 0xE9 } };
/**
 * Creates the Mega-block hob
 *
 * @param SetupData pointer to setup data structure
 *
 * @return VOID
 */

VOID *
EFIAPI
GetVariableAndSize (
  IN  CHAR16              *Name,
  IN  EFI_GUID            *Guid,
  IN  UINTN               BufferSize
  );

VOID
CreateValidationMegaBlockHob(
  SETUP_DATA *SetupData
  )
{
  VALIDATION_MEGA_BLOCK        *ValidationMegaBlock;
  SV_OTHER_CONFIGURATION       *SvConfiguration;
  EFI_GUID                     SvGuid = SV_OTHER_CONFIGURATION_GUID;

  SvConfiguration = (SV_OTHER_CONFIGURATION*)GetVariableAndSize (
                                               SV_OTHER_CONFIGURATION_NAME,
                                               &SvGuid,
                                               sizeof(SV_OTHER_CONFIGURATION));
  ValidationMegaBlock = (VALIDATION_MEGA_BLOCK *) AllocateZeroPool (sizeof(VALIDATION_MEGA_BLOCK));
  if (ValidationMegaBlock == NULL) {
    DEBUG ((DEBUG_INFO, "ValidationMegaBlockEntry:: Memmory allocation for MegaBlock HOB has failed, no validation block will be set\n"));
  } else {
    ///
    /// Setting the required Mega Block sizes
    ///
    ValidationMegaBlock->AboveOneMega     = SetupData->SystemConfig.ValidationMegaBlockAboveOneMb;
    ValidationMegaBlock->AboveFourGiga    = SetupData->SystemConfig.ValidationMegaBlockAboveFourGb;
    ValidationMegaBlock->BelowOneMega     = SetupData->SystemConfig.ValidationMegaBlockBelowOneMb;
    ValidationMegaBlock->CoreMegaSupport  = SetupData->SystemConfig.CoreMegaBlock;
    ValidationMegaBlock->SvBootMode       = SvConfiguration->SvBootMode;

    BuildGuidDataHob (
        &gMegaBlockGuid,
        (VOID *) ValidationMegaBlock,
        sizeof(VALIDATION_MEGA_BLOCK)
    );
    DEBUG ((DEBUG_INFO, "ValidationMegaBlockEntry:: HOB was constructed\n"));
  }
}
#endif // SV_HOOKS
//
// PPV:InternalOnlyEnd
//

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
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR    *NotifyDescriptor,
  IN VOID                         *Ppi
  )
{
  EFI_STATUS                      Status;
  SETUP_DATA                      *SetupDataPtr;
  EFI_PEI_PPI_DESCRIPTOR          *ReferenceCodePolicyPpiDescPtr;
  VOID                            *ReferenceCodePolicyPtr;

  DEBUG ((EFI_D_INFO, "\n\nEnter ReferenceCodePolicyNotifyCallback()!\n"));

  SetupDataPtr = (SETUP_DATA *) AllocateZeroPool (sizeof (SETUP_DATA));
  if (SetupDataPtr == NULL) {
    DEBUG ((EFI_D_ERROR, "AllocateZeroPool for SetupDataPtr failed!\n"));
    ASSERT (SetupDataPtr != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = GetEntireConfig (SetupDataPtr);
  if (EFI_ERROR (Status)) {
    FreePool ((VOID *)SetupDataPtr);
    DEBUG ((EFI_D_ERROR, "GetEntireConfig failed! Error = %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //
  // Locate the ReferenceCodePolicyPpi
  //
  Status = PeiServicesLocatePpi (
            &gReferenceCodePolicyPpiGuid,
            0,
            &ReferenceCodePolicyPpiDescPtr,
            &ReferenceCodePolicyPtr
            );
  if (EFI_ERROR (Status)) {
    FreePool ((VOID *)SetupDataPtr);
    DEBUG ((EFI_D_ERROR, "Failed to locate the ReferenceCodePolicyPpi! Error = %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }
  DEBUG ((EFI_D_INFO, "In ReferenceCodePolicyNotifyCallback(): Locate ReferenceCodePolicyPpi successfully!\n"));

  ((REFERENCE_CODE_POLICY *) ReferenceCodePolicyPtr)->NumaEn = SetupDataPtr->SocketConfig.CommonRcConfig.NumaEn;
  ((REFERENCE_CODE_POLICY *) ReferenceCodePolicyPtr)->UmaBasedClustering = SetupDataPtr->SocketConfig.CommonRcConfig.UmaBasedClustering;

  //
  // Print the platform settings for Reference Code Policy
  //
  DEBUG ((EFI_D_ERROR, ">>> Print the platform settings for Reference Code Policy! Begin >>>\n"));
  DEBUG ((EFI_D_ERROR, "ReferenceCodePolicyPtr->NumaEn = %X\n", ((REFERENCE_CODE_POLICY *) ReferenceCodePolicyPtr)->NumaEn));
  DEBUG ((EFI_D_ERROR, "ReferenceCodePolicyPtr->UmaBasedClustering = %X\n", ((REFERENCE_CODE_POLICY *) ReferenceCodePolicyPtr)->UmaBasedClustering));
  DEBUG ((EFI_D_ERROR, "<<< Print the platform settings for Reference Code Policy! End   <<<\n"));

  //
  // Free the allocated pool for SetupData
  //
  FreePool ((VOID *)SetupDataPtr);

  DEBUG ((EFI_D_INFO, "Exit ReferenceCodePolicyNotifyCallback()!\n\n"));

  return EFI_SUCCESS;
}

/**

  Print platform information such as Bios ID and platform Type in PEI.

  @param **PeiServices - Pointer to the list of PEI services
  @param *NotifyDescriptor - The notification structure this PEIM registered on install
  @param *Ppi - Pointer to the PPI

  @retval EFI_SUCCESS -  Function has completed successfully.


**/
EFI_STATUS
PeiPrintPlatformInfo (
  IN EFI_PEI_SERVICES                   **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR          *NotifyDescriptor,
  IN VOID                               *Ppi
  )
{
  EFI_STATUS                  Status;
  EFI_PLATFORM_INFO           *PlatformInfo;
  EFI_HOB_GUID_TYPE           *GuidHob;
  BIOS_ID_IMAGE               BiosIdImage;
  //
  // Search for the Platform Info PEIM GUID HOB.
  //
  GuidHob = GetFirstGuidHob (&gEfiPlatformInfoGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }
  PlatformInfo = GET_GUID_HOB_DATA(GuidHob);

  DEBUG ((EFI_D_ERROR, "Platform Type = %d\n", PlatformInfo->BoardId));

  Status = PeiGetBiosId(PeiServices, &BiosIdImage);

  if (!EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "Bios ID: %s\n", (CHAR16 *) (&(BiosIdImage.BiosIdString))));
  }

  return EFI_SUCCESS;
}

/**

  Configure the clock generator to enable free-running operation.  This keeps
  the clocks from being stopped when the system enters C3 or C4.

  @param None

  @retval EFI_SUCCESS    The function completed successfully.

**/
EFI_STATUS
ConfigurePlatformClocks (
  IN EFI_PEI_SERVICES                   **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR          *NotifyDescriptor,
  IN VOID                               *SmbusPpi
  )
{
  EFI_STATUS                            Status;
  Status = ConfigurePlatformClock (PeiServices, NotifyDescriptor, SmbusPpi);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "Fail to Configure Platform Clocks\n"));
  }
  return EFI_SUCCESS;
}

/**

  Find the BIOS ID from the BIOS ID binary included in the flash

  @param **PeiServices - Pointer to the list of PEI services
  @param *BiosIdImage  - Pointer to the BIOS ID structure being populated

  @retval EFI_NOT_FOUND - Failed to find the Bios ID binary.
  @retval EFI_SUCCESS   - Found the BIOS ID binary.


**/
EFI_STATUS
PeiGetBiosId (
  IN EFI_PEI_SERVICES       **PeiServices,
  IN OUT  BIOS_ID_IMAGE     *BiosIdImage
  )
{
  EFI_STATUS                  Status;
  VOID                        *Address;
  UINTN                       Size;
  EFI_PEI_FV_HANDLE           VolumeHandle;
  EFI_PEI_FILE_HANDLE         FileHandle;
  EFI_FFS_FILE_HEADER         *FileHeader;
  UINTN                       Instance          = 0;
  BOOLEAN                     BiosIdBinaryFound = FALSE;

  //
  // Find BiosId Binary - First loop for Volume Handle
  //
  while (TRUE) {
    VolumeHandle = NULL;
    Status = (*PeiServices)->FfsFindNextVolume (PeiServices, Instance++, &VolumeHandle);

    if (EFI_ERROR(Status)) {
      return Status;
    }

    FileHandle = NULL;
    //
    // Second loop for FileHandle
    //
    while (TRUE) {
      Status = (*PeiServices)->FfsFindNextFile (PeiServices, EFI_FV_FILETYPE_FREEFORM, VolumeHandle, &FileHandle);
      // According to PI spec, FileHandle is FileHeader.
      FileHeader = (EFI_FFS_FILE_HEADER *)FileHandle;
      if (EFI_ERROR (Status)){
        break;
      }
      //
      // Compare GUID to find BIOS ID FFS
      //
      if (CompareGuid (&(FileHeader->Name), &gEfiBiosIdGuid)) {
        BiosIdBinaryFound = TRUE;
        break;
      }
    }//end of second loop
    //
    //  If the binary was found, break Volume loop
    //
    if(BiosIdBinaryFound) {
      break;
    }
  }//end of first loop

  Status = (*PeiServices)->FfsFindSectionData (
            PeiServices,
            EFI_SECTION_RAW,
            FileHandle,
            &Address
            );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Size = sizeof (BIOS_ID_IMAGE);

  (*PeiServices)->CopyMem ((VOID *) BiosIdImage, Address, Size);

  return EFI_SUCCESS;

}

#ifdef LT_FLAG
/**

  Finds the address of a FIT Record in the bootblock region of bios.

  @param FITRecordAddressInFit = The address of the FIT Record (in the FIT table) if one exists, or 0 otherwise.
  @param FITRecordAddress      = The address of the FIT REcord if one exists, or 0 otherwise.

  @retval Nothing.

**/
BOOLEAN
FindFITRecord (
  IN    UINT8          Type,
  OUT   UINTN          *FITRecordAddressInFit,
  OUT   UINTN          *FITRecordAddress
)
{
  UINTN     *FITAddressPtr;
  UINT8     *FITAddress;
  UINT16     NumberOfFitEntries;
  BOOLEAN   OkAddress;
  UINTN     FITAddressTmp;
  BOOLEAN   FindValidFit;

  OkAddress = FALSE;
  FindValidFit = FALSE;

  FITAddressPtr = (UINTN *) 0xFFFFFFC0;      // first try the defualt address for bios that supports only WSM CPU family
  FITAddress = (UINT8 *) (* FITAddressPtr);
  if (FITAddress == (UINT8 *)0xFFFFFFFF) { // Bad Fit pointer - do not try to dereference
    *FITRecordAddressInFit = (UINTN) 0;
    *FITRecordAddress = (UINTN) 0;
    return FindValidFit;
  }


  if (  ((FITAddress[0])=='_') && ((FITAddress[1])=='F') && ((FITAddress[2])=='I') && ((FITAddress[3])=='T') && ((FITAddress[4])=='_')  ) {
      OkAddress = TRUE;
    }

  if (!OkAddress) {
    FITAddressPtr = (UINTN *) 0xFFFFFFE8;      // now try the defualt address for bios that supports only the special NHM C0 fused CPU
    FITAddress = (UINT8 *) (* FITAddressPtr);
    if (  ((FITAddress[0])=='_') && ((FITAddress[1])=='F') && ((FITAddress[2])=='I') && ((FITAddress[3])=='T') && ((FITAddress[4])=='_')  ) {
        OkAddress = TRUE;
      }
    }

  if (OkAddress) {
    OkAddress = FALSE;
    NumberOfFitEntries = (UINT16) FITAddress[8];
    while (NumberOfFitEntries) {
      if ( FITAddress[0x0E] == Type ) {
        FITAddressTmp = (UINTN) FITAddress[3];
        FITAddressTmp = FITAddressTmp << 8;
        FITAddressTmp |= (UINTN) FITAddress[2];
        FITAddressTmp = FITAddressTmp << 8;
        FITAddressTmp |= (UINTN) FITAddress[1];
        FITAddressTmp = FITAddressTmp << 8;
        FITAddressTmp |= (UINTN) FITAddress[0];
        FITAddressTmp =  FITAddressTmp;
        OkAddress = TRUE;
        FindValidFit = TRUE;
        FITAddressPtr = (UINTN *) FITAddressTmp;
        break;
      }
      FITAddress += 0x10;               // move to the next entry in the FIT table
      NumberOfFitEntries --;
    }
  }

  if (!OkAddress) {
    // specified FIT entry type not found in FIT table
    FITAddress = 0;
    FITAddressPtr = 0;
  }

  *FITRecordAddressInFit = (UINTN) FITAddress;
  *FITRecordAddress = (UINTN) FITAddressPtr;

  return FindValidFit;
}
#endif

/**

  Initialize POC register by Variable.

  @param *SystemConfiguration  -  Pointer to SystemConfiguration variables.

  @retval EFI_SUCCESS  -  Success.

**/
EFI_STATUS
UpdatePlatformInfo (
  IN   SYSTEM_CONFIGURATION               *SystemConfiguration,
  IN   SOCKET_CONFIGURATION               *SocketConfiguration
  )
{
  EFI_PLATFORM_INFO *PlatformInfo;
  EFI_HOB_GUID_TYPE *GuidHob;
  SOCKET_PROCESSORCORE_CONFIGURATION *SocketProcessorCoreConfig;
  SOCKET_IIO_CONFIGURATION *SocketIioConfig;
  UINT32 TsegSizeValue;

  SocketProcessorCoreConfig = &SocketConfiguration->SocketProcessorCoreConfiguration;
  SocketIioConfig = &SocketConfiguration->IioConfig;

  //
  // Update the PCIE base and 32/64bit PCI resource support
  //
  GuidHob       = GetFirstGuidHob (&gEfiPlatformInfoGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }
  PlatformInfo  = GET_GUID_HOB_DATA(GuidHob);

  PlatformInfo->SysData.SysIoApicEnable       = PCH_IOAPIC;
#if MAX_SOCKET <= 4
  if (SocketIioConfig->DevPresIoApicIio[0]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC00_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[1]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC01_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[2]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC02_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[3]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC03_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[4]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC04_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[5]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC05_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[6]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC06_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[7]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC07_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[8]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC08_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[9]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC09_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[10]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC10_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[11]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC11_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[12]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC12_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[13]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC13_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[14]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC14_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[15]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC15_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[16]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC16_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[17]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC17_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[18]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC18_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[19]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC19_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[20]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC20_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[21]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC21_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[22]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC22_IOAPIC);
  }
  if (SocketIioConfig->DevPresIoApicIio[23]) {
    PlatformInfo->SysData.SysIoApicEnable |= (PC23_IOAPIC);
  }
#else
  // Enable all 32 IOxAPIC
  PlatformInfo->SysData.SysIoApicEnable = 0xFFFFFFFF;
#endif
  //
  // Check to make sure TsegSize is in range, if not use default.
  //
  if (SocketProcessorCoreConfig->TsegSize > MAX_PROCESSOR_TSEG) {
    SocketProcessorCoreConfig->TsegSize = MAX_PROCESSOR_TSEG; // if out of range make default 64M
  }

  TsegSizeValue = MAX_PROCESSOR_TSEG;
//
// CPU:RestrictedBegin
//
  TsegSizeValue = SocketProcessorCoreConfig->TsegSize;
//
// CPU:RestrictedEnd
//
  PlatformInfo->MemData.MemTsegSize = (0x400000 << TsegSizeValue);
  PlatformInfo->MemData.MemIedSize = PcdGet32 (PcdCpuIEDRamSize);

  //
  // Minimum SMM range in TSEG should be larger than 3M
  //
  ASSERT (PlatformInfo->MemData.MemTsegSize - PlatformInfo->MemData.MemIedSize >= 0x300000);

#ifdef LT_FLAG
  PlatformInfo->MemData.MemLtMemSize  = LT_PLATFORM_DPR_SIZE + LT_DEV_MEM_SIZE;
  PlatformInfo->MemData.MemDprMemSize = LT_PLATFORM_DPR_SIZE;
#endif
  return EFI_SUCCESS;
}

/**
  Install Firmware Volume Hob's once there is main memory

  @param[in]  PeiServices       General purpose services available to every PEIM.
  @param[in]  NotifyDescriptor  Notify that this module published.
  @param[in]  Ppi               PPI that was installed.

  @retval     EFI_SUCCESS       The function completed successfully.
**/
EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                    Status;
  EFI_BOOT_MODE                 BootMode;

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  ///
  /// If S3 resume, then we are done
  ///
  if (BootMode == BOOT_ON_S3_RESUME) {
    return EFI_SUCCESS;
  }

  TestPointMemoryDiscovered ();

  return Status;
}

/**
  This function handles PlatformInit task after PeiReadOnlyVariable2 PPI produced

  @param[in]  PeiServices  Pointer to PEI Services Table.
  @param[in]  NotifyDesc   Pointer to the descriptor for the Notification event that
                           caused this function to execute.
  @param[in]  Ppi          Pointer to the PPI data associated with this function.

  @retval     EFI_SUCCESS  The function completes successfully
  @retval     others
**/
EFI_STATUS
EFIAPI
PlatformInitPreMem (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                        Status;
  Status = EFI_SUCCESS;

  TestPointDebugInitDone ();

  return Status;
}

/**

  Platform specific initializations in stage1.

  @param FileHandle         Pointer to the PEIM FFS file header.
  @param PeiServices        General purpose services available to every PEIM.

  @retval EFI_SUCCESS       Operation completed successfully.
  @retval Otherwise         Platform initialization failed.
**/
EFI_STATUS
EFIAPI
PlatformInitPreMemEntry (
  IN EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  EFI_STATUS                            Status;
  EFI_STATUS                            SetupDataStatus;
  SETUP_DATA                            SetupData;
  EFI_BOOT_MODE                         BootMode = BOOT_WITH_FULL_CONFIGURATION;
  UINT32                                SerialDebugMsgLevelSetting;
#ifdef LT_FLAG
  EFI_PLATFORM_TXT_POLICY_DATA          PlatformTxtPolicyData;
  UINTN                                 StartupAcmAddressInFit;
  UINTN                                 StartupAcmAddress;
  BOOLEAN                               FoundFit;
  UINTN                                 FailSafeAddressInFit=0;
  UINTN                                 FailSafeAddress=0;
  UINTN                                 LcpSize=0;
#endif
  UINT32                                DfxDebugJumperValue = ADV_DEBUG_AUTO;
  EFI_PLATFORM_INFO                     *PlatformInfo;
  EFI_HOB_GUID_TYPE                     *GuidHob;
  UINT16                                ABase;
  UINT16                                Pm1Sts = 0;
  UINT32                                Pm1Cnt;
  PEI_BOARD_SIO_INFO                    BoardSioInfoData;
  UINT8                                 DfxEnable;
#ifndef SKXD_EN
#if ME_SUPPORT_FLAG
  UINTN                                 Size;
#endif
#endif

  //
  // Report Status Code EFI_CU_CHIPSET_PLATFORM_PEIM_INIT
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_CHIPSET_PLATFORM_PEIM_INIT)
    );

//
// RestrictedBegin
//
  Status = PcdSet16S (PcdOvmfHostBridgePciDevId, INTEL_82441_DEVICE_ID);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Platform failed to init the PCD: PcdOvmfHostBridgePciDevId!\n"));
    ASSERT_EFI_ERROR (Status);
  }
//
// RestrictedEnd
//

  //
  // Get Setup Data
  //
  SetupDataStatus = GetEntireConfig (&SetupData);
  ASSERT_EFI_ERROR (SetupDataStatus);

  PlatformGetBoardSioInfo (&BoardSioInfoData);
  //
  // Configure SIO
  //
  LpcSioEarlyInit (&BoardSioInfoData);

  //
  // Configure GPIO
  //
  GuidHob       = GetFirstGuidHob (&gEfiPlatformInfoGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }
  PlatformInfo  = GET_GUID_HOB_DATA(GuidHob);
  Status = BoardInit (PlatformInfo);
  ASSERT_EFI_ERROR (Status);

  //
  // Check AdvDebug Jumper if set to Auto, otherwise, use Enabled/Disabled value
  //
  if ( SetupData.SystemConfig.DfxAdvDebugJumper == ADV_DEBUG_AUTO) {
    Status = GpioGetDfxPadVal (&DfxDebugJumperValue);
  } else {
    DfxDebugJumperValue = SetupData.SystemConfig.DfxAdvDebugJumper;
  }
  Status = PcdSetBoolS(PcdDfxAdvDebugJumper, (BOOLEAN)DfxDebugJumperValue);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  DEBUG ((EFI_D_INFO, "PcdDfxAdvDebugJumper: %d\n", PcdGetBool (PcdDfxAdvDebugJumper)));

  //
  // Set Serial Debug Level
  //
  if((EFI_ERROR (SetupDataStatus) )) {
    // If first boot, setup variable not yet set, so use default debug print level.
    SerialDebugMsgLevelSetting = PcdGet32(PcdDebugPrintErrorLevel);
  } else {
    switch(SetupData.SystemConfig.serialDebugMsgLvl) {
    case 0:
      // Disabled
      SerialDebugMsgLevelSetting = 0x00000000;
      break;
    case 1:
      // Error only
      SerialDebugMsgLevelSetting = EFI_D_ERROR;
      break;
    case 2:
      // Normal
      SerialDebugMsgLevelSetting = PcdGet32(PcdDebugPrintErrorLevel);
      break;
    case 3:
      // Maximum
      SerialDebugMsgLevelSetting = (EFI_D_VERBOSE | EFI_D_INIT | EFI_D_WARN | EFI_D_LOAD | EFI_D_FS | EFI_D_INFO | EFI_D_VARIABLE |
                                    EFI_D_BM | EFI_D_BLKIO | EFI_D_NET | EFI_D_UNDI |
                                    EFI_D_LOADFILE | EFI_D_EVENT | EFI_D_ERROR);
      break;
    case 4:
    default:
      // Auto
      if (PcdGetBool(PcdDfxAdvDebugJumper)) {
        DEBUG((EFI_D_INFO, "Advanced Debug Jumper set - Set Debug Msg Level to Normal\n"));
        SerialDebugMsgLevelSetting = PcdGet32(PcdDebugPrintErrorLevel);
      } else {
        // Error only
        SerialDebugMsgLevelSetting = EFI_D_ERROR;
      }
      break;
    }
  }
  SetDebugPrintErrorLevel(SerialDebugMsgLevelSetting);

  Status = PcdSet8S (PcdDfxRstCplBitsEn, SetupData.SocketConfig.CommonRcConfig.DfxRstCplBitsEn);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Failed to set platform setting to PcdDfxRstCplBitsEn !!! Current PcdDfxRstCplBitsEn = %X\n", PcdGet8 (PcdDfxRstCplBitsEn)));
    ASSERT_EFI_ERROR (Status);
  }

  Status = PcdSet8S (PcdDfxB2PErrAssertAtCommand, SetupData.SocketConfig.CommonRcConfig.DfxB2PErrAssertAtCommand);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Failed to set platform setting to PcdDfxB2PErrAssertAtCommand !!! Current PcdDfxB2PErrAssertAtCommand = %X\n", PcdGet8 (PcdDfxB2PErrAssertAtCommand)));
  }

  Status = PcdSetBoolS (PcdDfxWarmResetEliminationEn, (BOOLEAN) SetupData.SystemConfig.DfxWarmResetEliminationEn);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR,
            "Failed to set platform setting to PcdDfxWarmResetEliminationEn !!! Current PcdDfxWarmResetEliminationEn = %X\n",
            PcdGetBool (PcdDfxWarmResetEliminationEn)));
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Register notify callback function on the gReferenceCodePolicyPpiGuid installation
  //
  DEBUG ((EFI_D_INFO, "\nRegister notify callback function on gReferenceCodePolicyPpiGuid installation!\n"));
  Status = PeiServicesNotifyPpi (&ReferenceCodePolicyNotifyDesc);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Register notify callback function on gReferenceCodePolicyPpiGuid installation failed! Error = %r\n", Status));
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Do Early PCH init
  //
  EarlyPlatformPchInit ((EFI_PEI_SERVICES**)PeiServices, &SetupData.SystemConfig, &SetupData.PchSetup);
  Test ((EFI_PEI_SERVICES**)PeiServices, &SetupData.SystemConfig, &SetupData.PchSetup);
  PeiPolicyInitPreMem (PeiServices, SetupData.PchSetup.FirmwareConfiguration);
  //
  // Do Silicon Policy init
  //
  PeiSiPolicyInitPreMem (PeiServices);

  //
  // Update SPI Flash Descriptor if necessary
  //
  SpiFlashDescriptorUpdate((EFI_PEI_SERVICES**)PeiServices);

  ///
  /// Set what state (S0/S5) to go to when power is re-applied after a power failure (G3 state)
  ///
  PmcSetPlatformStateAfterPowerFailure (SetupData.PchSetup.StateAfterG3);

  CreateDefaultIioSiPolicy ();

  //
  // Check PWR FLR
  //
  if (PmcIsPowerFailureDetected ()) {
    PmcClearPowerFailureStatus ();
  }

  ///----------------------------------------------------------------------------------
  ///
  /// Clear PWR_BTN_STS if set. BIOS should check the WAK_STS bit in PM1_STS[15] (PCH register ABASE+00h) before memory
  /// initialization to determine if ME has reset the system while the Host was in a sleep state.
  /// If WAK_STS is not set, BIOS should ensure a non-sleep exit path is taken by overwriting
  /// PM1_CNT[12:10] (PCH register ABASE+04h) to 111b to force an s5 exit.
  ///
  ABase = PmcGetAcpiBase ();

  Pm1Sts = IoRead16 (ABase + R_ACPI_IO_PM1_STS);
  if ((Pm1Sts & B_ACPI_IO_PM1_STS_PWRBTN) == B_ACPI_IO_PM1_STS_PWRBTN) {
    IoWrite16 (ABase + R_ACPI_IO_PM1_STS, B_ACPI_IO_PM1_STS_PWRBTN);
  }

  if ((Pm1Sts & B_ACPI_IO_PM1_STS_WAK) == 0) {
    Pm1Cnt = IoRead32 (ABase + R_ACPI_IO_PM1_CNT);
    Pm1Cnt |= V_ACPI_IO_PM1_CNT_S5;
    IoWrite32 (ABase + R_ACPI_IO_PM1_CNT, Pm1Cnt);
  }

  //
  // Install MRC {Normal,Recovery} FV info
  //
  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  //
  // Create the platform Flash Map
  //
  Status = PeimInitializeFlashMap (FileHandle, PeiServices);
  ASSERT_EFI_ERROR (Status);

#ifdef SV_HOOKS                             // PPV:InternalOnlyContent
  CreateValidationMegaBlockHob( &SetupData ); // PPV:InternalOnlyContent
#endif                                      // PPV:InternalOnlyContent

  //
  // Update Platform Info
  //
  UpdatePlatformInfo (&SetupData.SystemConfig, &SetupData.SocketConfig);

  if (BootMode == BOOT_WITH_MFG_MODE_SETTINGS) {
   Status = PcdSetBoolS (PcdTpmPhysicalPresence, TRUE);
   ASSERT_EFI_ERROR (Status);
   if (EFI_ERROR(Status)) {
     return Status;
   }
  }

  //
  // Do platform specific on-board Zoar init
  //
  PlatformPchLanConfig(&SetupData.SystemConfig);

  //
  // Initialize platform PPIs
  //
  Status = PeiServicesInstallPpi (&mPpiList[0]);
  ASSERT_EFI_ERROR (Status);

  Status = PeimInstallFlashMapPpi (FileHandle, PeiServices);
  ASSERT_EFI_ERROR (Status);

#ifdef LT_FLAG
    //
    // Add BIOS ACM address hob
    //
    FoundFit = FindFITRecord (PLATFORM_TXT_FIT_BIOSACM_TYPE, &StartupAcmAddressInFit, &StartupAcmAddress);
    //
    // Get BIOS LCP PD Base & Size by searching FAIL_SAFE  Policy in FIT Table
    //
    FindFITRecord (PLATFORM_TXT_FIT_FAILSAFE_TYPE, &FailSafeAddressInFit, &FailSafeAddress);
    if (FailSafeAddress != 0) {
        LcpSize = (UINTN)*(UINT64 *)(FailSafeAddressInFit+8);
        LcpSize = LcpSize & 0x0000FFFFFF;
        LcpSize = LcpSize << 4;
        LcpSize = LcpSize + 0xC;
    }
    // Only build HOB when LTSX strapping ON, in which case ucode would setup LT_EXIST.
    if ( (FoundFit) && ((*(volatile UINT32 *)(UINTN)LT_SPAD_EXIST)) ) {
      PlatformTxtPolicyData.BiosAcmAddress           = ACM_BASE;
      PlatformTxtPolicyData.StartupAcmAddressInFit   = (EFI_PHYSICAL_ADDRESS) StartupAcmAddressInFit; // This is to be used later in EFI shell utility when updating the FI table to enable the ACM module (might not be needed anymore)
      PlatformTxtPolicyData.StartupAcmAddress        = (EFI_PHYSICAL_ADDRESS) StartupAcmAddress;
      PlatformTxtPolicyData.MiscPolicy               = (EFI_PLATFORM_TXT_POLICY_MISC_NO_SCHECK_IN_RESUME | EFI_PLATFORM_TXT_POLICY_MISC_NO_NODMA_TABLE);
      PlatformTxtPolicyData.BiosOsDataRegionRevision = 3;
      PlatformTxtPolicyData.LcpPolicyDataBase        = (EFI_PHYSICAL_ADDRESS) FailSafeAddress;
      PlatformTxtPolicyData.LcpPolicyDataSize        = (UINTN)LcpSize;
      PlatformTxtPolicyData.TxtScratchAddress        = 0;
      PlatformTxtPolicyData.BiosAcmPolicy            = 0;
      PlatformTxtPolicyData.FlagVariable             = 0;

      BuildGuidDataHob (
        &gEfiPlatformTxtPolicyDataGuid,
        &PlatformTxtPolicyData,
        sizeof (PlatformTxtPolicyData)
        );
    } else {
      DEBUG((EFI_D_ERROR, "Fit Table not found, do nothing for LT-SX ...\n"));
    }
    //
    //Add platform policy for LT Config Lock control
    //User can decide whether to perform LT Config Lock by setting PCD for platform specific requirement.
    //Need to force LT config lock for Whitley PC
  DfxEnable = DFX_DISABLE;
//
// IIO:RestrictedBegin
//
  DfxEnable = SetupData.SocketConfig.IioConfig.DFXEnable;
  if ((SetupData.SocketConfig.SocketProcessorCoreConfiguration.LockChipset == 0) || (DfxEnable == 1))
    Status = PcdSetBoolS (PcdLtConfigLockEnable, FALSE);
//
// IIO:RestrictedEnd
//

#endif

  //
  //  Suppress all TPM commands if MePttSupporessCommand or PmcIsDwrBootMode == TRUE by setting
  //  PcdTpmInstanceGuid = gEfiTpmDeviceInstanceNoneGuid and PcdTpmAutoDetection = FALSE
  //
#ifndef SKXD_EN
#if ME_SUPPORT_FLAG

  if (!EFI_ERROR (Status)) {
    if ((SetupData.MeRcConfig.MePttSuppressCommandSend == TRUE) || PmcIsDwrBootMode()) {
      Size = sizeof(gEfiTpmDeviceInstanceNoneGuid);
      Status = PcdSetPtrS(PcdTpmInstanceGuid, &Size, &gEfiTpmDeviceInstanceNoneGuid);
      DEBUG ((DEBUG_INFO, "BIOS will send no further commands to PTT\n"));
      ASSERT_EFI_ERROR(Status);

      Status = PcdSetBoolS(PcdTpmAutoDetection, FALSE);
      DEBUG((DEBUG_INFO, "If PTTSuppressCommandSend == TRUE, Set PcdTpmAutoDetection to FALSE\n"));
      ASSERT_EFI_ERROR(Status);

      Status = PcdSetBoolS(PcdMePttSuppressCommandSend, TRUE);
      DEBUG((DEBUG_INFO, "PTTSuppressCommandSend == TRUE and Set PcdTpmAutoDetection to FALSE\n"));
      ASSERT_EFI_ERROR(Status);
    }
  }
#endif
#endif

  if (PcdGetBool(PcdTestPointFeatureSupport)) {
    //
    // Performing PlatformInitPreMem after PeiReadOnlyVariable2 PPI produced
    //
    Status = PeiServicesNotifyPpi (&mPreMemNotifyList);

    //
    // After code reorangized, memorycallback will run because the PPI is already
    // installed when code run to here, it is supposed that the InstallEfiMemory is
    // done before.
    //
    Status = PeiServicesNotifyPpi (&mMemDiscoveredNotifyList);
  }

  //
  // Initialize platform PPIs
  //
  Status = PeiServicesNotifyPpi (&mNotifyList[0]);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**

  Checks the setup option for breakpoint type and starts
  a deadloop if After MRC was selected

  @param **PeiServices - Pointer to the list of PEI services
  @param *NotifyDescriptor - The notification structure this PEIM registered on install
  @param *Ppi - Pointer to the PPI

  @retval EFI_SUCCESS  -  Function has completed successfully.

**/
EFI_STATUS
EFIAPI
AfterMrcBreakpoint (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor,
  IN  VOID                        *Ppi
  )
{
  EFI_STATUS Status;
  UINT8 ValidationBreakpointTypeOption;

  Status = GetOptionData(&gEfiSetupVariableGuid, OFFSET_OF (SYSTEM_CONFIGURATION, ValidationBreakpointType), &ValidationBreakpointTypeOption, sizeof(ValidationBreakpointTypeOption));
  if (EFI_ERROR(Status)) {
    ValidationBreakpointTypeOption = BP_NONE;
  }

  if (ValidationBreakpointTypeOption == BP_AFTER_MRC) {
    DEBUG((EFI_D_INFO, "== Validation Breakpoint After MRC is done! == \n"));
    EFI_DEADLOOP();
  }

  return Status;
}

/**

  Checks the setup option for breakpoint type and starts
  a deadloop if After KTI IRC was selected

  @param **PeiServices - Pointer to the list of PEI services
  @param *NotifyDescriptor - The notification structure this PEIM registered on install
  @param *Ppi - Pointer to the PPI

  @retval EFI_SUCCESS  -  Function has completed successfully.

**/
EFI_STATUS
EFIAPI
AfterKtiRcBreakpoint (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor,
  IN  VOID                        *Ppi
  )
{
  EFI_STATUS Status;
  UINT8 ValidationBreakpointTypeOption;

  Status = GetOptionData(&gEfiSetupVariableGuid, OFFSET_OF (SYSTEM_CONFIGURATION, ValidationBreakpointType), &ValidationBreakpointTypeOption, sizeof(ValidationBreakpointTypeOption));
  if (EFI_ERROR(Status)) {
    ValidationBreakpointTypeOption = BP_NONE;
  }

  if (ValidationBreakpointTypeOption == BP_AFTER_KTIRC) {
    DEBUG((EFI_D_INFO, "== Validation Breakpoint After KTI RC is done! == \n"));
    EFI_DEADLOOP();
  }

  DEBUG((EFI_D_ERROR, "After KTI RC is done UpdateIioSiPolicy! \n"));

  UpdateIioSiPolicy ();

  PrintIioSiPolicy ();

  return Status;
}


/**

  Checks the setup option for breakpoint type and starts
  a deadloop if Ready for IBIST was selected

  @param **PeiServices - Pointer to the list of PEI services
  @param *NotifyDescriptor - The notification structure this PEIM registered on install
  @param *Ppi - Pointer to the PPI

  @retval EFI_SUCCESS  -  Function has completed successfully.

**/
EFI_STATUS
EFIAPI
ReadyForIbistBreakpoint (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor,
  IN  VOID                        *Ppi
  )
{
  EFI_STATUS Status;
  UINT8 ValidationBreakpointTypeOption;

  Status = GetOptionData(&gEfiSetupVariableGuid, OFFSET_OF (SYSTEM_CONFIGURATION, ValidationBreakpointType), &ValidationBreakpointTypeOption, sizeof(ValidationBreakpointTypeOption));
  if (EFI_ERROR(Status)) {
    ValidationBreakpointTypeOption = BP_NONE;
  }

  if (ValidationBreakpointTypeOption == BP_READY_FOR_IBIST) {
    DEBUG((EFI_D_INFO, "== Validation Breakpoint when Ready for IBIST! == \n"));
    EFI_DEADLOOP();
  }

  return Status;
}

/**

  Checks the setup option for breakpoint type and starts
  a deadloop if the After Full Speed Setup was selected

  @param **PeiServices - Pointer to the list of PEI services
  @param *NotifyDescriptor - The notification structure this PEIM registered on install
  @param *Ppi - Pointer to the PPI

  @retval EFI_SUCCESS  -  Function has completed successfully.

**/
EFI_STATUS
EFIAPI
AfterFullSpeedSetupBreakpoint (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor,
  IN  VOID                        *Ppi
  )
{
  EFI_STATUS Status;
  UINT8 ValidationBreakpointTypeOption;

  Status = GetOptionData(&gEfiSetupVariableGuid, OFFSET_OF (SYSTEM_CONFIGURATION, ValidationBreakpointType), &ValidationBreakpointTypeOption, sizeof(ValidationBreakpointTypeOption));
  if (EFI_ERROR(Status)) {
    ValidationBreakpointTypeOption = BP_NONE;
  }

  if (ValidationBreakpointTypeOption == BP_AFTER_FULL_SPEED_SETUP) {
    DEBUG((EFI_D_INFO, "== Validation Breakpoint before QPI go Fast! == \n"));
    EFI_DEADLOOP();
  }

  return Status;
}

/**

    Read reset type and count warm resets in CMOS location.
    For warm reset counter access from DXE, put counter
    to Pcd also.

    @param PeiServices   - pointer to the PEI Service Table

    @retval None.
**/
VOID
UpdateResetTypeInformation(
    IN EFI_PEI_SERVICES **PeiServices
    )
{

  BOOLEAN    WarmReset;
  UINT8      CmosValue;
#if defined(ME_SUPPORT_FLAG) && ME_SUPPORT_FLAG
  EFI_STATUS Status;
#endif // ME_SUPPORT_FLAG

  DEBUG((EFI_D_INFO, "UpdateResetTypeInformation() "));

  // Read CMOS WarmReset Counter
  IoWrite8(R_IOPORT_CMOS_UPPER_INDEX, CMOS_WARM_RESET_COUNTER_OFFSET);
  CmosValue = IoRead8( R_IOPORT_CMOS_UPPER_DATA );

  WarmReset = PmcIsMemoryPlacedInSelfRefreshState ();

  if (WarmReset) {
    // current boot is warm
    CmosValue++;
    DEBUG((EFI_D_INFO, "to WARM\n"));
#define WARM_RESET_COUNTER_LIMIT 10
    if (CmosValue > WARM_RESET_COUNTER_LIMIT) {
      // limit CmosValue to avoid counter rewind
      // and false cold reset indication
      CmosValue = WARM_RESET_COUNTER_LIMIT;
    }
#undef WARM_RESET_COUNTER_LIMIT
  } else {
    // current boot is cold, reset warm reset counter
    CmosValue = 0;
    DEBUG((EFI_D_INFO, "to COLD\n"));
  }

  IoWrite8(R_IOPORT_CMOS_UPPER_INDEX, CMOS_WARM_RESET_COUNTER_OFFSET);
  IoWrite8(R_IOPORT_CMOS_UPPER_DATA, CmosValue);
#if defined(ME_SUPPORT_FLAG) && ME_SUPPORT_FLAG
  Status = PcdSet32S(PcdMePlatformResetType, (UINT32) CmosValue);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR(Status)) {
    return;
  }
#endif // ME_SUPPORT_FLAG

}

