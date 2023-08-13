#pragma once

#include <efi/efi.h>

// all of definitions taken from
// https://trustedcomputinggroup.org/resource/tcg-efi-protocol-specification/

#define EFI_TCG_PROTOCOL_GUID \
{ 0xf541796d, 0xa62e, 0x4954, 0xa7, 0x75, 0x95, 0x84, 0xf6, 0x1b, 0x9c, 0xdd }

typedef struct {
	UINT8 Major;
	UINT8 Minor;
	UINT8 RevMajor;
	UINT8 RevMinor;
} EFI_TCG_VERSION;

typedef struct {
	UINT8			Size;
	EFI_TCG_VERSION	StructureVersion;
	EFI_TCG_VERSION	ProtocolSpecVersion;
	UINT8			HashAlgorithmBitmap;
	BOOLEAN			TPMPresentFlag;
	BOOLEAN			TPMDeactivatedFlag;
} EFI_TCG_BOOT_SERVICE_CAPABILITY;

typedef
EFI_STATUS
(EFIAPI *EFI_TCG_STATUS_CHECK) (
	IN struct _EFI_TCG_PROTOCOL			*This,
	OUT EFI_TCG_BOOT_SERVICE_CAPABILITY	*ProtocolCapability,
	OUT UINT32							*TCGFeatureFlags,
	OUT EFI_PHYSICAL_ADDRESS			*EventLogLocation,
	OUT EFI_PHYSICAL_ADDRESS			*EventLogLastEntry
	);

typedef UINT32 EFI_TCG_ALGORITHM_ID;

#define EFI_TCG_ALG_SHA 0x00000004

typedef
EFI_STATUS
(EFIAPI *EFI_TCG_HASH_ALL) (
	IN struct _EFI_TCG_PROTOCOL	*This,
	IN UINT8					*HashData,
	IN UINT64					HashDataLen,
	IN EFI_TCG_ALGORITHM_ID		AlgorithmId,
	IN OUT UINT64				*HashedDataLen,
	IN OUT UINT8				**HashedDataResult
	);

typedef UINT32 EFI_TCG_PCRINDEX;
typedef UINT32 EFI_TCG_EVENTTYPE;

#define EFI_TCG_SHA1_DIGEST_SIZE 20

typedef struct {
	UINT8 Digest[EFI_TCG_SHA1_DIGEST_SIZE];
} EFI_TCG_DIGEST;

typedef struct {
	EFI_TCG_PCRINDEX	PCRIndex;
	EFI_TCG_EVENTTYPE	EventType;
	EFI_TCG_DIGEST		Digest;
	UINT32				EventSize;
	UINT8				Event[1];
} EFI_TCG_PCR_EVENT;

typedef
EFI_STATUS
(EFIAPI *EFI_TCG_LOG_EVENT) (
	IN struct _EFI_TCG_PROTOCOL	*This,
	IN EFI_TCG_PCR_EVENT		*TCGLogData,
	IN OUT UINT32				*EventNumber,
	IN UINT32					Flags
	);

typedef
EFI_STATUS
(EFIAPI *EFI_TCG_PASS_THROUGH_TO_TPM) (
	IN struct _EFI_TCG_PROTOCOL	*This,
	IN UINT32					TpmInputParameterBlockSize,
	IN UINT8					*TpmInputParameterBlock,
	IN UINT32					TpmOutputParameterBlockSize,
	IN UINT8					*TpmOutputParameterBlock
	);

typedef
EFI_STATUS
(EFIAPI *EFI_TCG_HASH_LOG_EXTEND_EVENT) (
	IN struct _EFI_TCG_PROTOCOL	*This,
	IN EFI_PHYSICAL_ADDRESS		HashData,
	IN UINT64					HashDataLen,
	IN EFI_TCG_ALGORITHM_ID		AlgorithmId,
	IN OUT EFI_TCG_PCR_EVENT	*TCGLogData,
	IN OUT UINT32				*EventNumber,
	OUT EFI_PHYSICAL_ADDRESS	*EventLogLastEntry
	);

typedef struct _EFI_TCG_PROTOCOL {
	EFI_TCG_STATUS_CHECK			StatusCheck;
	EFI_TCG_HASH_ALL				HashAll;
	EFI_TCG_LOG_EVENT				LogEvent;
	EFI_TCG_PASS_THROUGH_TO_TPM		PassThroughToTPM;
	EFI_TCG_HASH_LOG_EXTEND_EVENT	HashLogExtendEvent;
} EFI_TCG_PROTOCOL;
