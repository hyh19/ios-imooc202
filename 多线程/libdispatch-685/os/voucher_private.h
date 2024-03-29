/*
 * Copyright (c) 2013-2014 Apple Inc. All rights reserved.
 *
 * @APPLE_APACHE_LICENSE_HEADER_START@
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @APPLE_APACHE_LICENSE_HEADER_END@
 */

#ifndef __OS_VOUCHER_PRIVATE__
#define __OS_VOUCHER_PRIVATE__

#ifndef __linux__
#include <os/base.h>
#endif
#ifdef __APPLE__
#include <os/object.h>
#include <mach/mach.h>
#endif
#if __has_include(<bank/bank_types.h>)
#include <bank/bank_types.h>
#endif
#if __has_include(<sys/persona.h>)
#include <sys/persona.h>
#endif

#ifndef __DISPATCH_BUILDING_DISPATCH__
#include <dispatch/dispatch.h>
#endif /* !__DISPATCH_BUILDING_DISPATCH__ */

#define OS_VOUCHER_SPI_VERSION 20150630

#if OS_VOUCHER_WEAK_IMPORT
#define OS_VOUCHER_EXPORT OS_EXPORT OS_WEAK_IMPORT
#else
#define OS_VOUCHER_EXPORT OS_EXPORT
#endif

DISPATCH_ASSUME_NONNULL_BEGIN

__BEGIN_DECLS

/*!
 * @group Voucher Transport SPI
 * SPI intended for clients that need to transport vouchers.
 */

/*!
 * @typedef voucher_t
 *
 * @abstract
 * Vouchers are immutable sets of key/value attributes that can be adopted on a
 * thread in the current process or sent to another process.
 *
 * @discussion
 * Voucher objects are os_objects (c.f. <os/object.h>). They are memory-managed
 * with the os_retain()/os_release() functions or -[retain]/-[release] methods.
 */
OS_OBJECT_DECL_CLASS(voucher);

/*!
 * @const VOUCHER_NULL
 * Represents the empty base voucher with no attributes.
 */
#define VOUCHER_NULL		((voucher_t)0)
/*!
 * @const VOUCHER_INVALID
 * Represents an invalid voucher
 */
#define VOUCHER_INVALID		((voucher_t)-1)

/*!
 * @function voucher_adopt
 *
 * @abstract
 * Adopt the specified voucher on the current thread and return the voucher
 * that had been adopted previously.
 *
 * @discussion
 * Adopted vouchers are automatically carried forward by the system to other
 * threads and processes (across IPC).
 *
 * Consumes a reference to the specified voucher.
 * Returns a reference to the previous voucher.
 *
 * @param voucher
 * The voucher object to adopt on the current thread.
 *
 * @result
 * The previously adopted voucher object.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_10,__IPHONE_8_0)
OS_VOUCHER_EXPORT OS_OBJECT_RETURNS_RETAINED OS_WARN_RESULT_NEEDS_RELEASE
OS_NOTHROW
voucher_t _Nullable
voucher_adopt(voucher_t _Nullable voucher OS_OBJECT_CONSUMED);

/*!
 * @function voucher_copy
 *
 * @abstract
 * Returns a reference to the voucher that had been adopted previously on the
 * current thread (or carried forward by the system).
 *
 * @result
 * The currently adopted voucher object.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_10,__IPHONE_8_0)
OS_VOUCHER_EXPORT OS_OBJECT_RETURNS_RETAINED OS_WARN_RESULT OS_NOTHROW
voucher_t _Nullable
voucher_copy(void);

/*!
 * @function voucher_copy_without_importance
 *
 * @abstract
 * Returns a reference to a voucher object with all the properties of the
 * voucher that had been adopted previously on the current thread, but
 * without the importance properties that are frequently attached to vouchers
 * carried with IPC requests. Importance properties may elevate the scheduling
 * of threads that adopt or retain the voucher while they service the request.
 * See xpc_transaction_begin(3) for further details on importance.
 *
 * @result
 * A copy of the currently adopted voucher object, with importance removed.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_10,__IPHONE_8_0)
OS_VOUCHER_EXPORT OS_OBJECT_RETURNS_RETAINED OS_WARN_RESULT OS_NOTHROW
voucher_t _Nullable
voucher_copy_without_importance(void);

/*!
 * @function voucher_replace_default_voucher
 *
 * @abstract
 * Replace process attributes of default voucher (used for IPC by this process
 * when no voucher is adopted on the sending thread) with the process attributes
 * of the voucher adopted on the current thread.
 *
 * @discussion
 * This allows a daemon to indicate from the context of an incoming IPC request
 * that all future outgoing IPC from the process should be marked as acting
 * "on behalf of" the sending process of the current IPC request (as long as the
 * thread sending that outgoing IPC is not itself in the direct context of an
 * IPC request, i.e. no voucher is adopted).
 *
 * If no voucher is adopted on the current thread or the current voucher does
 * not contain any process attributes, the default voucher is reset to the
 * default process attributes for the current process.
 *
 * CAUTION: Do NOT use this SPI without contacting the Darwin Runtime team.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_10,__IPHONE_8_0)
OS_VOUCHER_EXPORT OS_NOTHROW
void
voucher_replace_default_voucher(void);

/*!
 * @function voucher_decrement_importance_count4CF
 *
 * @abstract
 * Decrement external importance count of the mach voucher in the specified
 * voucher object.
 *
 * @discussion
 * This is only intended for use by CoreFoundation to explicitly manage the
 * App Nap state of an application following reception of a de-nap IPC message.
 *
 * CAUTION: Do NOT use this SPI without contacting the Darwin Runtime team.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_10,__IPHONE_8_0)
OS_VOUCHER_EXPORT OS_NOTHROW
void
voucher_decrement_importance_count4CF(voucher_t _Nullable voucher);

/*!
 * @group Voucher dispatch block SPI
 */

/*!
 * @typedef dispatch_block_flags_t
 * SPI Flags to pass to the dispatch_block_create* functions.
 *
 * @const DISPATCH_BLOCK_NO_VOUCHER
 * Flag indicating that a dispatch block object should not be assigned a voucher
 * object. If invoked directly, the block object will be executed with the
 * voucher adopted on the calling thread. If the block object is submitted to a
 * queue, this replaces the default behavior of associating the submitted block
 * instance with the voucher adopted at the time of submission.
 * This flag is ignored if a specific voucher object is assigned with the
 * dispatch_block_create_with_voucher* functions, and is equivalent to passing
 * the NULL voucher to these functions.
 */
#define DISPATCH_BLOCK_NO_VOUCHER (0x40)

/*!
 * @function dispatch_block_create_with_voucher
 *
 * @abstract
 * Create a new dispatch block object on the heap from an existing block and
 * the given flags, and assign it the specified voucher object.
 *
 * @discussion
 * The provided block is Block_copy'ed to the heap, it and the specified voucher
 * object are retained by the newly created dispatch block object.
 *
 * The returned dispatch block object is intended to be submitted to a dispatch
 * queue with dispatch_async() and related functions, but may also be invoked
 * directly. Both operations can be performed an arbitrary number of times but
 * only the first completed execution of a dispatch block object can be waited
 * on with dispatch_block_wait() or observed with dispatch_block_notify().
 *
 * The returned dispatch block will be executed with the specified voucher
 * adopted for the duration of the block body. If the NULL voucher is passed,
 * the block will be executed with the voucher adopted on the calling thread, or
 * with no voucher if the DISPATCH_BLOCK_DETACHED flag was also provided.
 *
 * If the returned dispatch block object is submitted to a dispatch queue, the
 * submitted block instance will be associated with the QOS class current at the
 * time of submission, unless one of the following flags assigned a specific QOS
 * class (or no QOS class) at the time of block creation:
 *  - DISPATCH_BLOCK_ASSIGN_CURRENT
 *  - DISPATCH_BLOCK_NO_QOS_CLASS
 *  - DISPATCH_BLOCK_DETACHED
 * The QOS class the block object will be executed with also depends on the QOS
 * class assigned to the queue and which of the following flags was specified or
 * defaulted to:
 *  - DISPATCH_BLOCK_INHERIT_QOS_CLASS (default for asynchronous execution)
 *  - DISPATCH_BLOCK_ENFORCE_QOS_CLASS (default for synchronous execution)
 * See description of dispatch_block_flags_t for details.
 *
 * If the returned dispatch block object is submitted directly to a serial queue
 * and is configured to execute with a specific QOS class, the system will make
 * a best effort to apply the necessary QOS overrides to ensure that blocks
 * submitted earlier to the serial queue are executed at that same QOS class or
 * higher.
 *
 * @param flags
 * Configuration flags for the block object.
 * Passing a value that is not a bitwise OR of flags from dispatch_block_flags_t
 * results in NULL being returned.
 *
 * @param voucher
 * A voucher object or NULL. Passing NULL is equivalent to specifying the
 * DISPATCH_BLOCK_NO_VOUCHER flag.
 *
 * @param block
 * The block to create the dispatch block object from.
 *
 * @result
 * The newly created dispatch block object, or NULL.
 * When not building with Objective-C ARC, must be released with a -[release]
 * message or the Block_release() function.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_10, __IPHONE_8_0)
DISPATCH_EXPORT DISPATCH_NONNULL3 DISPATCH_RETURNS_RETAINED_BLOCK
DISPATCH_WARN_RESULT DISPATCH_NOTHROW
dispatch_block_t
dispatch_block_create_with_voucher(dispatch_block_flags_t flags,
		voucher_t _Nullable voucher, dispatch_block_t block);

/*!
 * @function dispatch_block_create_with_voucher_and_qos_class
 *
 * @abstract
 * Create a new dispatch block object on the heap from an existing block and
 * the given flags, and assign it the specified voucher object, QOS class and
 * relative priority.
 *
 * @discussion
 * The provided block is Block_copy'ed to the heap, it and the specified voucher
 * object are retained by the newly created dispatch block object.
 *
 * The returned dispatch block object is intended to be submitted to a dispatch
 * queue with dispatch_async() and related functions, but may also be invoked
 * directly. Both operations can be performed an arbitrary number of times but
 * only the first completed execution of a dispatch block object can be waited
 * on with dispatch_block_wait() or observed with dispatch_block_notify().
 *
 * The returned dispatch block will be executed with the specified voucher
 * adopted for the duration of the block body. If the NULL voucher is passed,
 * the block will be executed with the voucher adopted on the calling thread, or
 * with no voucher if the DISPATCH_BLOCK_DETACHED flag was also provided.
 *
 * If invoked directly, the returned dispatch block object will be executed with
 * the assigned QOS class as long as that does not result in a lower QOS class
 * than what is current on the calling thread.
 *
 * If the returned dispatch block object is submitted to a dispatch queue, the
 * QOS class it will be executed with depends on the QOS class assigned to the
 * block, the QOS class assigned to the queue and which of the following flags
 * was specified or defaulted to:
 *  - DISPATCH_BLOCK_INHERIT_QOS_CLASS: default for asynchronous execution
 *  - DISPATCH_BLOCK_ENFORCE_QOS_CLASS: default for synchronous execution
 * See description of dispatch_block_flags_t for details.
 *
 * If the returned dispatch block object is submitted directly to a serial queue
 * and is configured to execute with a specific QOS class, the system will make
 * a best effort to apply the necessary QOS overrides to ensure that blocks
 * submitted earlier to the serial queue are executed at that same QOS class or
 * higher.
 *
 * @param flags
 * Configuration flags for the block object.
 * Passing a value that is not a bitwise OR of flags from dispatch_block_flags_t
 * results in NULL being returned.
 *
 * @param voucher
 * A voucher object or NULL. Passing NULL is equivalent to specifying the
 * DISPATCH_BLOCK_NO_VOUCHER flag.
 *
 * @param qos_class
 * A QOS class value:
 *  - QOS_CLASS_USER_INTERACTIVE
 *  - QOS_CLASS_USER_INITIATED
 *  - QOS_CLASS_DEFAULT
 *  - QOS_CLASS_UTILITY
 *  - QOS_CLASS_BACKGROUND
 *  - QOS_CLASS_UNSPECIFIED
 * Passing QOS_CLASS_UNSPECIFIED is equivalent to specifying the
 * DISPATCH_BLOCK_NO_QOS_CLASS flag. Passing any other value results in NULL
 * being returned.
 *
 * @param relative_priority
 * A relative priority within the QOS class. This value is a negative
 * offset from the maximum supported scheduler priority for the given class.
 * Passing a value greater than zero or less than QOS_MIN_RELATIVE_PRIORITY
 * results in NULL being returned.
 *
 * @param block
 * The block to create the dispatch block object from.
 *
 * @result
 * The newly created dispatch block object, or NULL.
 * When not building with Objective-C ARC, must be released with a -[release]
 * message or the Block_release() function.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_10, __IPHONE_8_0)
DISPATCH_EXPORT DISPATCH_NONNULL5 DISPATCH_RETURNS_RETAINED_BLOCK
DISPATCH_WARN_RESULT DISPATCH_NOTHROW
dispatch_block_t
dispatch_block_create_with_voucher_and_qos_class(dispatch_block_flags_t flags,
		voucher_t _Nullable voucher, dispatch_qos_class_t qos_class,
		int relative_priority, dispatch_block_t block);

/*!
 * @group Voucher dispatch queue SPI
 */

/*!
 * @function dispatch_queue_create_with_accounting_override_voucher
 *
 * @abstract
 * Creates a new dispatch queue with an accounting override voucher created
 * from the specified voucher.
 *
 * @discussion
 * See dispatch_queue_create() headerdoc for generic details on queue creation.
 *
 * The resource accounting attributes of the specified voucher are extracted
 * and used to create an accounting override voucher for the new queue.
 *
 * Every block executed on the returned queue will initially have this override
 * voucher adopted, any voucher automatically associated with or explicitly
 * assigned to the block will NOT be used and released immediately before block
 * execution starts.
 *
 * The accounting override voucher will be automatically propagated to any
 * asynchronous work generated from the queue following standard voucher
 * propagation rules.
 *
 * NOTE: this SPI should only be used in special circumstances when a subsystem
 * has complete control over all workitems submitted to a queue (e.g. no client
 * block is ever submitted to the queue) and if and only if such queues have a
 * one-to-one mapping with resource accounting identities.
 *
 * CAUTION: use of this SPI represents a potential voucher propagation hole. It
 * is the responsibility of the caller to ensure that any callbacks into client
 * code from the queue have the correct client voucher applied (rather than the
 * automatically propagated accounting override voucher), e.g. by use of the
 * dispatch_block_create() API to capture client state at the time the callback
 * is registered.
 *
 * @param label
 * A string label to attach to the queue.
 * This parameter is optional and may be NULL.
 *
 * @param attr
 * DISPATCH_QUEUE_SERIAL, DISPATCH_QUEUE_CONCURRENT, or the result of a call to
 * the function dispatch_queue_attr_make_with_qos_class().
 *
 * @param voucher
 * A voucher whose resource accounting attributes are used to create the
 * accounting override voucher attached to the queue.
 *
 * @result
 * The newly created dispatch queue.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_11,__IPHONE_9_0)
DISPATCH_EXPORT DISPATCH_MALLOC DISPATCH_RETURNS_RETAINED DISPATCH_WARN_RESULT
DISPATCH_NOTHROW
dispatch_queue_t
dispatch_queue_create_with_accounting_override_voucher(
		const char *_Nullable label,
		dispatch_queue_attr_t _Nullable attr,
		voucher_t _Nullable voucher);

/*!
 * @group Voucher Mach SPI
 * SPI intended for clients that need to interact with mach messages or mach
 * voucher ports directly.
 */

/*!
 * @function voucher_create_with_mach_msg
 *
 * @abstract
 * Creates a new voucher object from a mach message carrying a mach voucher port
 *
 * @discussion
 * Ownership of the mach voucher port in the message is transfered to the new
 * voucher object and the message header mach voucher field is cleared.
 *
 * @param msg
 * The mach message to query.
 *
 * @result
 * The newly created voucher object or NULL if the message was not carrying a
 * mach voucher.
 */
__OSX_AVAILABLE_STARTING(__MAC_10_10,__IPHONE_8_0)
OS_VOUCHER_EXPORT OS_OBJECT_RETURNS_RETAINED OS_WARN_RESULT OS_NOTHROW
voucher_t _Nullable
voucher_create_with_mach_msg(mach_msg_header_t *msg);

#ifdef __APPLE__
/*!
 * @group Voucher Persona SPI
 * SPI intended for clients that need to interact with personas.
 */

struct proc_persona_info;

/*!
 * @function voucher_get_current_persona
 *
 * @abstract
 * Retrieve the persona identifier of the 'originator' process for the current
 * voucher.
 *
 * @discussion
 * Retrieve the persona identifier of the ’originator’ process possibly stored
 * in the PERSONA_TOKEN attribute of the currently adopted voucher.
 *
 * If the thread has not adopted a voucher, or the current voucher does not
 * contain a PERSONA_TOKEN attribute, this function returns the persona
 * identifier of the current process.
 *
 * If the process is not running under a persona, then this returns
 * PERSONA_ID_NONE.
 *
 * @result
 * The persona identifier of the 'originator' process for the current voucher,
 * or the persona identifier of the current process
 * or PERSONA_ID_NONE
 */
__OSX_AVAILABLE_STARTING(__MAC_NA,__IPHONE_9_2)
OS_VOUCHER_EXPORT OS_WARN_RESULT OS_NOTHROW
uid_t
voucher_get_current_persona(void);

/*!
 * @function voucher_get_current_persona_originator_info
 *
 * @abstract
 * Retrieve the ’originator’ process persona info for the currently adopted
 * voucher.
 *
 * @discussion
 * If there is no currently adopted voucher, or no PERSONA_TOKEN attribute
 * in that voucher, this function fails.
 *
 * @param persona_info
 * The proc_persona_info structure to fill in case of success
 *
 * @result
 * 0 on success: currently adopted voucher has a PERSONA_TOKEN
 * -1 on failure: persona_info is untouched/uninitialized
 */
__OSX_AVAILABLE_STARTING(__MAC_NA,__IPHONE_9_2)
OS_VOUCHER_EXPORT OS_WARN_RESULT OS_NOTHROW OS_NONNULL1
int
voucher_get_current_persona_originator_info(
	struct proc_persona_info *persona_info);

/*!
 * @function voucher_get_current_persona_proximate_info
 *
 * @abstract
 * Retrieve the ’proximate’ process persona info for the currently adopted
 * voucher.
 *
 * @discussion
 * If there is no currently adopted voucher, or no PERSONA_TOKEN attribute
 * in that voucher, this function fails.
 *
 * @param persona_info
 * The proc_persona_info structure to fill in case of success
 *
 * @result
 * 0 on success: currently adopted voucher has a PERSONA_TOKEN
 * -1 on failure: persona_info is untouched/uninitialized
 */
__OSX_AVAILABLE_STARTING(__MAC_NA,__IPHONE_9_2)
OS_VOUCHER_EXPORT OS_WARN_RESULT OS_NOTHROW OS_NONNULL1
int
voucher_get_current_persona_proximate_info(
	struct proc_persona_info *persona_info);

#endif // __APPLE__

__END_DECLS

DISPATCH_ASSUME_NONNULL_END

#endif // __OS_VOUCHER_PRIVATE__

#if OS_VOUCHER_ACTIVITY_SPI
#include "voucher_activity_private.h"
#endif
