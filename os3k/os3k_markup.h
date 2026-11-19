#ifndef _OS3K_MARKUP_H_
#define _OS3K_MARKUP_H_

/** @file os3k_markup.h
 * @brief Bounded ROM markup scanning and borrowed element spans on newer Neo.
 *
 * These operations use shared parser state and are not reentrant. Input spans
 * must remain readable while element records are used. This is a restricted
 * ROM dialect, not a general XML implementation. Betawise wrappers normalize
 * the native six-byte aggregate result and its GCC return convention.
 */
#include <stdint.h>

/** @brief Status values produced by the ROM markup scanner. */
typedef enum _MarkupStatus_e {
    MARKUP_OK = 0, /**< Operation completed; a lookup can still have no matches. */
    MARKUP_INVALID_NAME = 1, /**< Invalid name-state transition. */
    MARKUP_SYNTAX_ERROR = 2, /**< Invalid markup or entity syntax. */
    MARKUP_INCOMPLETE = 5, /**< Input ended before the construct was complete. */
    MARKUP_DEPTH_LIMIT = 6, /**< Nesting exceeds the ten-level parser stack. */
    MARKUP_NAME_TOO_LONG = 7, /**< Element name is longer than twenty bytes. */
    MARKUP_ELEMENT_LIMIT = 8, /**< Accepted element records filled the supplied array. */
    MARKUP_CLOSE_MISMATCH = 9, /**< Closing name does not match the open element. */
    MARKUP_NO_ROOT = 10, /**< No root element was established. */
    MARKUP_MULTIPLE_ROOTS = 11, /**< More than one root element was encountered. */
    MARKUP_EMPTY_SPAN = 12, /**< Element's attribute span is empty; see extraction caveat. */
    MARKUP_OUTPUT_FULL = 13 /**< Output buffer could not hold the complete value. */
} MarkupStatus_e;

/** @brief Borrowed spans describing one accepted element; sixteen bytes. */
typedef struct _MarkupElement_t {
    const char* attributesBegin; /**< Beginning of the opening-tag attribute span. */
    const char* attributesEnd; /**< Exclusive end of the opening-tag attribute span. */
    const char* contentBegin; /**< First content byte after the opening tag. */
    const char* contentEnd; /**< Exclusive content end at the closing tag. */
} MarkupElement_t;

/** @brief Six-byte parser result under the project's two-byte alignment ABI. */
typedef struct _MarkupResult_t {
    MarkupStatus_e status; /**< Operation status; one byte with -fshort-enums. */
    uint8_t reserved; /**< Alignment byte; Betawise sets it to zero. */
    int32_t offset; /**< Error/progress offset; zeroed by the wrapper on success. */
} MarkupResult_t;

/** @brief Select elements to retain during MarkupParse().
 *
 * Called synchronously with inherited A5. Returning zero validates the element
 * without advancing the output index. NULL as a filter retains no elements, but still requires valid element-array storage.
 * @param path NUL-terminated absolute path such as /root/child.
 * @param name NUL-terminated current element name within the parser's path storage.
 * @param depth One-based nesting depth.
 * @param index Zero-based output position proposed for this element.
 * @return Nonzero to retain the element; zero to omit it.
 */
typedef int16_t (*MarkupFilter_t)(const char* path, const char* name, int8_t depth, int8_t index);

/** @brief Parse an input span.
 * @param begin First input byte.
 * @param end Exclusive input end.
 * @param elements Output array retaining borrowed spans for accepted elements.
 * @param capacity Number of output records, from 1 through 127.
 * @param filter Acceptance callback; NULL performs syntax validation without retention.
 * @return Parser status and error offset from begin. Offset is zero on success.
 */
MarkupResult_t MarkupParse(const char* begin, const char* end, MarkupElement_t* elements,
                           int8_t capacity, MarkupFilter_t filter);

/** @brief Extract an attribute.
 *
 * A missing attribute returns success with length zero and can leave output
 * unchanged. Values are NUL-terminated when copied. Entity references are decoded.
 * @param element Existing borrowed element record.
 * @param name NUL-terminated attribute name, matched case-sensitively.
 * @param output Writable character buffer.
 * @param capacity Buffer size including terminator; at least one.
 * @param length Output copied byte count; can remain unchanged on an empty span.
 * @return Status and parser offset. An empty attribute span returns MARKUP_EMPTY_SPAN.
 */
MarkupResult_t MarkupGetAttribute(const MarkupElement_t* element, const char* name,
                                  char* output, int16_t capacity, int16_t* length);

/** @brief Copy element content and decode entities.
 *
 * The ROM tests the attribute span for emptiness before reading content.
 * Consequently an element without attributes can take the empty-span path
 * even when its content is nonempty. Nested tags are copied as content bytes.
 * @param element Existing borrowed element record.
 * @param output Writable character buffer; always terminated on the normal path.
 * @param capacity Buffer size including terminator; at least one.
 * @param length Output copied byte count.
 * @return Status; on truncation offset is the negative number of input bytes consumed.
 */
MarkupResult_t MarkupGetText(const MarkupElement_t* element, char* output,
                             int16_t capacity, int16_t* length);

/** @brief Find descendant elements by path.
 * @param parent Element whose content span is searched.
 * @param path NUL-terminated path relative to parent, with leading slash, e.g. /child.
 * @param elements Output array of borrowed element records.
 * @param capacity Available records, from 1 through 127.
 * @param count Output number of matched records, written as one byte.
 * @return Status and error offset relative to the parent's content beginning.
 */
MarkupResult_t MarkupFindElements(const MarkupElement_t* parent, const char* path,
                                  MarkupElement_t* elements, int8_t capacity, int8_t* count);

/** @brief Read the first matching child's attribute.
 *
 * The ROM uses a fixed eighteen-byte buffer limit, irrespective of its unused
 * legacy capacity argument. A missing child leaves output and length unchanged.
 * @param parent Parent element.
 * @param path Relative child path beginning with slash.
 * @param name Attribute name.
 * @param output Storage for at least eighteen bytes, including terminator.
 * @param length Output copied byte count when a child is found.
 * @return Lookup/extraction result, with success offset normalized to zero.
 */
MarkupResult_t MarkupGetChildAttribute(const MarkupElement_t* parent, const char* path,
                                       const char* name, char output[18], int16_t* length);

/** @brief Read the first matching child's content.
 * @param parent Parent element.
 * @param path Relative child path beginning with slash.
 * @param output Writable content buffer; unchanged when no child matches.
 * @param capacity Buffer size including terminator; at least one.
 * @param length Output copied byte count when a child is found.
 * @return Lookup/extraction result; the MarkupGetText() attribute-span caveat applies.
 */
MarkupResult_t MarkupGetChildText(const MarkupElement_t* parent, const char* path,
                                  char* output, int16_t capacity, int16_t* length);

#endif
