#ifndef _OS3K_SPEECH_H_
#define _OS3K_SPEECH_H_

/** @file os3k_speech.h
 * @brief Queued text-to-speech and system speech controls on newer Neo firmware.
 *
 * These entries exist in the inspected Neo 3.21 ROM, and are outside the
 * AS3000 3.01 and Neo 3.01 syscall tables. Speech uses shared UART and
 * timer resources and requires the speech peripheral for audible output.
 */
#include <stdint.h>

/** @brief Notifications requested for a queued speech item; flags may be combined. */
typedef enum _SpeechNotifyFlags_e {
    SPEECH_NOTIFY_NONE = 0, /**< Request no notifications. */
    SPEECH_NOTIFY_STARTED = 1, /**< Notify when the ROM submits the item. */
    SPEECH_NOTIFY_FINISHED = 2 /**< Notify when the ROM advances past the item. */
} SpeechNotifyFlags_e;

/** @brief Events delivered to a queued item's speech callback. */
typedef enum _SpeechEvent_e {
    SPEECH_EVENT_STARTED = 1, /**< The ROM submitted the item's command. */
    SPEECH_EVENT_FINISHED = 2 /**< The ROM advanced past the item, possibly after a timeout. */
} SpeechEvent_e;

/** @brief Result of accepting an item into the speech queue. */
typedef enum _SpeechQueueResult_e {
    SPEECH_QUEUE_OK = 0, /**< Accepted; this does not confirm audible playback. */
    SPEECH_QUEUE_DISABLED = 1, /**< Global speech is disabled; item was not queued. */
    SPEECH_QUEUE_FULL = 2 /**< All 200 pending queue positions are occupied. */
} SpeechQueueResult_e;

/** @brief Operations accepted by SpeechControl(). */
typedef enum _SpeechControl_e {
    SPEECH_ENABLE = 0, /**< Enable speech; return the previous enable byte. */
    SPEECH_DISABLE = 1, /**< Disable future enqueues; return the previous enable byte. */
    SPEECH_TOGGLE = 2, /**< Toggle enable bit zero; return the previous enable byte. */
    SPEECH_CLEAR = 3, /**< Reset the queue and active speech state; return the enable byte. */
    SPEECH_MENU_READBACK_ENABLE = 6, /**< Enable menu-selection readback; return its previous byte. */
    SPEECH_MENU_READBACK_DISABLE = 7, /**< Disable menu-selection readback; return its previous byte. */
    SPEECH_SHOW_SETTINGS = 8, /**< Clear active speech and open the settings menu; return the previous enable byte. */
    SPEECH_GET_ENABLED = 9, /**< Return the current enable byte without changing it. */
    SPEECH_QUEUE_IS_EMPTY = 10 /**< Return 1 if no pending items remain, otherwise 0; excludes the active item. */
} SpeechControl_e;

/** @brief Callback for a speech item's requested notifications.
 *
 * The ROM passes two stack arguments and ignores the return value. It does
 * not restore the applet's A5/GOT context before calling. Code accessing applet
 * globals must arrange the correct context, as with timer callbacks. A
 * finished notification is not proof that the peripheral spoke successfully.
 * @param event Start or finish notification.
 * @param userData Opaque value supplied to SpeechEnqueue().
 */
typedef void (*SpeechCallback_t)(SpeechEvent_e event, void* userData);

/** @brief Queue a NUL-terminated string for speech.
 *
 * The ROM retains the text pointer without copying its contents. Keep the
 * text and callback valid until completion or clearing of the queue. An
 * enqueue can start processing synchronously and can invoke the callback
 * before this function returns. The function does not validate text length;
 * the lower transport limits a formatted command to 35 bytes including its
 * protocol suffix. Oversize commands can fail after queue acceptance.
 * @param notifyFlags Requested start/finish notifications; combine flag values.
 * @param text NUL-terminated text; must remain readable while queued or active.
 * @param callback Notification callback, or NULL to suppress callbacks.
 * @param userData Opaque callback argument retained with this item.
 * @return Acceptance result; only D0's low byte is defined.
 */
SpeechQueueResult_e SpeechEnqueue(SpeechNotifyFlags_e notifyFlags, const char* text,
                                  SpeechCallback_t callback, void* userData);

/** @brief Control speech enablement, queue state, or settings.
 *
 * Disabling only changes the enable byte; it does not clear queued items.
 * Clearing discards pending notifications. The settings operation blocks
 * until dismissed and can change global readback, voice speed, and volume.
 * @param operation Operation to perform. Values 4, 5, and values above 10 are invalid.
 * @return Operation-dependent byte described by SpeechControl_e; 0xFF for an
 * invalid operation. Higher bits of D0 are unspecified.
 */
uint8_t SpeechControl(SpeechControl_e operation);

#endif
