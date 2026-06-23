/*********************************************************************
 File:              rio.h
 Description:   Radio Device Driver
 *********************************************************************
	      Copyright (c) 1997 Microsoft Corporation
 *********************************************************************/
#if !defined(RIO_H)
#define RIO_H

// Include <wis.h> first

#define RADIO_PROGRAM                       1
#define RADIO_REGISTER                      2
#define RADIO_GET_DRIVER_INFO               3
#define RADIO_GET_MANUFACTURER_INFO         4
#define RADIO_GET_HW_INFO                   5
#define RADIO_GET_CARRIER_INFO              6
#define RADIO_GET_ADDRESS_INFO              7
#define RADIO_GET_GROUP_INFO                8
#define RADIO_GET_KEY_INFO                  9
#define RADIO_GET_STATUS                    10
#define RADIO_GET_STATISTICS                11
#define RADIO_SET_STATUS                    12
#define RADIO_SET_STATISTICS                13
#define RADIO_SET_EXTRA_INFO_STRIP_RULES    14
#define RADIO_CRYPT_DERIVE_KEY              15
#define RADIO_READ_MSG_INFO                 16
#define RADIO_READ_MSG_DATA                 17
#define RADIO_WRITE_MSG_INFO                18
#define RADIO_WRITE_MSG_DATA                19
#define RADIO_LOCK                          20
#define RADIO_TEST_MSG                      100
#define RADIO_TEST_MSG2                     101

#define ADDRESS_FLAG_ENABLE                 1
#define ADDRESS_FLAG_PRIORITY               2
#define ADDRESS_FLAG_AC_ONLY                4
#define ADDRESS_FLAG_PO_ONLY                8

#define GROUP_FLAG_ENABLE                   1
#define GROUP_FLAG_PRIORITY                 2
#define GROUP_FLAG_AC_ONLY                  4
#define GROUP_FLAG_PO_ONLY                  8

#define RADIO_PGM_OPERATION_PROGRAM         1
#define RADIO_PGM_OPERATION_UNPROGRAM       2

#define RADIO_PGM_TYPE_CARRIER              1
#define RADIO_PGM_TYPE_KEY                  2
#define RADIO_PGM_TYPE_ADDRESS              3
#define RADIO_PGM_TYPE_GROUP                4

#define DATA_MESSAGE                        1
#define DATA_RESPONSE                       2
#define DATA_ERRORS                         3
#define DATA_OEM                            4

typedef struct _radio_reg {
    WORD wStructSize;
    DWORD dwMemberValidMask;
    DWORD dwStateChange;
    DWORD dwCauseWakeup;
    BYTE EventNameLen;
//  BYTE EventName[EventNameLen];
} RADIO_REG, *LPRADIO_REG;
#define RADIO_REG_MVM_STATE_CHANGE          0x0001  // dwStateChange field is valid

// dwStateChange values:
#define RADIO_REG_RADIO_STATE_CHANGE        0x0001
#define RADIO_REG_POWER_CONTROL_CHANGE      0x0002
#define RADIO_REG_SYSTEM_WAKEUP_FROM_RADIO  0x0004
#define RADIO_REG_NEW_MESSAGE               0x0008
#define RADIO_REG_LOW_BATTERY               0x0010
#define RADIO_REG_RECHARGING_STATE_CHANGE   0x0020
#define RADIO_REG_OUT_OF_RANGE              0x0040
#define RADIO_REG_ROAMING_CHANGE            0x0080
#define RADIO_REG_POWER_LEVEL_CHANGE        0x0100
#define RADIO_REG_SIGNAL_STRENGTH_CHANGE    0x0200
#define RADIO_REG_PROGRAMMING_OPERATION     0x0400
#define RADIO_REG_TOD_UPDATE                0x0800
#define RADIO_REG_OVERFLOW                  0x1000
#define RADIO_REG_HW_STATE_CHANGE           0x2000
#define RADIO_REG_ALL                       0xFFFF


typedef struct _radio_driver_info {
    WORD wStructSize;
    DWORD dwMemberValidMask;
    BYTE MajorVersionDriver;
    BYTE MinorVersionDriver;
    BYTE MajorVersionAPI;
    BYTE MinorVersionAPI;
    FILETIME DateTimeDriverRelease;
    WORD wDriverDescriptionLen;
//  BYTE DriverDescription[wDriverDescriptionLen];
} RADIO_DRIVER_INFO, *LPRADIO_DRIVER_INFO;

typedef struct _radio_manufacturer_info {
    WORD wStructSize;
    DWORD dwMemberValidMask;
    BYTE ManufacturerNameLen;
    WORD wManufacturerDescriptionLen;
//  BYTE ManufacturerName[ManufacturerNameLen];
//  BYTE ManufacturerDescription[wManufacturerDescriptionLen];
} RADIO_MANUFACTURER_INFO, *LPRADIO_MANUFACTURER_INFO;

typedef struct _radio_hw_info {
    WORD wStructSize;
    DWORD dwMemberValidMask;
    BYTE MajorVersionHW;
    BYTE MinorVersionHW;
    FILETIME DateTimeHWRelease;
    BYTE HWFormFactor;
    BYTE CommunicationType;
    WORD wCapability;
    BYTE Protocol;
    BYTE PowerLevel;
    BYTE TxSignalLevel;
    BYTE RxSignalLevel;
    DWORD dwBufferSize;
    BYTE NumAddresses;
    BYTE NumGroups;
    BYTE NumKeys;
    BYTE SerialNumberLen;
    WORD wHWDescriptionLen;
    BYTE DeviceNameLen;
//  BYTE SerialNumber[SerialNumberLen];
//  BYTE HWDescription[wHWDescriptionLen];
//  BYTE DeviceName[DeviceNameLen];
} RADIO_HW_INFO, *LPRADIO_HW_INFO;
#define RADIO_HWINFO_MVM_MAJOR_VERSION_HW       0x0001  // field MajorVersionHW is valid
#define RADIO_HWINFO_MVM_MINOR_VERSION_HW       0x0002  // field MinorVersionHW is valid
#define RADIO_HWINFO_MVM_DATE_TIME_HW_RELEASE   0x0004  // field DateTimeHWRelease is valid
#define RADIO_HWINFO_MVM_HW_FORM_FACTOR         0x0008  // field HWFormFactor is valid
#define RADIO_HWINFO_MVM_COMMUNICATION_TYPE     0x0010  // field CommunicationType is valid
#define RADIO_HWINFO_MVM_CAPABILITY             0x0020  // field wCapability is valid
#define RADIO_HWINFO_MVM_PROTOCOL               0x0040  // field Protocol is valid
#define RADIO_HWINFO_MVM_POWER_LEVEL            0x0080  // field PowerLevel is valid
#define RADIO_HWINFO_MVM_TX_SIGNAL_LEVEL        0x0100  // field TxSignalLevel is valid
#define RADIO_HWINFO_MVM_RX_SIGNAL_LEVEL        0x0200  // field RxSignalLevel is valid
#define RADIO_HWINFO_MVM_BUFFER_SIZE            0x0400  // field dwBufferSize is valid
#define RADIO_HWINFO_MVM_NUM_ADDRESSES          0x0800  // field NumAddresses is valid
#define RADIO_HWINFO_MVM_NUM_GROUPS             0x1000  // field NumGroups is valid
#define RADIO_HWINFO_MVM_NUM_KEYS               0x2000  // field NumKeys is valid
                                                           
                                                           
typedef struct _radio_carrier_info {                       
    WORD wStructSize;                                      
    DWORD dwMemberValidMask;
    DWORD dwRxFrequency;
    DWORD dwTxFrequency;
    BYTE UserIDLen;
    BYTE CarrierNameLen;
    WORD wCarrierDescriptionLen;
//  BYTE UserId[UserIDLen];
//  BYTE CarrierName[CarrierNameLen];
//  BYTE CarrierDescription[wCarrierDescriptionLen];
} RADIO_CARRIER_INFO, *LPRADIO_CARRIER_INFO;
#define RADIO_CARINFO_MVM_RX_FREQ                0x0001  // field dwRxFrequency is valid
#define RADIO_CARINFO_MVM_TX_FREQ                0x0002  // field dwTxFrequency is valid

typedef struct _radio_address {
    WORD wStructSize;
    DWORD dwMemberValidMask;
    BYTE AddressNumber;
    BYTE Status;
    BYTE ExpirationDate[2];
    BYTE KeyTagLen;
    BYTE AddressTagLen;
    BYTE AddressNameLen;
    WORD wAddressDescriptionLen;
    WORD wAddressInfoLen;
//  BYTE KeyTag[KeyTagLen];
//  BYTE AddressTag[AddressTagLen];
//  BYTE AddressName[AddressNameLen];
//  BYTE AddressDescription[wAddressDescriptionLen];
//  BYTE AddressInfo[wAddressInfoLen];
} RADIO_ADDRESS, *LPRADIO_ADDRESS;
#define RADIO_ADDRESS_MVM_NUMBER            0x0001  // AddressNumber field is valid
#define RADIO_ADDRESS_MVM_STATUS            0x0002  // Status field is valid
#define RADIO_ADDRESS_MVM_EXPIRATION_DATE   0x0004  // ExpirationDate field is valid
#define RADIO_ADDRESS_MVM_KEY_TAG_LEN       0x0008  // KeyTagLen field is valid
#define RADIO_ADDRESS_MVM_TAG_LEN           0x0010  // AddressTagLen field is valid
#define RADIO_ADDRESS_MVM_NAME_LEN          0x0020  // AddressNameLen field is valid
#define RADIO_ADDRESS_MVM_DESCRIPTION_LEN   0x0040  // AddressDescriptionLen field is valid
#define RADIO_ADDRESS_MVM_INFO_LEN          0x0080  // AddressInfoLen field is valid

typedef struct _radio_group {
    WORD wStructSize;
    DWORD dwMemberValidMask;
    WORD wGroupNumber;
    BYTE Status;
    BYTE GroupCode;
    BYTE ExpirationDate[2];
    BYTE KeyTagLen;
    BYTE AddressTagLen;
    BYTE GroupTagLen;
    BYTE GroupNameLen;
    WORD wGroupDescriptionLen;
//  BYTE KeyTag[KeyTagLen];
//  BYTE AddressTag[AddressTagLen];
//  BYTE GroupTag[GroupTagLen];
//  BYTE GroupName[GroupNameLen];
//  BYTE GroupDescription[wGroupDescriptionLen];
} RADIO_GROUP, *LPRADIO_GROUP;
#define RADIO_GROUP_MVM_NUMBER              0x0001  // GroupNumber field is valid
#define RADIO_GROUP_MVM_STATUS              0x0002  // Status field is valid
#define RADIO_GROUP_MVM_CODE                0x0004  // GroupCode field is valid
#define RADIO_GROUP_MVM_EXPIRATION_DATE     0x0008  // ExpirationDate field is valid
#define RADIO_GROUP_MVM_KEY_TAG_LEN         0x0010  // KeyTag field is valid
#define RADIO_GROUP_MVM_ADDRESS_TAG_LEN     0x0020  // AddressTag field is valid
#define RADIO_GROUP_MVM_TAG_LEN             0x0040  // GroupTag field is valid
#define RADIO_GROUP_MVM_NAME_LEN            0x0080  // GroupName field is valid
#define RADIO_GROUP_MVM_DESCRIPTION_LEN     0x0100  // GroupDescription field is valid

typedef struct _radio_key {
    WORD wStructSize;
    DWORD dwMemberValidMask;
    BYTE KeyNumber;
    DWORD dwAlgCode;
    BYTE KeyTagLen;
    BYTE KeyLen;
//  BYTE KeyTag[KeyTagLen];
//  BYTE Key[KenLen];
} RADIO_KEY, *LPRADIO_KEY;
#define RADIO_KEY_MVM_NUMBER                0x0001  // KeyNumber field is valid
#define RADIO_KEY_MVM_ALG_CODE              0x0002  // AlgCode field is valid
#define RADIO_KEY_MVM_TAG_LEN               0x0004  // KeyTag field is valid
#define RADIO_KEY_MVM_LEN                   0x0008  // KeyLen field is valid

typedef struct _radio_status {
    WORD wStructSize;
    DWORD dwMemberValidMask;
    DWORD dwStatus;
} RADIO_STATUS, *LPRADIO_STATUS;
#define RADIO_STATUS_MVM_STATUS             0x0001  // dwStatus field is valid
// dwStatus field codes
#define RADIO_STATUS_ENABLED                    0x0001
#define RADIO_STATUS_POWER_DOWN_ENABLED         0x0002
#define RADIO_STATUS_SYSTEM_WAKEUP_FROM_RADIO   0x0004
#define RADIO_STATUS_MSGS_AVAILABLE             0x0008
#define RADIO_STATUS_LOW_BATTERY                0x0010
#define RADIO_STATUS_RECHARGING                 0x0020
#define RADIO_STATUS_OUT_OF_RANGE               0x0040
#define RADIO_STATUS_ROAMING                    0x0100
#define RADIO_STATUS_OVERFLOW                   0x0400
#define RADIO_STATUS_HW_PRESENT                 0x0800

typedef struct _radio_statistics {
    WORD wStructSize;
    DWORD dwMemberValidMask;
    DWORD dwNumPacketsMonitored;
    DWORD dwNumPacketsReceived;
    DWORD dwNumErrorPackets;
    DWORD dwNumTotalBytesReceived;
    DWORD dwNumErrorBytes;
    DWORD dwNumPacketsSent;
    DWORD dwNumPacketsRetried;
    DWORD dwNumBytesSent;
    DWORD dwNumBytesRetried;
    WORD wOEMStats;
//  VOID OEMStats;
} RADIO_STATISTICS, *LPRADIO_STATISTICS;

enum RADIO_MSG_PART_TYPE {
    RADIO_MSG_PART_MSG_DATA             = 0x1,
    RADIO_MSG_PART_RESPONSE_DATA        = 0x2,
    RADIO_MSG_PART_ERROR_VECTORS        = 0x3,
    RADIO_MSG_PART_OEM_DATA             = 0x4
};

typedef struct _radio_msg_part_info {
    BYTE PartType;
    DWORD dwOffset;
    DWORD dwSize;
} RADIO_MSG_PART_INFO, *LPRADIO_MSG_PART_INFO;

typedef struct _radio_read_info {
    DWORD dwOffset;
    DWORD dwNumBytes;
} RADIO_READ_INFO, *LPRADIO_READ_INFO;

typedef struct _radio_error_vector {
    DWORD dwOffset;             // offset in the message where error begins
    DWORD dwNumBytes;           // number of bytes starting at the above offset for which errors continue
} RADIO_ERROR_VECTOR, *LPRADIO_ERROR_VECTOR;

enum RADIO_MSG_TYPE  {
    RADIO_MSG_TYPE_NO_TYPE              = 0x00,
    RADIO_MSG_TYPE_NUMERIC_MSG          = 0x01, // This is a numeric message
    RADIO_MSG_TYPE_ALPHA_MSG            = 0x02, // This is an alpha message
    RADIO_MSG_TYPE_BINARY_MSG           = 0x03, // This is a binary message
    RADIO_MSG_TYPE_TOD_UPDATE_MSG       = 0x04, // This is a TOD update message
    RADIO_MSG_TYPE_OTAP_MSG             = 0x05, // This is an OTAP (Over the air programming) message
    RADIO_MSG_TYPE_CANNED_RESPONSE_MSG  = 0x06, // This is a canned response message
    RADIO_MSG_TYPE_NORMAL_RESPONSE_MSG  = 0x07, // This is a normal response message
    RADIO_MSG_TYPE_OEM_MSG              = 0x80  // This is an OEM specific message [0x80-0xEF]
};

enum RADIO_MSG_PRIORITY  {
    RADIO_MSG_PRIORITY_NO_PRIORITY      = 0x00,
    RADIO_MSG_PRIORITY_HIGH_PRIORITY    = 0x01,
    RADIO_MSG_PRIORITY_MEDIUM_PRIORITY  = 0x02,
    RADIO_MSG_PRIORITY_LOW_PRIORITY     = 0x03
};

enum RADIO_MSG_FLAG  {
    RADIO_MSG_FLAG_NO_VALUE             = 0x00,
    RADIO_MSG_FLAG_MAIL_DROP_MSG        = 0x01,    // This is a mail drop msg
    RADIO_MSG_FLAG_MSG_FRAGMENT         = 0x02     // incomplemet message
};

enum RADIO_MSG_ERRORFLAG  {
    RADIO_MSG_ERRORFLAG_NO_VALUE        = 0x00,
    RADIO_MSG_ERRORFLAG_ERRORS          = 0x01,     // message contains errors
    RADIO_MSG_ERRORFLAG_OUT_OF_ORDER    = 0x02      // message is out of order
};

typedef struct _radio_msg_info {
    WORD wStructSize;
    DWORD dwMemberValidMask;
    BYTE MsgType;
    BYTE MsgPriority;
    BYTE MsgFlags;
    BYTE NumParts;
    WORD wErrorFlags;
    WORD wMsgSequenceNumber;
    FILETIME ReceivedDateTime;
    WORD wMsgIdLen;
    WORD wResponseIdLen;
    BYTE AddressTagLen;
    BYTE GroupTagLen;
//  BYTE MsgId[MsgIdLen];
//  BYTE ResponseId[ResponseIdLen];
//  BYTE AddressTag[AddressTagLen];
//  BYTE GroupTag[GroupTagLen];
//  RADIO_MSG_PART_INFO PartInfo;
} RADIO_MSG_INFO, *LPRADIO_MSG_INFO;

#define RADIO_MSGINFO_MVM_MSGTYPE           0x0001  // MsgType is valid
#define RADIO_MSGINFO_MVM_NUMPARTS          0x0008  // NumParts is valid
#define RADIO_MSGINFO_MVM_ERRORFLAGS        0x0010  // Error Flags is valid

//********************************************************************//

#define HRADIO_TYPE_END                     (-1)
#define HRADIO_CODE                         1
#define HRADIO_DATA                         2

#define HRADIO_USER_DEFINED                 0
#define HRADIO_WAKEUP                       1
#define HRADIO_ANALYZE_MSG                  2
#define HRADIO_FILTER_MSG                   3

#define CALLPADDR(CtlBlk, x)
#define CALLVADDR(CtlBlk, x)

typedef struct _hradio_block {
    DWORD dwId;
    DWORD PAddr;
    DWORD VAddr;
    DWORD dwSize;
} HRADIO_BLOCK, *LPHRADIO_BLOCK;

#define HRADIO_MAKE_SYSTEM_ID(Par1, Par2, wPar3)    \
    MAKELONG(MAKEWORD(Par1, Par2), wPar3)

#define HRADIO_MAKE_DRIVER_ID(Par1, wPar2) \
    MAKELONG(MAKEWORD(Par1, HRADIO_USER_DEFINED), wPar2)

//********************************************************************//

// fix
#define RADIO_SUSPEND_ON                    0
#define RADIO_HAS_POWER                     0
#define RADIO_SIGNAL_INFO                   0
// end fix

#endif  // RIO_H
