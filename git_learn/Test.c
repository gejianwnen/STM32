#include "PlatformInitPreMem.h"

#include <Library/PchPcrLib.h>


VOID
Test (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN SYSTEM_CONFIGURATION        *SystemConfiguration,
  IN PCH_SETUP                   *PchSetup
  )
{
	// SystemTable->ConOut->OutputString(SystemTable->ConOut, L"HelloWorld --gejianwen\n");
	DEBUG((EFI_D_ERROR, "Test --gejianwen \n"));

}