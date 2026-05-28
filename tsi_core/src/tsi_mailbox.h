#ifndef TSI_CORE_MAILBOX_H_
#define TSI_CORE_MAILBOX_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* TF-M mailbox ABI constants used by SMC side. */
#define TSI_MB_MAILBOX_PSA_FRAMEWORK_VERSION 0x1u
#define TSI_MB_MAILBOX_PSA_VERSION           0x2u
#define TSI_MB_MAILBOX_PSA_CONNECT           0x3u
#define TSI_MB_MAILBOX_PSA_CALL              0x4u
#define TSI_MB_MAILBOX_PSA_CLOSE             0x5u

#define TSI_MB_SUCCESS      0
#define TSI_MB_INVAL_PARAMS (-2147483646) /* INT32_MIN + 2 */
#define TSI_MB_TIMEOUT      (-110)

/*
 * Must match SMC TF-M build (`NUM_MAILBOX_QUEUE_SLOT`).
 */
#ifndef TSI_MB_SLOT_COUNT
#define TSI_MB_SLOT_COUNT 4u
#endif

/*
 * Match TF-M when MAILBOX_IS_UNCACHED_S/NS == 1: MAILBOX_ALIGN is empty in
 * tfm_mailbox.h (no 32-byte padding). Do not add __aligned__(32) here.
 */
#define TSI_MB_ALIGN

/* SCU mailbox registers and doorbell words. */
#define TSI_SCU_MAILBOX10_REG 0x800001F8u

#define TSI_MB_DOORBELL_NS_INIT   0x000000AEu
#define TSI_MB_DOORBELL_SPE_READY 0x000000C3u
#define TSI_MB_DOORBELL_PSA_REQ   0xA5CF50C6u
#define TSI_MB_DOORBELL_PSA_REPLY 0xC605FC5Au

/* Lab probe SID / reply magic (must match SMC tsi_mailbox.h). */
#define TSI_MB_PSA_PROBE_SID           0x474D4302u /* 'GMC\x02' */
#define TSI_MB_PSA_PROBE_REPLY_MAGIC   0x50454101u /* 'PEA\x01' */

/* TFM platform partition (manifest stateless_handle 6 -> encoded PSA handle). */
#define TSI_MB_PLATFORM_SERVICE_SID        0x00000040u
#define TSI_MB_PLATFORM_SERVICE_VERSION    1u
#define TSI_MB_PLATFORM_STATELESS_HANDLE   0x40000105u
#define TSI_MB_PLATFORM_API_ID_IOCTL       1013

#define TSI_MB_PLATFORM_IOCTL_PROBE_REQ    0x54534901u /* 'TSI\x01' */
#define TSI_MB_PLATFORM_IOCTL_REPLY_MAGIC  0x504C4101u /* 'PLA\x01' */
/* TF-M tfm_spm_client_psa_version() returns service manifest version (see tfm_platform.yaml). */
#define TSI_MB_PLATFORM_VERSION_EXPECTED     TSI_MB_PLATFORM_SERVICE_VERSION
/* psa/error.h — stateless platform CONNECT lab (TF-M returns PROGRAMMER_ERROR) */
#define TSI_MB_PSA_ERROR_PROGRAMMER_ERROR      ((int32_t)-129)
#define TSI_MB_PSA_ERROR_CONNECTION_REFUSED    ((int32_t)-130)

/*
 * IO buffers for PSA_CALL in/out vectors (both M85 map this NS SRAM in guest_mem).
 * Must sit in GMC RAM and below MAILBOX_SHM.
 */
#define TSI_MB_IOBUF_BASE 0x601FE000u

/* Shared mailbox window in NS SRAM (must match TF-M `TSI_MAILBOX_SHM_BASE`). */
#ifndef TSI_MB_SHM_BASE
#define TSI_MB_SHM_BASE 0x601FF000u
#endif

#define TSI_MB_NS_PEER_READY_MAGIC  0x474D4301u /* 'GMC\x01' */
#define TSI_MB_SPE_PEER_READY_MAGIC 0x53504501u /* 'SPE\x01' */

/* GDB / xxd helpers (packed layout, same as struct tsi_mailbox_shm_layout_t). */
#define TSI_MB_SHM_OFF_INIT_STATUS   0x98u
#define TSI_MB_SHM_OFF_SLOT_COUNT    0xa0u
#define TSI_MB_SHM_OFF_NS_PEER_READY 0xb0u
#define TSI_MB_SHM_OFF_SPE_PEER_READY 0xb4u

/* Same as struct psa_client_params_t in tfm_mailbox.h (32-bit). */
struct tsi_mb_psa_client_params {
	union {
		struct {
			uint32_t sid;
		} psa_version_params;
		struct {
			uint32_t sid;
			uint32_t version;
		} psa_connect_params;
		struct {
			int32_t handle;
			int32_t type;
			uint32_t in_vec;
			uint32_t in_len;
			uint32_t out_vec;
			uint32_t out_len;
		} psa_call_params;
		struct {
			int32_t handle;
		} psa_close_params;
	};
};

struct tsi_mb_msg {
	uint32_t call_type;
	struct tsi_mb_psa_client_params params;
	int32_t client_id;
} TSI_MB_ALIGN;

struct tsi_mb_reply {
	int32_t return_val;
} TSI_MB_ALIGN;

struct tsi_mb_slot {
	struct tsi_mb_msg msg;
	struct tsi_mb_reply reply;
} TSI_MB_ALIGN;

struct tsi_mb_status {
	uint32_t pend_slots;
	uint32_t replied_slots;
} TSI_MB_ALIGN;

struct tsi_mb_init {
	struct tsi_mb_status *status;
	uint32_t slot_count;
	struct tsi_mb_slot *slots;
};

/*
 * Must match struct tsi_mailbox_shm_layout_t in tsi_mailbox_shm.h (TF-M side).
 */
struct tsi_mb_shm {
	struct tsi_mb_status status;
	struct tsi_mb_slot slots[TSI_MB_SLOT_COUNT];
	struct tsi_mb_init init;
	uint32_t ns_peer_ready;
	uint32_t spe_peer_ready;
};

/* PSA in/out vector layout for mailbox PSA_CALL (uintptr_t + len). */
struct tsi_mb_iovec {
	uintptr_t base;
	uint32_t len;
};

struct tsi_mb_iobuf {
	int32_t ioctl_request;
	uint32_t ioctl_reply;
	struct tsi_mb_iovec in_vec[1];
	struct tsi_mb_iovec out_vec[1];
};

#define TSI_MB_IOBUF ((volatile struct tsi_mb_iobuf *)(uintptr_t)TSI_MB_IOBUF_BASE)

void tsi_mailbox_log_transport(void);

int tsi_mailbox_boot_handshake(uint32_t timeout_ms);
int tsi_mailbox_enqueue_raw(uint8_t slot_idx, const struct tsi_mb_msg *msg);
int tsi_mailbox_enqueue_psa_version(uint8_t slot_idx, uint32_t sid, int32_t client_id);
int tsi_mailbox_enqueue_psa_connect(uint8_t slot_idx, uint32_t sid, uint32_t version,
				    int32_t client_id);
int tsi_mailbox_enqueue_psa_call(uint8_t slot_idx, int32_t handle, int32_t type,
				 const struct tsi_mb_iovec *in_vec, uint32_t in_len,
				 const struct tsi_mb_iovec *out_vec, uint32_t out_len,
				 int32_t client_id);
int tsi_mailbox_enqueue_psa_close(uint8_t slot_idx, int32_t handle, int32_t client_id);
int tsi_mailbox_wait_reply(uint8_t slot_idx, int32_t *return_val, uint32_t timeout_ms);
int tsi_mailbox_transact(uint8_t slot_idx, const struct tsi_mb_msg *msg, int32_t *reply,
			 uint32_t timeout_ms);
int tsi_mailbox_run_psa_lab_sequence(void);

uint32_t tsi_mailbox_read_doorbell(void);
uint32_t tsi_mailbox_read_pend_mask(void);
uint32_t tsi_mailbox_read_replied_mask(void);
bool tsi_mailbox_spe_ready_seen(void);

#endif
