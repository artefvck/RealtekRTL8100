/* RealtekRTL8100.h -- RTL8100 driver class definition.
 *
 * Copyright (c) 2014 Laura Müller <laura-mueller@uni-duesseldorf.de>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * Driver for Realtek RTL8100x PCIe fast ethernet controllers.
 *
 * This driver is based on Realtek's r8101 Linux driver (1.030.02).
 */

#include "RealtekRTL8100Linux-103002.h"

#ifdef DEBUG
#define DebugLog(args...) IOLog(args)
#else
#define DebugLog(args...)
#endif

#define	RELEASE(x)	if(x){(x)->release();(x)=NULL;}

#define WriteReg8(reg, val8)    _OSWriteInt8((baseAddr), (reg), (val8))
#define WriteReg16(reg, val16)  OSWriteLittleInt16((baseAddr), (reg), (val16))
#define WriteReg32(reg, val32)  OSWriteLittleInt32((baseAddr), (reg), (val32))
#define ReadReg8(reg)           _OSReadInt8((baseAddr), (reg))
#define ReadReg16(reg)          OSReadLittleInt16((baseAddr), (reg))
#define ReadReg32(reg)          OSReadLittleInt32((baseAddr), (reg))

#define super IOEthernetController

enum
{
    MEDIUM_INDEX_AUTO = 0,
    MEDIUM_INDEX_10HD,
    MEDIUM_INDEX_10FD,
    MEDIUM_INDEX_100HD,
    MEDIUM_INDEX_100FD,
    MEDIUM_INDEX_100FDFC,
    MEDIUM_INDEX_100FDEEE,
    MEDIUM_INDEX_100FDFCEEE,
    MEDIUM_INDEX_COUNT
};

#define MBit 1000000

enum {
    kSpeed100MBit = 100*MBit,
    kSpeed10MBit = 10*MBit,
};

enum {
    kFlowControlOff = 0,
    kFlowControlOn = 0x01
};

enum {
    kEEEMode100 = 0x0002,
};

enum {
    kEEETypeNo = 0,
    kEEETypeYes = 1,
    kEEETypeCount
};

/* RTL8100's dma descriptor. */
typedef struct RtlDmaDesc {
    UInt32 opts1;
    UInt32 opts2;
    UInt64 addr;
} RtlDmaDesc;

/* RTL8100's statistics dump data structure */
typedef struct RtlStatData {
	UInt64	txPackets;
	UInt64	rxPackets;
	UInt64	txErrors;
	UInt32	rxErrors;
	UInt16	rxMissed;
	UInt16	alignErrors;
	UInt32	txOneCollision;
	UInt32	txMultiCollision;
	UInt64	rxUnicast;
	UInt64	rxBroadcast;
	UInt32	rxMulticast;
	UInt16	txAborted;
	UInt16	txUnderun;
} RtlStatData;

#define kTransmitQueueCapacity  1024

/* With up to 40 segments we should be on the save side. */
#define kMaxSegs 40

/* The number of descriptors must be a power of 2. */
#define kNumTxDesc	1024	/* Number of Tx descriptors */
#define kNumRxDesc	512     /* Number of Rx descriptors */
#define kTxLastDesc    (kNumTxDesc - 1)
#define kRxLastDesc    (kNumRxDesc - 1)
#define kTxDescMask    (kNumTxDesc - 1)
#define kRxDescMask    (kNumRxDesc - 1)
#define kTxDescSize    (kNumTxDesc*sizeof(struct RtlDmaDesc))
#define kRxDescSize    (kNumRxDesc*sizeof(struct RtlDmaDesc))

/* This is the receive buffer size (must be large enough to hold a packet). */
#define kRxBufferPktSize    2000
#define kRxNumSpareMbufs    100
#define kMCFilterLimit  32

/* statitics timer period in ms. */
#define kTimeoutMS 1000

/* Treshhold value in ns for the modified interrupt sequence. */
#define kFastIntrTreshhold 200000

/* Treshhold value to wake a stalled queue */
#define kTxQueueWakeTreshhold (kNumTxDesc / 3)

/* transmitter deadlock treshhold in seconds. */
#define kTxDeadlockTreshhold 3
#define kTxCheckTreshhold (kTxDeadlockTreshhold - 1)

/* IPv4 specific stuff */
#define kMinL4HdrOffsetV4 34

/* IPv6 specific stuff */
#define kMinL4HdrOffsetV6 54

/* This definitions should have been in IOPCIDevice.h. */
enum
{
    kIOPCIPMCapability = 2,
};

enum
{
    kIOPCIELinkCapability = 12,
    kIOPCIELinkControl = 16,
};

enum
{
    kIOPCIELinkCtlASPM = 0x0003,    /* ASPM Control */
    kIOPCIELinkCtlL0s = 0x0001,     /* L0s Enable */
    kIOPCIELinkCtlL1 = 0x0002,      /* L1 Enable */
    kIOPCIELinkCtlCcc = 0x0040,     /* Common Clock Configuration */
    kIOPCIELinkCtlClkReqEn = 0x100, /* Enable clkreq */
};

enum
{
    kPowerStateOff = 0,
    kPowerStateOn,
    kPowerStateCount
};

#define kEnableEeeName "enableEEE"
#define kEnableCSO6Name "enableCSO6"
#define kEnableTSO4Name "enableTSO4"
#define kEnableTSO6Name "enableTSO6"
#define kIntrMitigateName "intrMitigate"
#define kDisableASPMName "disableASPM"
#define kDriverVersionName "Driver_Version"
#define kNameLenght 64

#define kEnableRxPollName "rxPolling"

extern const struct RTLChipInfo rtl_chip_info[];

class RTL8100 : public super
{
	
	OSDeclareDefaultStructors(RTL8100)
	
public:
	/* IOService (or its superclass) methods. */
	virtual bool start(IOService *provider) override;
	virtual void stop(IOService *provider) override;
	virtual bool init(OSDictionary *properties) override;
	virtual void free() override;
	
	/* Power Management Support */
	virtual IOReturn registerWithPolicyMaker(IOService *policyMaker) override;
    virtual IOReturn setPowerState(unsigned long powerStateOrdinal, IOService *policyMaker ) override;
	virtual void systemWillShutdown(IOOptionBits specifier) override;
    
	/* IONetworkController methods. */
	virtual IOReturn enable(IONetworkInterface *netif) override;
	virtual IOReturn disable(IONetworkInterface *netif) override;
	
    virtual IOReturn outputStart(IONetworkInterface *interface, IOOptionBits options ) override;
    virtual IOReturn setInputPacketPollingEnable(IONetworkInterface *interface, bool enabled) override;
    virtual void pollInputPackets(IONetworkInterface *interface, uint32_t maxCount, IOMbufQueue *pollQueue, void *context) override;
	
	virtual void getPacketBufferConstraints(IOPacketBufferConstraints *constraints) const override;
	
	virtual IOOutputQueue* createOutputQueue() override;
	
	virtual const OSString* newVendorString() const override;
	virtual const OSString* newModelString() const override;
	
	virtual IOReturn selectMedium(const IONetworkMedium *medium) override;
	virtual bool configureInterface(IONetworkInterface *interface) override;
	
	virtual bool createWorkLoop() override;
	virtual IOWorkLoop* getWorkLoop() const override;
	
	/* Methods inherited from IOEthernetController. */
	virtual IOReturn getHardwareAddress(IOEthernetAddress *addr) override;
	virtual IOReturn setHardwareAddress(const IOEthernetAddress *addr) override;
	virtual IOReturn setPromiscuousMode(bool active) override;
	virtual IOReturn setMulticastMode(bool active) override;
	virtual IOReturn setMulticastList(IOEthernetAddress *addrs, UInt32 count) override;
	virtual IOReturn getChecksumSupport(UInt32 *checksumMask, UInt32 checksumFamily, bool isOutput) override;
	virtual IOReturn setMaxPacketSize(UInt32 maxSize) override;
	virtual IOReturn getMaxPacketSize(UInt32 *maxSize) const override;
    virtual IOReturn setWakeOnMagicPacket(bool active) override;
    virtual IOReturn getPacketFilters(const OSSymbol *group, UInt32 *filters) const override;
    
    virtual UInt32 getFeatures() const override;
    
private:
    bool initPCIConfigSpace(IOPCIDevice *provider);
    static IOReturn setPowerStateWakeAction(OSObject *owner, void *arg1, void *arg2, void *arg3, void *arg4);
    static IOReturn setPowerStateSleepAction(OSObject *owner, void *arg1, void *arg2, void *arg3, void *arg4);
    void getParams();
    bool setupMediumDict();
    bool initEventSources(IOService *provider);
    void interruptOccurred(OSObject *client, IOInterruptEventSource *src, int count);
    void pciErrorInterrupt();
    void txInterrupt();
    void interruptOccurredPoll(OSObject *client, IOInterruptEventSource *src, int count);
    UInt32 rxInterrupt(IONetworkInterface *interface, uint32_t maxCount, IOMbufQueue *pollQueue, void *context);
    bool setupDMADescriptors();
    void freeDMADescriptors();
    void txClearDescriptors();

    void updateStatitics();
    void setLinkUp(UInt8 linkState);
    void setLinkDown();
    bool checkForDeadlock();
    
    /* Hardware initialization methods. */
    bool initRTL8100();
    void enableRTL8100();
    void disableRTL8100();
    void startRTL8100(UInt16 newIntrMitigate, bool enableInterrupts);
    void setOffset79(UInt8 setting);
    void extracted(UInt32 addr, UInt16 &regAlignAddr);
        
    UInt8 csiFun0ReadByte(UInt32 addr);
    void csiFun0WriteByte(UInt32 addr, UInt8 value);
    void disablePCIOffset99();
    void enablePCIOffset99();
    void initPCIOffset99(struct rtl8101_private *tp);
    void setPCI99_180ExitDriverPara();
    void hardwareD3Para();
    void enableEEESupport();
    void disableEEESupport();
    void restartRTL8100();
    void setPhyMedium();

    void powerdownPLL();

    /* Hardware specific methods */
    inline void getChecksumCommand(UInt32 *cmd1, UInt32 *cmd2, mbuf_csum_request_flags_t checksums);
    inline void getTso4Command(UInt32 *cmd1, UInt32 *cmd2, UInt32 mssValue, mbuf_tso_request_flags_t tsoFlags);
    inline void getTso6Command(UInt32 *cmd1, UInt32 *cmd2, UInt32 mssValue, mbuf_tso_request_flags_t tsoFlags);
    inline void getChecksumResult(mbuf_t m, UInt32 status1, UInt32 status2);
    
    void timerActionRTL8100(IOTimerEventSource *timer);
    
private:
	IOWorkLoop *workLoop;
    IOCommandGate *commandGate;
	IOPCIDevice *pciDevice;
	OSDictionary *mediumDict;
	IONetworkMedium *mediumTable[MEDIUM_INDEX_COUNT];
	IOBasicOutputQueue *txQueue;
	
	IOInterruptEventSource *interruptSource;
	IOTimerEventSource *timerSource;
	IOEthernetInterface *netif;
	IOMemoryMap *baseMap;
    volatile void *baseAddr;
    
    /* transmitter data */
    mbuf_t txNext2FreeMbuf;
    IOBufferMemoryDescriptor *txBufDesc;
    IOPhysicalAddress64 txPhyAddr;
    struct RtlDmaDesc *txDescArray;
    IOMbufNaturalMemoryCursor *txMbufCursor;
    UInt64 txDescDoneCount;
    UInt64 txDescDoneLast;
    UInt32 txNextDescIndex;
    UInt32 txDirtyDescIndex;
    SInt32 txNumFreeDesc;
    
    /* receiver data */
    IOBufferMemoryDescriptor *rxBufDesc;
    IOPhysicalAddress64 rxPhyAddr;
    struct RtlDmaDesc *rxDescArray;
	IOMbufNaturalMemoryCursor *rxMbufCursor;
    UInt64 multicastFilter;
    UInt32 rxNextDescIndex;
    UInt32 rxConfigMask;
    
    /* power management data */
    unsigned long powerState;
    
    /* statistics data */
    UInt32 deadlockWarn;
    IONetworkStats *netStats;
	IOEthernetStats *etherStats;
    IOBufferMemoryDescriptor *statBufDesc;
    IOPhysicalAddress64 statPhyAddr;
    struct RtlStatData *statData;
    
    UInt32 mtu;
    UInt32 speed;
    UInt32 duplex;
    UInt16 flowCtl;
    UInt16 autoneg;
    UInt16 eeeAdv;
    UInt16 eeeCap;
    struct pci_dev pciDeviceData;
    struct rtl8101_private linuxData;
    struct IOEthernetAddress currMacAddr;
    struct IOEthernetAddress origMacAddr;
    
    UInt16 intrMask;
    UInt16 intrMitigateValue;
    UInt16 intrMaskRxTx;
    UInt16 intrMaskPoll;
    
    IONetworkPacketPollingParameters pollParams;
    
    bool rxPoll;
    bool polling;

    /* flags */
    bool isEnabled;
	bool promiscusMode;
	bool multicastMode;
    bool linkUp;
    bool needsUpdate;
    bool wolCapable;
    bool wolActive;
    bool revision2;
    bool enableTSO4;
    bool enableTSO6;
    bool enableCSO6;
    bool disableASPM;
    bool enableEEE;
    
    /* mbuf_t arrays */
    mbuf_t txMbufArray[kNumTxDesc];
    mbuf_t rxMbufArray[kNumRxDesc];
};
