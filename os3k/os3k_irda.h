#ifndef _OS3K_IRDA_H_
#define _OS3K_IRDA_H_

/** @file os3k_irda.h
 * @brief Firmware IrDA discovery, logical channels, and service callbacks.
 *
 * These services share global firmware state and the UART with other users.
 * Blocking calls require functioning firmware interrupts and a responsive link.
 * Callbacks inherit the active execution context; the ROM does not install the
 * registering applet's A5. Supply a context-safe callback when globals are used.
 */
#include <stdint.h>

/** @brief Byte status returned by the IrDA channel interface.
 *
 * Only D0.b is defined for these statuses. Not every firmware status has an
 * assigned name here; preserve unknown nonzero values when reporting errors.
 */
typedef enum _IrdaStatus_e {
    IRDA_OK = 0, /**< Operation completed at the firmware interface. */
    IRDA_PAYLOAD_TOO_LARGE = 17, /**< Payload exceeds the negotiated maximum. */
    IRDA_LINK_BUSY = 19, /**< Low-level link operation is not valid in the current state. */
    IRDA_TRANSMIT_PENDING = 20, /**< Previous frame is pending; new frame was not queued. */
    IRDA_WRONG_STATE = 21, /**< Required link state is not active. */
    IRDA_INVALID_CHANNEL = 22, /**< Channel index is outside 0–4. */
    IRDA_CHANNEL_BUSY = 23, /**< Opening a channel whose state is nonzero. */
    IRDA_CHANNEL_OPEN_FAILED = 24, /**< Open negotiation ended without the open state. */
    IRDA_CHANNEL_NOT_OPEN = 25, /**< Channel or IAS-query prerequisites are absent. */
    IRDA_TIMEOUT = 27, /**< Operation did not complete before its firmware deadline. */
    IRDA_CONNECT_FAILED = 30, /**< Link connection ended outside the connected state. */
    IRDA_DISCONNECT_FAILED = 31, /**< Link disconnection ended outside the idle state. */
    IRDA_TRANSMIT_BUSY = 32, /**< Deferred send or transmitter drain failed. */
    IRDA_NO_CREDIT = 33, /**< Data on channel 3/4 requires an available transport credit. */
    IRDA_INVALID_SERVICE = 34 /**< Required service strings/receive callback absent or name too long. */
} IrdaStatus_e;

/** @brief Firmware logical-channel indices, distinct from remote LSAP selectors. */
typedef enum _IrdaChannel_e {
    IRDA_CHANNEL_IAS_SERVER = 0, /**< Local information-access server. */
    IRDA_CHANNEL_1 = 1, /**< Firmware channel with an empty default receive handler. */
    IRDA_CHANNEL_IAS_CLIENT = 2, /**< Information-access query responses. */
    IRDA_CHANNEL_IRCOMM = 3, /**< IrCOMM data, including transport framing/credits. */
    IRDA_CHANNEL_PRINTER = 4 /**< Printer data; forwarded to the registered receive callback. */
} IrdaChannel_e;

/** @brief One six-byte device-discovery slot.
 *
 * IrdaDiscover() requires an array of six slots. Zero address marks an empty
 * slot. This is the native m68k layout, with no padding between slots.
 */
typedef struct _IrdaDevice_t {
    uint32_t address; /**< Discovered remote device address. */
    uint8_t hint1; /**< First advertised discovery hint byte. */
    uint8_t hint2; /**< Second advertised discovery hint byte. */
} IrdaDevice_t;

/** @brief Receive borrowed printer-channel data; return value is ignored.
 * @param data Payload after the transport credit byte; valid during the callback.
 * @param length Payload byte count; nonzero when the callback is called.
 */
typedef void (*IrdaReceiveCallback_t)(const uint8_t* data, uint8_t length);

/** @brief Notification codes delivered to an IrDA service status callback. */
typedef enum _IrdaNotification_e {
    IRDA_CHANNELS_RESET = 35, /**< Channel table was reset. */
    IRDA_PRINTER_DISCONNECTED = 36 /**< Received a close request for the printer channel. */
} IrdaNotification_e;

/** @brief Receive a firmware service notification; return value is ignored.
 * @param status Notification byte: 35 when the channel table is reset, 36 when
 * the printer channel is disconnected by a received close request.
 */
typedef void (*IrdaStatusCallback_t)(IrdaNotification_e status);

/** @brief Query the negotiated application payload bound.
 * @return Negotiated frame-size word minus six, with 16-bit arithmetic.
 */
uint16_t IrdaGetMaxPayload(void);

/** @brief Register an IAS service and printer-channel callbacks.
 *
 * Copies service names into shared firmware storage and retains callbacks.
 * Use names shorter than 20 bytes: the ROM's second copy incorrectly checks
 * the first string's length and does not bound the second independently.
 * Even a rejected request resets service state and can replace callbacks.
 * @param class_name Required NUL-terminated IAS class name.
 * @param attribute_name Required NUL-terminated IAS attribute name.
 * @param receive Required receive callback.
 * @param status Optional notification callback; NULL disables notifications.
 * @return IRDA_OK or IRDA_INVALID_SERVICE.
 */
IrdaStatus_e IrdaConfigureService(const char* class_name, const char* attribute_name,
                                  IrdaReceiveCallback_t receive, IrdaStatusCallback_t status);

/** @brief Discover remote devices while the link is idle.
 *
 * Repeats discovery once when duplicate addresses are found, then clears later
 * duplicates. This is a blocking operation using shared firmware state.
 * @param devices Required writable array of six discovery slots.
 * @param info Optional 32-byte output copied from the firmware discovery-info block.
 * Its internal text/encoding is not exposed as a string contract.
 * @return Byte status; IRDA_OK on completion, IRDA_WRONG_STATE if not idle.
 */
IrdaStatus_e IrdaDiscover(IrdaDevice_t* devices, uint8_t* info);

/** @brief Connect the IrDA link to a discovered address.
 * @param address Remote device address, not a pointer or channel index.
 * @return Byte status; retries the low-level pending status and waits for link negotiation.
 */
IrdaStatus_e IrdaConnect(uint32_t address);

/** @brief Open a logical channel on the current link.
 * @param channel Local channel 0–4.
 * @param remote_selector Remote LSAP selector stored in that channel's state.
 * @return Byte status; zero after the peer confirms the open request.
 */
IrdaStatus_e IrdaOpenChannel(IrdaChannel_e channel, uint8_t remote_selector);

/** @brief Disconnect the link and wait for its idle state.
 * @return Byte status, including IRDA_DISCONNECT_FAILED for an unexpected final state.
 */
IrdaStatus_e IrdaDisconnect(void);

/** @brief Send a channel-close request and wait for transmitter drain.
 * @param channel Open local channel 0–4.
 * @return Byte status; zero after the close request drains.
 */
IrdaStatus_e IrdaCloseChannel(IrdaChannel_e channel);

/** @brief Send one payload on a logical channel.
 *
 * Prepends remote/local LSAP selectors. Data on channels 3/4 also adds credit
 * framing; nonempty data consumes one credit. Channel 3 adds a zero control
 * length byte. A nonzero control argument sets bit 7 of the remote selector,
 * bypasses the open-channel check, and omits data-credit framing.
 * @param control Zero for data; any nonzero byte for channel-control traffic.
 * @param channel Local channel 0–4.
 * @param data Payload source; must remain valid until transmission finishes.
 * @param length Payload byte count, no greater than IrdaGetMaxPayload().
 * @return Firmware byte status. Zero is not application-level peer acknowledgement.
 */
IrdaStatus_e IrdaSend(uint8_t control, IrdaChannel_e channel, const void* data, uint8_t length);

/** @brief Submit an IAS query and extract one response byte.
 *
 * Requires open channel 2 and no outstanding query. The parser accepts an IAS
 * integer and stores only its low byte, or stores 0xFF for unsupported results.
 * Caller must construct a valid query and matching transaction identifier.
 * @param value Required one-byte output, retained until response completion.
 * @param query Query packet; its first byte supplies the transaction identifier.
 * Keep its storage valid throughout this blocking call.
 * @param length Nonzero query packet byte count.
 * @return Peer response status byte, or a firmware error such as IRDA_TIMEOUT.
 */
uint8_t IrdaQueryByte(uint8_t* value, const uint8_t* query, uint8_t length);

/** @brief Enter the firmware IrDA environment.
 *
 * Saves and disables idle sleep, resets the transport, and clears the service
 * status callback. AS3000 3.01 first probes the infrared hardware; Neo skips that probe.
 * @param previous_idle_sleep Required one-byte output, unchanged if the AS probe fails.
 * @return Zero on setup, -1302 if the AS3000 3.01 hardware probe rejects it.
 */
int32_t IrdaBegin(uint8_t* previous_idle_sleep);

/** @brief Leave the IrDA environment and restore idle sleep.
 *
 * Shuts down the UART transport, clears the status callback, and resets channel state.
 * @param previous_idle_sleep Byte previously saved by IrdaBegin().
 */
void IrdaEnd(uint8_t previous_idle_sleep);

#endif
