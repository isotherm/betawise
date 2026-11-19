#ifndef _OS3K_SERIAL_H_
#define _OS3K_SERIAL_H_

/** @file os3k_serial.h
 * @brief Legacy UART configuration, polling transfers, and interrupt callbacks.
 *
 * These interfaces apply to AS3000 3.01 and Neo 3.21 firmware.
 * Transfers are binary; neither function appends a NUL. Configuration, callbacks, and interrupt masks are
 * system-wide, so these operations require coordination with other users.
 *
 * @par Neo compatibility
 * These legacy entries do not consistently select the same UART on Neo:
 * configuration of the wired mode, baud changes, status queries, reads, and
 * receiver disabling address UART1; writes, callback interrupt enabling, and
 * receiver enabling address UART2. The common callback handler still samples
 * UART1. Infrared configuration initializes UART2 using a separate ROM handler.
 * Do not assume the AS3000 3.01 receive/callback workflow works unchanged on Neo.
 * UART routing is a firmware compatibility constraint, not a channel selector.
 */

#include <stdint.h>

/** @brief Packed SerialConfigure() settings; combine exactly one from each group.
 *
 * The five groups occupy separate nibbles. Zero is not a default setting.
 * Bits outside these groups are ignored. Neo's infrared path ignores all
 * groups except the link selection, as described on SerialConfigure().
 */
typedef enum _SerialConfig_e {
    SERIAL_STOP_BITS_1 = 0x00001, /**< One stop bit. */
    SERIAL_STOP_BITS_2 = 0x00002, /**< Two stop bits. */
    SERIAL_PARITY_NONE = 0x00010, /**< No parity bit. */
    SERIAL_PARITY_EVEN = 0x00020, /**< Even parity. */
    SERIAL_PARITY_ODD = 0x00030, /**< Odd parity. */
    SERIAL_DATA_BITS_8 = 0x00100, /**< Eight data bits. */
    SERIAL_DATA_BITS_7 = 0x00200, /**< Seven data bits. */
    SERIAL_IGNORE_CTS = 0x01000, /**< Permit transmission regardless of CTS. */
    SERIAL_USE_CTS = 0x02000, /**< Gate transmission using CTS. */
    SERIAL_LINK_WIRED = 0x10000, /**< Normal UART signaling through the wired port. */
    SERIAL_LINK_INFRARED = 0x20000 /**< Infrared UART signaling. */
} SerialConfig_e;

/** @brief SerialRead() options; combine flags with bitwise OR. */
typedef enum _SerialReadFlags_e {
    SERIAL_READ_BLOCKING = 0, /**< Wait for the requested length unless another option stops the read. */
    SERIAL_READ_TERMINATOR = 0x04, /**< Stop after storing the terminator byte; include it in the count. */
    SERIAL_READ_AVAILABLE = 0x08, /**< Return when no byte is ready; overrides SERIAL_READ_IDLE_TIMEOUT. */
    SERIAL_READ_IDLE_TIMEOUT = 0x10 /**< Limit each wait for the next byte using timeout_ms. */
} SerialReadFlags_e;

/** @brief SerialWrite() options; combine flags with bitwise OR.
 *
 * CTS options are considered only while no transmit FIFO slot is available.
 * They do not prevent initially filling available slots while CTS is high.
 */
typedef enum _SerialWriteFlags_e {
    SERIAL_WRITE_BLOCKING = 0, /**< Wait until every byte has been queued. */
    SERIAL_WRITE_AVAILABLE = 0x02, /**< Return immediately when the transmit FIFO has no slot. */
    SERIAL_WRITE_RETURN_ON_CTS_HIGH = 0x04, /**< Return if the FIFO has no slot and CTS is high. */
    SERIAL_WRITE_CTS_TIMEOUT = 0x08 /**< Time a wait begun with a full FIFO and CTS high. */
} SerialWriteFlags_e;

/** @brief Bits in the UART receive-status byte passed to SerialReceiveCallback_t.
 *
 * Multiple bits can be set. SerialGetStatus() exposes only the upper nibble;
 * callbacks expose all eight bits. Data is valid only with SERIAL_RX_READY.
 */
typedef enum _SerialRxStatus_e {
    SERIAL_RX_PARITY_ERROR = 0x01, /**< Parity error on the sampled character. */
    SERIAL_RX_BREAK = 0x02, /**< Break detected on the sampled character. */
    SERIAL_RX_FRAME_ERROR = 0x04, /**< Invalid stop bit on the sampled character. */
    SERIAL_RX_OVERRUN = 0x08, /**< Receive FIFO overflow. */
    SERIAL_RX_OLD_DATA = 0x10, /**< Receive FIFO data has aged past the UART idle threshold. */
    SERIAL_RX_READY = 0x20, /**< Sample includes a received byte. */
    SERIAL_RX_HALF_FULL = 0x40, /**< Receive FIFO has reached its half-full threshold. */
    SERIAL_RX_FULL = 0x80 /**< Receive FIFO is full. */
} SerialRxStatus_e;

/** @brief UART transmit-status/control byte returned by status queries and callbacks.
 *
 * Multiple bits can be set. These values differ from SerialTxEvent_e, which
 * selects interrupts rather than reporting the transmitter register.
 */
typedef enum _SerialTxStatus_e {
    SERIAL_TX_CTS_CHANGED = 0x01, /**< CTS changed since its change flag was cleared. */
    SERIAL_TX_CTS_HIGH = 0x02, /**< CTS input is high (deasserted). */
    SERIAL_TX_BUSY = 0x04, /**< A character is being transmitted or remains queued. */
    SERIAL_TX_IGNORING_CTS = 0x08, /**< Transmitter is configured to ignore CTS. */
    SERIAL_TX_SENDING_BREAK = 0x10, /**< Continuous break output is enabled. */
    SERIAL_TX_AVAILABLE = 0x20, /**< At least one transmit FIFO slot is available. */
    SERIAL_TX_BELOW_HALF = 0x40, /**< Transmit FIFO is below its half-full threshold. */
    SERIAL_TX_EMPTY = 0x80 /**< Transmit FIFO is empty; this alone does not imply SERIAL_TX_BUSY is clear. */
} SerialTxStatus_e;

/** @brief Interrupt conditions requested by SerialWrite() for its one-shot callback. */
typedef enum _SerialTxEvent_e {
    SERIAL_TX_EVENT_NONE = 0, /**< Do not enable a transmit interrupt condition. */
    SERIAL_TX_EVENT_AVAILABLE = 0x01, /**< Notify when a transmit FIFO slot is available. */
    SERIAL_TX_EVENT_BELOW_HALF = 0x02, /**< Notify when transmit FIFO occupancy is below half. */
    SERIAL_TX_EVENT_EMPTY = 0x04, /**< Notify when the transmit FIFO is empty. */
    SERIAL_TX_EVENT_CTS_CHANGED = 0x40 /**< Notify when CTS changes. */
} SerialTxEvent_e;

/** @brief Receive notification called directly from the UART interrupt handler.
 *
 * The handler consumes ready bytes and also calls once with the final sample
 * for which SERIAL_RX_READY is clear. Check status before consuming data.
 * There is no return value or applet A5 data-base switch. Keep the handler
 * short, avoid blocking calls, and arrange any data base needed for globals.
 * The callback and its storage must outlive registration; unregister before exit.
 * @param status Complete high byte of the sampled receive register.
 * @param data Low byte of that sample; invalid when SERIAL_RX_READY is clear.
 */
typedef void (*SerialReceiveCallback_t)(SerialRxStatus_e status, uint8_t data);

/** @brief One-shot transmitter notification from the UART interrupt handler.
 *
 * The ROM clears the callback pointer and disables transmit interrupts before
 * calling. It does not switch the applet A5 data base. Keep code/storage valid
 * until notification or explicit removal with SerialWrite(). A FIFO-empty
 * notification need not mean the last character has left the shift register.
 * @param status Complete transmit register high byte sampled by the handler.
 */
typedef void (*SerialTransmitCallback_t)(SerialTxStatus_e status);

/** @brief Operation succeeded; inspect the transfer count for partial completion. */
#define SERIAL_OK 0
/** @brief Unsupported baud rate. */
#define SERIAL_ERROR_BAUD_RATE (-502)
/** @brief Invalid parity-selection nibble. */
#define SERIAL_ERROR_PARITY (-503)
/** @brief Invalid stop-bit-selection nibble. */
#define SERIAL_ERROR_STOP_BITS (-504)
/** @brief Invalid data-bit-selection nibble. */
#define SERIAL_ERROR_DATA_BITS (-505)
/** @brief Invalid CTS-selection nibble. */
#define SERIAL_ERROR_CTS_MODE (-509)
/** @brief Invalid wired/infrared-selection nibble. */
#define SERIAL_ERROR_LINK (-511)
/** @brief SerialRead() was given a zero capacity. */
#define SERIAL_ERROR_READ_SIZE (-512)
/** @brief Receive overrun; the erroneous sample is consumed but not stored. */
#define SERIAL_ERROR_OVERRUN (-513)
/** @brief Receive framing error; the erroneous sample is consumed but not stored. */
#define SERIAL_ERROR_FRAME (-514)
/** @brief Timeout timer could not be created, including an excessive timeout. */
#define SERIAL_ERROR_TIMER (-515)
/** @brief SerialConfigure() has not succeeded since the last SerialReset(). */
#define SERIAL_ERROR_NOT_CONFIGURED (-516)

/** @brief Disable serial operation and clear both callbacks and cached masks.
 *
 * Also restores the ROM's idle serial pin configuration and
 * clears its configured flag. Neo also invokes the UART2 shutdown helper.
 * @return Always zero.
 */
uint32_t SerialReset(void);

/** @brief Configure and enable the serial port.
 *
 * Enables receive and transmit and automatic RTS flow control.
 * Errors can leave hardware partially changed; reset/reconfigure after failure.
 * AS3000 3.01 also reads and discards one receive-register sample during setup.
 * AS3000 3.01 selects wired or infrared mode using the same UART. Neo 3.21 firmware
 * infrared branch instead initializes UART2 to fixed 38400 baud, 8N1, ignoring
 * CTS; it ignores baud_rate and the other configuration groups, even invalid
 * ones. Neo's wired branch configures UART1. See this header's compatibility note.
 * @param baud_rate One of 9600, 19200, 38400, 57600, or 115200 bits per second.
 * @param config OR of one setting from each SerialConfig_e group.
 * @return SERIAL_OK, or SERIAL_ERROR_BAUD_RATE, SERIAL_ERROR_PARITY,
 * SERIAL_ERROR_STOP_BITS, SERIAL_ERROR_DATA_BITS, SERIAL_ERROR_CTS_MODE,
 * or SERIAL_ERROR_LINK. Validation stops at the first invalid group.
 */
int32_t SerialConfigure(uint32_t baud_rate, SerialConfig_e config);

/** @brief Change the UART baud divisor without configuring the other settings.
 *
 * Does not require or set the configured flag. Addresses UART1
 * on Neo, including after infrared configuration selected UART2.
 * @param baud_rate One of 9600, 19200, 38400, 57600, or 115200 bits per second.
 * @return SERIAL_OK or SERIAL_ERROR_BAUD_RATE; failure leaves the divisor alone.
 */
int32_t SerialSetBaudRate(uint32_t baud_rate);

/** @brief Sample transmit status and receive FIFO status without consuming a byte.
 *
 * Reads UART1 on Neo. When unconfigured, AS3000 3.01 writes zero
 * to both outputs, while Neo 3.21 firmware writes 0xFF to both. No pointer may be NULL.
 * @param tx_status Output byte containing SerialTxStatus_e bits.
 * @param rx_status Output byte containing only the upper four SerialRxStatus_e bits.
 */
void SerialGetStatus(uint8_t* tx_status, uint8_t* rx_status);

/** @brief Install or remove the shared receive interrupt callback.
 *
 * Stores the pointer even before configuration. A non-NULL
 * callback selects receive interrupts; NULL removes them. If configured, this
 * replaces the UART interrupt-enable byte from the cached mask, which can also
 * remove transmit interrupts previously enabled by SerialWrite(). Neo writes
 * UART2's interrupt mask although the common callback handler reads UART1.
 * Do not use polling reads and a callback as competing consumers of the FIFO.
 * @param callback Handler to install, or NULL to unregister.
 */
void SerialSetReceiveCallback(SerialReceiveCallback_t callback);

/** @brief Clear the UART receiver-enable bit without clearing configured state.
 *
 * Addresses UART1 on both systems. Transmission and
 * callback registration are retained; a blocking read may therefore still wait.
 */
void SerialDisableReceiver(void);

/** @brief Set the UART receiver-enable bit, then consume one receive-register sample.
 *
 * Does not enable the UART as a whole or set configured state.
 * Addresses UART1 on AS3000 3.01 and UART2 on Neo 3.21 firmware. The sampled byte is
 * discarded if present; toggling receiver enable can also reset the hardware FIFO.
 */
void SerialEnableReceiver(void);

/** @brief Read bytes from the UART receive FIFO into a caller-owned buffer.
 *
 * Polls UART1 on Neo. Checks overrun before framing error;
 * parity errors are not rejected. Timeout expiry returns SERIAL_OK with a
 * short count. Timed mode restarts its timer for each idle gap, not per call.
 * It needs a running timer service and enabled timer interrupts. Only the
 * documented flag bits affect the implementation.
 * @param buffer Writable storage for capacity bytes; required for a nonzero read.
 * @param capacity Maximum number of bytes to store; must be nonzero.
 * @param received Required uint32_t output written on every return path, including errors.
 * @param flags Blocking, available-only, terminator, and/or idle-timeout options.
 * @param terminator Byte that ends a read when SERIAL_READ_TERMINATOR is set.
 * @param timeout_ms Idle wait in milliseconds, used only in timed mode. TimerStart()
 * accepts at most 10000 and clamps values below 3 to 3.
 * @return SERIAL_OK, SERIAL_ERROR_NOT_CONFIGURED, SERIAL_ERROR_READ_SIZE,
 * SERIAL_ERROR_OVERRUN, SERIAL_ERROR_FRAME, or SERIAL_ERROR_TIMER.
 */
int32_t SerialRead(uint8_t* buffer, uint32_t capacity, uint32_t* received,
                   SerialReadFlags_e flags, uint8_t terminator, uint16_t timeout_ms);

/** @brief Queue bytes for transmission and optionally arm a one-shot notification.
 *
 * Queuing completion does not wait for the physical transmitter
 * to finish. Polls/writes UART2 on Neo and first disables that UART's receiver.
 * A zero length is valid and can register/remove a callback without sending.
 *
 * Timed mode applies only when no FIFO slot is available and CTS is high.
 * After starting a timer, it waits for a slot or expiry without rechecking CTS.
 * Expiry and early-return options return SERIAL_OK with a partial count.
 * Timer interrupts and the timer service must run for timed waits to expire.
 *
 * A non-NULL callback replaces the pointer, but its cached status-condition mask
 * accumulates until cleared by a NULL callback, configuration, or reset. Hardware
 * interrupt enables are ORed into the existing register. To remove pending
 * transmit enables as well, reset or update the receive callback mask. On the
 * Neo 3.21, notification enables target UART2 but the common handler examines
 * UART1; successful notification is not guaranteed by this legacy entry.
 * @param buffer Readable length-byte input; may be NULL when length is zero.
 * @param length Number of bytes to queue; no terminator processing.
 * @param written Required uint32_t output written on every return path, including errors.
 * @param flags Waiting/early-return policy; other bits are ignored.
 * @param timeout_ms Milliseconds for a CTS-blocked wait. TimerStart() accepts
 * at most 10000 and clamps values below 3 to 3; ignored unless timed waiting occurs.
 * @param callback One-shot notification handler, or NULL to clear the handler and cached conditions.
 * @param events OR of SerialTxEvent_e conditions; ignored for a NULL callback.
 * @return SERIAL_OK, SERIAL_ERROR_NOT_CONFIGURED, or SERIAL_ERROR_TIMER.
 */
int32_t SerialWrite(const uint8_t* buffer, uint32_t length, uint32_t* written,
                    SerialWriteFlags_e flags, uint16_t timeout_ms,
                    SerialTransmitCallback_t callback, SerialTxEvent_e events);

#endif
