#ifndef _OS3K_WIRELESS_H_
#define _OS3K_WIRELESS_H_

/** @file os3k_wireless.h
 * @brief Wireless packets, association, transfers, and receive status on newer Neo.
 *
 * Contracts cover the inspected Neo 3.21 ROM. These entries are outside
 * the AS3000 3.01 and Neo 3.01 syscall tables.
 */
#include <stdint.h>
#include <stdbool.h>

/** @brief Read the cached wireless receive status.
 *
 * Available in the inspected Neo 3.21 ROM; outside the AS3000 3.01 and November
 * Neo 3.01 tables. This reads the cached word without clearing it. Successful
 * receive processing stores 0xFF00 OR the byte at offset 7 of the received
 * packet. The getter does not report whether new data has arrived since the
 * previous call. The low-byte protocol meanings are not established.
 * @return Cached 16-bit status; higher bits of D0 are unspecified.
 */
uint16_t WirelessGetLastReceiveStatus(void);

/** @brief Framing bits retained in a wireless packet's flags byte. */
typedef enum _WirelessPacketFlags_e {
    WIRELESS_PACKET_FIRST = 1, /**< First fragment of a transfer. */
    WIRELESS_PACKET_LAST = 2 /**< Last fragment of a transfer. */
} WirelessPacketFlags_e;

/** @brief Packet header followed by variable payload and a final checksum byte.
 *
 * All multi-byte wire fields are big-endian. Receive storage must provide at
 * least 2068 bytes in total; the receive syscall has no buffer-size argument.
 */
typedef struct _WirelessPacket_t {
    uint8_t sync; /**< Packet marker, 0xA5. */
    uint8_t flags; /**< Fragment/control flags. */
    uint16_t length; /**< Total frame length including header and checksum. */
    uint16_t command; /**< Protocol command word. */
    uint8_t payload[]; /**< Payload followed by checksum making the byte sum zero modulo 256. */
} WirelessPacket_t;

/** @brief Association manager phases supplied to its callback. */
typedef enum _WirelessAssociationPhase_e {
    WIRELESS_ASSOCIATION_SELECT = 5, /**< Network-selection request completed immediately. */
    WIRELESS_ASSOCIATION_WAIT = 6, /**< Association completed after polling. */
    WIRELESS_DISASSOCIATION_WAIT = 7 /**< Disassociation completed after polling. */
} WirelessAssociationPhase_e;

/** @brief Notification from the asynchronous association manager.
 *
 * The ROM supplies inherited A5, ignores the return, and clears the callback
 * after notification. Network information is borrowed system storage.
 * @param phase Manager phase that produced the notification.
 * @param success True for the manager's completion path; false for its failure path.
 * @param networkInfo Borrowed network information, or NULL on failure.
 */
typedef void (*WirelessAssociationCallback_t)(WirelessAssociationPhase_e phase,
                                             bool success, const void* networkInfo);

/** @brief Transfer directions accepted by WirelessConfigureTransfer(). */
typedef enum _WirelessTransferMode_e {
    WIRELESS_TRANSFER_SEND = 0x0802, /**< Prepare an upload of the supplied total byte count. */
    WIRELESS_TRANSFER_RECEIVE = 0x0803, /**< Send a receive request and install its callback. */
    WIRELESS_TRANSFER_RESET = 0xFFFF /**< Reset transport/transfer state; may block. */
} WirelessTransferMode_e;

/** @brief Events delivered to a transfer callback. */
typedef enum _WirelessTransferEvent_e {
    WIRELESS_TRANSFER_DONE = 0, /**< Transfer completed, or an incoming complete packet was delivered. */
    WIRELESS_TRANSFER_ABORTED = 7, /**< Protocol abort notification. */
    WIRELESS_TRANSFER_TIMEOUT = 9, /**< Transport/response timeout. */
    WIRELESS_TRANSFER_DATA = 11 /**< Supply an upload block or consume an incoming data block. */
} WirelessTransferEvent_e;

/** @brief Supply/consume transfer data in the system pump's context.
 *
 * A5 is inherited. Incoming bytes are copied separately with
 * WirelessReadTransfer(); upload data is submitted with WirelessWriteTransfer().
 * Other protocol status bytes can also be forwarded as event values.
 * @param offset Current transfer byte offset.
 * @param length Requested upload length or received frame length; zero for status events.
 * @param event Data request, completion, or protocol status.
 * @return Low-byte application status; an upload return value 9 triggers the ROM timeout path.
 */
typedef uint8_t (*WirelessTransferCallback_t)(uint32_t offset, uint16_t length,
                                            WirelessTransferEvent_e event);

/** @brief Receive one protocol frame.
 * @param packet Writable storage of at least 2068 bytes, cast to this header type.
 * @return Frame length, zero when no packet is ready, -2 on request failure,
 * -3 when synchronization fails, -4 on checksum failure, or 4 on handshake timeout.
 */
int16_t WirelessReceivePacket(WirelessPacket_t* packet);

/** @brief Send a framed protocol packet with up to six attempts.
 * @param command Command word; bit 15 suppresses the response-acknowledgment exchange.
 * @param payload Binary payload, or NULL to transmit bytes already prepared in the shared TX buffer.
 * @param length Payload length, at most 2058 bytes.
 * @param flags Flags byte retained in the frame.
 * @return Low-byte transport status: zero on success; nonzero on failure.
 */
uint8_t WirelessSendPacket(uint16_t command, const void* payload, uint16_t length,
                           WirelessPacketFlags_e flags);

/** @brief Read the cached wireless-availability flag.
 * @return True when the firmware's wireless initialization recognized a supported device.
 */
bool WirelessIsAvailable(void);

/** @brief Test whether the association manager is active.
 * @return True when its state byte is nonzero; this is not a transfer-ready query.
 */
bool WirelessIsOperationPending(void);

/** @brief Schedule network selection/association if the manager is idle.
 * @param callback Completion callback, or NULL. An already-active operation leaves it unchanged.
 */
void WirelessBeginAssociation(WirelessAssociationCallback_t callback);

/** @brief Schedule disassociation if the manager is idle.
 *
 * Also draws a native status message and waits about two seconds before returning.
 * @param callback Completion callback, or NULL. An already-active operation is unchanged.
 */
void WirelessBeginDisassociation(WirelessAssociationCallback_t callback);

/** @brief Exchange device-identification records with the wireless module.
 *
 * Sends protocol commands 6 and 7. The native routine can reuse the first
 * receive result when processing the second exchange, so success does not
 * establish a distinct data response to each command. Requires a working
 * wireless transport and advancing clocks; it can block while waiting.
 * @return Zero on success, one on failure, in the low byte.
 */
uint8_t WirelessSynchronizeDeviceInfo(void);

/** @brief Submit the next upload block.
 * @param data Readable binary data for this block.
 * @param length Requested block size; clamped to the remaining transfer size.
 * @return Submitted byte count, or 8 if not ready, 9 on timeout, 10 if framing is too large.
 */
uint16_t WirelessWriteTransfer(const void* data, uint16_t length);

/** @brief Copy the current received transfer block.
 *
 * Copies the length advertised in the cached frame without checking capacity.
 * @param output Storage large enough for that block; no terminator is added.
 * @return Copied byte count, in the low word.
 */
uint16_t WirelessReadTransfer(void* output);

/** @brief Configure the shared transfer engine.
 * @param mode Upload, receive request, or reset.
 * @param subtype Protocol subtype/request byte.
 * @param size Total upload size; in receive mode zero requests a fresh transfer and nonzero a continuation.
 * @param callback Transfer callback, retained until engine reset/completion.
 * @return Low-byte status: 0 accepted, 3 request timeout, 5 invalid mode, 8 when both send and receive engines are active.
 * The ROM permits reconfiguration when either engine is idle; callers must
 * coordinate ownership to avoid replacing a live transfer.
 */
uint8_t WirelessConfigureTransfer(WirelessTransferMode_e mode, uint8_t subtype,
                                  uint32_t size, WirelessTransferCallback_t callback);

/** @brief Copy the selected network record's leading identifier string.
 *
 * This is an unbounded NUL-terminated copy from shared association storage.
 * @param output Writable storage of at least 43 bytes. This is not a freshness query.
 */
void WirelessCopySelectedNetworkId(char* output);

#endif
