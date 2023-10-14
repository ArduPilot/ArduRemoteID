
struct CANFrame;

class CANDriver {
public:
    CANDriver();
    void init(uint32_t bitrate, uint32_t acceptance_code, uint32_t acceptance_mask);

    bool send(const CANFrame &frame);
    bool receive(CANFrame &out_frame);

private:
    struct Timings {
        uint16_t prescaler;
        uint8_t sjw;
        uint8_t bs1;
        uint8_t bs2;

        Timings()
            : prescaler(0)
            , sjw(0)
            , bs1(0)
            , bs2(0)
        { }
    };

    bool init_bus(const uint32_t bitrate);
    void init_once(bool enable_irq);
    bool computeTimings(uint32_t target_bitrate, Timings& out_timings);

    uint32_t bitrate;
    uint32_t last_bus_recovery_ms;
};

/**
 * Raw CAN frame, as passed to/from the CAN driver.
 */
struct CANFrame {
    static const uint32_t MaskStdID = 0x000007FFU;
    static const uint32_t MaskExtID = 0x1FFFFFFFU;
    static const uint32_t FlagEFF = 1U << 31;                  ///< Extended frame format
    static const uint32_t FlagRTR = 1U << 30;                  ///< Remote transmission request
    static const uint32_t FlagERR = 1U << 29;                  ///< Error frame

    static const uint8_t NonFDCANMaxDataLen = 8;
    static const uint8_t MaxDataLen = 8;
    uint32_t id;                ///< CAN ID with flags (above)
    union {
        uint8_t data[MaxDataLen];
        uint32_t data_32[MaxDataLen/4];
    };
    uint8_t dlc;                ///< Data Length Code

    CANFrame() :
        id(0),
        dlc(0)
    {
        memset(data,0, MaxDataLen);
    }

    CANFrame(uint32_t can_id, const uint8_t* can_data, uint8_t data_len, bool canfd_frame = false);

    bool operator!=(const CANFrame& rhs) const
    {
        return !operator==(rhs);
    }
    bool operator==(const CANFrame& rhs) const
    {
        return (id == rhs.id) && (dlc == rhs.dlc) && (memcmp(data, rhs.data, dlc) == 0);
    }

    bool isExtended()                  const
    {
        return id & FlagEFF;
    }
    bool isRemoteTransmissionRequest() const
    {
        return id & FlagRTR;
    }
    bool isErrorFrame()                const
    {
        return id & FlagERR;
    }

    static uint8_t dlcToDataLength(uint8_t dlc);
    static uint8_t dataLengthToDlc(uint8_t data_length);
    /**
     * CAN frame arbitration rules, particularly STD vs EXT:
     *     Marco Di Natale - "Understanding and using the Controller Area Network"
     *     http://www6.in.tum.de/pub/Main/TeachingWs2013MSE/CANbus.pdf
     */
    bool priorityHigherThan(const CANFrame& rhs) const;
    bool priorityLowerThan(const CANFrame& rhs) const
    {
        return rhs.priorityHigherThan(*this);
    }
};
