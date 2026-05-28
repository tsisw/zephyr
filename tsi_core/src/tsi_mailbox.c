#include "tsi_mailbox.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sys_io.h>

LOG_MODULE_DECLARE(plat_boot, LOG_LEVEL_INF);

/* SMC programs init.slot_count in tfm_mailbox_hal_init() (ns_agent_mailbox). */
#define TSI_MB_SMC_INIT_TIMEOUT_MS  120000U
#define TSI_MB_SPE_READY_TIMEOUT_MS   60000U

static volatile struct tsi_mb_shm *const mb_shm =
	(volatile struct tsi_mb_shm *)(uintptr_t)TSI_MB_SHM_BASE;

void tsi_mailbox_log_transport(void)
{
	printk("[GMC-MB] transport: data=MAILBOX_SHM @0x%08x "
	       "(pend/replied/slots); ctrl=SCU mailbox10 @0x%08x "
	       "(doorbell, local per QEMU instance)\n",
	       TSI_MB_SHM_BASE, TSI_SCU_MAILBOX10_REG);
	printk("[GMC-MB] transport: doorbell words NS_INIT=0x%08x SPE_READY=0x%08x "
	       "PSA_REQ=0x%08x PSA_REPLY=0x%08x\n",
	       TSI_MB_DOORBELL_NS_INIT, TSI_MB_DOORBELL_SPE_READY,
	       TSI_MB_DOORBELL_PSA_REQ, TSI_MB_DOORBELL_PSA_REPLY);
	printk("[GMC-MB] transport: lab probe sid=0x%08x expect_reply=0x%08x\n",
	       TSI_MB_PSA_PROBE_SID, TSI_MB_PSA_PROBE_REPLY_MAGIC);
	printk("[GMC-MB] transport: platform sid=0x%08x handle=%u ioctl iobuf@0x%08x\n",
	       TSI_MB_PLATFORM_SERVICE_SID, TSI_MB_PLATFORM_STATELESS_HANDLE,
	       TSI_MB_IOBUF_BASE);
}

static void tsi_mailbox_log_enqueue_meta(uint8_t slot_idx, const struct tsi_mb_msg *msg)
{
	uint32_t detail = 0U;

	switch (msg->call_type) {
	case TSI_MB_MAILBOX_PSA_VERSION:
		detail = msg->params.psa_version_params.sid;
		break;
	case TSI_MB_MAILBOX_PSA_CONNECT:
		detail = msg->params.psa_connect_params.sid;
		break;
	case TSI_MB_MAILBOX_PSA_CALL:
		detail = (uint32_t)msg->params.psa_call_params.type;
		break;
	default:
		break;
	}

	printk("[GMC-MB] enqueue: slot=%u psa_type=0x%x client=%d detail=0x%08x "
	       "pend=0x%08x replied=0x%08x shm@0x%08x doorbell10=0x%08x (PSA_REQ=0x%08x)\n",
	       slot_idx, msg->call_type, msg->client_id, detail,
	       mb_shm->status.pend_slots, mb_shm->status.replied_slots,
	       TSI_MB_SHM_BASE, tsi_mailbox_read_doorbell(), TSI_MB_DOORBELL_PSA_REQ);
}

static inline void tsi_mb_doorbell_write(uint32_t value)
{
	sys_write32(value, TSI_SCU_MAILBOX10_REG);
}

uint32_t tsi_mailbox_read_doorbell(void)
{
	return sys_read32(TSI_SCU_MAILBOX10_REG);
}

uint32_t tsi_mailbox_read_pend_mask(void)
{
	return mb_shm->status.pend_slots;
}

uint32_t tsi_mailbox_read_replied_mask(void)
{
	return mb_shm->status.replied_slots;
}

bool tsi_mailbox_spe_ready_seen(void)
{
	return (mb_shm->spe_peer_ready == TSI_MB_SPE_PEER_READY_MAGIC);
}

static bool tsi_mailbox_smc_mailbox_initialized(void)
{
	/*
	 * Require init.status and slot_count from the current SMC reset only.
	 * Stale slot_count in guest_mem (previous run) must not pass this check:
	 * GMC could write ns_peer_ready, then SMC memset clears it, and SMC
	 * later times out (BOOT_STATUS 0xB4) while GMC waits for spe_ready.
	 */
	return (mb_shm->init.slot_count != 0U &&
		(uintptr_t)mb_shm->init.status == (uintptr_t)&mb_shm->status);
}

int tsi_mailbox_boot_handshake(uint32_t timeout_ms)
{
	uint32_t waited_ms = 0U;
	uint32_t spe_wait_ms = 0U;

	(void)timeout_ms;

	/*
	 * GMC may run long before tfm_s: wait for SMC mailbox HAL (init.slot_count).
	 * Doorbell NS_INIT is not reliable on dual-QEMU (mailbox10 is per instance).
	 */
	printk("[GMC-MB] handshake: waiting for SMC mailbox init (timeout=%u ms)\n",
	       TSI_MB_SMC_INIT_TIMEOUT_MS);
	while (waited_ms < TSI_MB_SMC_INIT_TIMEOUT_MS) {
		if (tsi_mailbox_smc_mailbox_initialized()) {
			break;
		}
		if ((waited_ms % 10000U) == 0U && waited_ms > 0U) {
			printk("[GMC-MB] handshake: still waiting SMC init (%u ms) "
			       "slot_count=%u init.status=0x%08x spe_ready=0x%08x\n",
			       waited_ms, mb_shm->init.slot_count,
			       (uint32_t)(uintptr_t)mb_shm->init.status,
			       mb_shm->spe_peer_ready);
		}
		k_msleep(10);
		waited_ms += 10U;
	}

	if (!tsi_mailbox_smc_mailbox_initialized()) {
		const uintptr_t off_slot_count =
			(uintptr_t)&mb_shm->init.slot_count - TSI_MB_SHM_BASE;

		printk("[GMC-MB] handshake: SMC init timeout slot_count=%u "
		       "init.status=0x%08x (SHM @ 0x%08x; guest_mem xxd -s 0x%zx)\n",
		       mb_shm->init.slot_count,
		       (uint32_t)(uintptr_t)mb_shm->init.status,
		       TSI_MB_SHM_BASE, off_slot_count);
		return TSI_MB_TIMEOUT;
	}

	printk("[GMC-MB] handshake: SMC init ok slot_count=%u (waited=%u ms)\n",
	       mb_shm->init.slot_count, waited_ms);

	/*
	 * guest_mem may retain spe_ready from a previous QEMU run while tfm_s is
	 * still booting and not polling SHM. Wait for SMC tsi_mailbox_shm_reset()
	 * to clear spe_ready before we publish ns_peer_ready.
	 */
	if (mb_shm->spe_peer_ready != 0U) {
		printk("[GMC-MB] handshake: stale spe_ready=0x%08x; waiting for SMC reset\n",
		       mb_shm->spe_peer_ready);
		waited_ms = 0U;
		while (waited_ms < TSI_MB_SMC_INIT_TIMEOUT_MS) {
			if (mb_shm->spe_peer_ready == 0U) {
				printk("[GMC-MB] handshake: SMC shm reset seen (spe_ready=0)\n");
				break;
			}
			k_msleep(10);
			waited_ms += 10U;
		}
		if (mb_shm->spe_peer_ready != 0U) {
			printk("[GMC-MB] handshake: spe_ready never cleared (still 0x%08x)\n",
			       mb_shm->spe_peer_ready);
		}
	}

	mb_shm->ns_peer_ready = TSI_MB_NS_PEER_READY_MAGIC;
	barrier_dmem_fence_full();

	printk("[GMC-MB] handshake: waiting for spe_ready (timeout=%u ms)\n",
	       TSI_MB_SPE_READY_TIMEOUT_MS);
	while (spe_wait_ms < TSI_MB_SPE_READY_TIMEOUT_MS) {
		if (tsi_mailbox_spe_ready_seen()) {
			break;
		}
		/*
		 * Re-publish ns_peer_ready: SMC tsi_mailbox_shm_reset() may run after
		 * we first wrote it (stale slot_count race or late tfm_s init).
		 */
		mb_shm->ns_peer_ready = TSI_MB_NS_PEER_READY_MAGIC;
		barrier_dmem_fence_full();
		k_msleep(10);
		spe_wait_ms += 10U;
	}

	if (!tsi_mailbox_spe_ready_seen()) {
		/*
		 * Dual-QEMU: SMC may set spe_ready only after a late poll in
		 * ns_agent_mailbox. If ns_peer_ready is set, continue for lab tests.
		 */
		if (mb_shm->ns_peer_ready == TSI_MB_NS_PEER_READY_MAGIC) {
			printk("[GMC-MB] handshake: spe_ready timeout (0x%08x); "
			       "continuing with ns_ready for dual-QEMU lab\n",
			       mb_shm->spe_peer_ready);
			return TSI_MB_SUCCESS;
		}
		printk("[GMC-MB] handshake: spe_ready timeout spe_ready=0x%08x\n",
		       mb_shm->spe_peer_ready);
		return TSI_MB_TIMEOUT;
	}

	printk("[GMC-MB] handshake: ns_ready=0x%08x spe_ready=0x%08x doorbell=0x%08x (shm)\n",
	       mb_shm->ns_peer_ready, mb_shm->spe_peer_ready,
	       tsi_mailbox_read_doorbell());

	/* Allow SMC ns_agent_mailbox to enter the SHM poll loop before PSA traffic. */
	k_msleep(500);

	return TSI_MB_SUCCESS;
}

int tsi_mailbox_enqueue_psa_version(uint8_t slot_idx, uint32_t sid, int32_t client_id)
{
	struct tsi_mb_msg msg = { 0 };

	msg.call_type = TSI_MB_MAILBOX_PSA_VERSION;
	msg.client_id = client_id;
	msg.params.psa_version_params.sid = sid;

	return tsi_mailbox_enqueue_raw(slot_idx, &msg);
}

int tsi_mailbox_enqueue_psa_connect(uint8_t slot_idx, uint32_t sid, uint32_t version,
				    int32_t client_id)
{
	struct tsi_mb_msg msg = { 0 };

	msg.call_type = TSI_MB_MAILBOX_PSA_CONNECT;
	msg.client_id = client_id;
	msg.params.psa_connect_params.sid = sid;
	msg.params.psa_connect_params.version = version;

	return tsi_mailbox_enqueue_raw(slot_idx, &msg);
}

int tsi_mailbox_enqueue_psa_call(uint8_t slot_idx, int32_t handle, int32_t type,
				 const struct tsi_mb_iovec *in_vec, uint32_t in_len,
				 const struct tsi_mb_iovec *out_vec, uint32_t out_len,
				 int32_t client_id)
{
	struct tsi_mb_msg msg = { 0 };

	if ((in_len > 0U && in_vec == NULL) || (out_len > 0U && out_vec == NULL)) {
		return TSI_MB_INVAL_PARAMS;
	}

	msg.call_type = TSI_MB_MAILBOX_PSA_CALL;
	msg.client_id = client_id;
	msg.params.psa_call_params.handle = handle;
	msg.params.psa_call_params.type = type;
	msg.params.psa_call_params.in_vec = (uint32_t)(uintptr_t)in_vec;
	msg.params.psa_call_params.in_len = in_len;
	msg.params.psa_call_params.out_vec = (uint32_t)(uintptr_t)out_vec;
	msg.params.psa_call_params.out_len = out_len;

	return tsi_mailbox_enqueue_raw(slot_idx, &msg);
}

int tsi_mailbox_enqueue_psa_close(uint8_t slot_idx, int32_t handle, int32_t client_id)
{
	struct tsi_mb_msg msg = { 0 };

	msg.call_type = TSI_MB_MAILBOX_PSA_CLOSE;
	msg.client_id = client_id;
	msg.params.psa_close_params.handle = handle;

	return tsi_mailbox_enqueue_raw(slot_idx, &msg);
}

int tsi_mailbox_transact(uint8_t slot_idx, const struct tsi_mb_msg *msg, int32_t *reply,
			 uint32_t timeout_ms)
{
	int ret;

	ret = tsi_mailbox_enqueue_raw(slot_idx, msg);
	if (ret != TSI_MB_SUCCESS) {
		return ret;
	}

	return tsi_mailbox_wait_reply(slot_idx, reply, timeout_ms);
}

int tsi_mailbox_enqueue_raw(uint8_t slot_idx, const struct tsi_mb_msg *msg)
{
	volatile struct tsi_mb_slot *slot;
	uint32_t slot_bit;

	if (slot_idx >= TSI_MB_SLOT_COUNT || msg == NULL) {
		return TSI_MB_INVAL_PARAMS;
	}

	slot = &mb_shm->slots[slot_idx];
	slot_bit = BIT(slot_idx);

	slot->msg = *msg;
	slot->reply.return_val = 0;

	barrier_dmem_fence_full();
	mb_shm->status.replied_slots &= ~slot_bit;
	mb_shm->status.pend_slots |= slot_bit;
	barrier_dmem_fence_full();

	tsi_mb_doorbell_write(TSI_MB_DOORBELL_PSA_REQ);
	tsi_mailbox_log_enqueue_meta(slot_idx, msg);
	return TSI_MB_SUCCESS;
}

static void tsi_mailbox_iobuf_setup_platform_ioctl(void)
{
	volatile struct tsi_mb_iobuf *io = TSI_MB_IOBUF;

	io->ioctl_request = (int32_t)TSI_MB_PLATFORM_IOCTL_PROBE_REQ;
	io->ioctl_reply = 0U;
	io->in_vec[0].base = (uintptr_t)&io->ioctl_request;
	io->in_vec[0].len = sizeof(io->ioctl_request);
	io->out_vec[0].base = (uintptr_t)&io->ioctl_reply;
	io->out_vec[0].len = sizeof(io->ioctl_reply);
	barrier_dmem_fence_full();
}

int tsi_mailbox_run_psa_lab_sequence(void)
{
	int32_t reply = 0;
	int ret;
	const int32_t client_id = -1;
	const uint8_t slot = 0U;
	const uint32_t timeout_ms = 5000U;

	/*
	 * Re-publish ns_peer_ready and give SMC a few poll iterations to leave
	 * boot-time handshake loops before the first PSA request.
	 */
	mb_shm->ns_peer_ready = TSI_MB_NS_PEER_READY_MAGIC;
	barrier_dmem_fence_full();

	/* 1) SHM transport probe (platform fast-path on SMC). */
	printk("[GMC-MB] psa-lab: [1/4] PSA_VERSION probe sid=0x%08x\n", TSI_MB_PSA_PROBE_SID);
	ret = tsi_mailbox_enqueue_psa_version(slot, TSI_MB_PSA_PROBE_SID, client_id);
	if (ret == TSI_MB_SUCCESS) {
		ret = tsi_mailbox_wait_reply(slot, &reply, timeout_ms);
	}
	if (ret != TSI_MB_SUCCESS || reply != (int32_t)TSI_MB_PSA_PROBE_REPLY_MAGIC) {
		printk("[GMC-MB] psa-lab: probe failed ret=%d reply=0x%08x "
		       "(repackage tfm_s; SMC BOOT_STATUS expect 0xB6)\n",
		       ret, (uint32_t)reply);
		return (ret != TSI_MB_SUCCESS) ? ret : TSI_MB_INVAL_PARAMS;
	}
	printk("[GMC-MB] psa-lab: probe OK reply=0x%08x\n", (uint32_t)reply);

	/* 2) PSA_VERSION on TFM platform service (real SID). */
	printk("[GMC-MB] psa-lab: [2/4] PSA_VERSION platform sid=0x%08x\n",
	       TSI_MB_PLATFORM_SERVICE_SID);
	ret = tsi_mailbox_enqueue_psa_version(slot, TSI_MB_PLATFORM_SERVICE_SID, client_id);
	if (ret == TSI_MB_SUCCESS) {
		ret = tsi_mailbox_wait_reply(slot, &reply, timeout_ms);
	}
	if (ret != TSI_MB_SUCCESS) {
		printk("[GMC-MB] psa-lab: PSA_VERSION failed ret=%d\n", ret);
		return ret;
	}
	if (reply == (int32_t)TSI_MB_PLATFORM_VERSION_EXPECTED) {
		printk("[GMC-MB] psa-lab: PSA_VERSION OK reply=0x%08x (platform SP present)\n",
		       (uint32_t)reply);
	} else if (reply == 0) {
		printk("[GMC-MB] psa-lab: PSA_VERSION reply=0 (PSA_VERSION_NONE; "
		       "platform SP may be absent — continuing)\n");
	} else {
		printk("[GMC-MB] psa-lab: PSA_VERSION reply=0x%08x (expected 0 or 0x%08x)\n",
		       (uint32_t)reply, (uint32_t)TSI_MB_PLATFORM_VERSION_EXPECTED);
	}

	/*
	 * 3) PSA_CONNECT on stateless platform (TF-M: PSA_ERROR_PROGRAMMER_ERROR).
	 */
	printk("[GMC-MB] psa-lab: [3/4] PSA_CONNECT platform sid=0x%08x (stateless -> error)\n",
	       TSI_MB_PLATFORM_SERVICE_SID);
	ret = tsi_mailbox_enqueue_psa_connect(slot, TSI_MB_PLATFORM_SERVICE_SID,
					      TSI_MB_PLATFORM_SERVICE_VERSION, client_id);
	if (ret == TSI_MB_SUCCESS) {
		ret = tsi_mailbox_wait_reply(slot, &reply, timeout_ms);
	}
	if (ret != TSI_MB_SUCCESS) {
		printk("[GMC-MB] psa-lab: PSA_CONNECT transact failed ret=%d\n", ret);
		return ret;
	}
	if (reply != TSI_MB_PSA_ERROR_PROGRAMMER_ERROR &&
	    reply != TSI_MB_PSA_ERROR_CONNECTION_REFUSED) {
		printk("[GMC-MB] psa-lab: PSA_CONNECT unexpected reply=0x%08x "
		       "(expect 0x%08x PROGRAMMER_ERROR or 0x%08x CONNECTION_REFUSED)\n",
		       (uint32_t)reply, (uint32_t)TSI_MB_PSA_ERROR_PROGRAMMER_ERROR,
		       (uint32_t)TSI_MB_PSA_ERROR_CONNECTION_REFUSED);
		return TSI_MB_INVAL_PARAMS;
	}
	printk("[GMC-MB] psa-lab: PSA_CONNECT OK reply=0x%08x (stateless reject)\n",
	       (uint32_t)reply);

	/* 4) PSA_CALL platform IOCTL with in/out vectors in shared IOBUF. */
	{
		volatile struct tsi_mb_iobuf *io = TSI_MB_IOBUF;

		tsi_mailbox_iobuf_setup_platform_ioctl();
		printk("[GMC-MB] psa-lab: [4/4] PSA_CALL handle=%u type=IOCTL(%d) iobuf@0x%08x\n",
		       TSI_MB_PLATFORM_STATELESS_HANDLE, TSI_MB_PLATFORM_API_ID_IOCTL,
		       TSI_MB_IOBUF_BASE);
		ret = tsi_mailbox_enqueue_psa_call(slot, TSI_MB_PLATFORM_STATELESS_HANDLE,
						   TSI_MB_PLATFORM_API_ID_IOCTL,
						   io->in_vec, 1U, io->out_vec, 1U, client_id);
	}
	if (ret == TSI_MB_SUCCESS) {
		ret = tsi_mailbox_wait_reply(slot, &reply, timeout_ms);
	}
	if (ret != TSI_MB_SUCCESS) {
		printk("[GMC-MB] psa-lab: PSA_CALL failed ret=%d\n", ret);
		return ret;
	}
	printk("[GMC-MB] psa-lab: PSA_CALL status=0x%08x ioctl_reply=0x%08x (expect 0x%08x)\n",
	       (uint32_t)reply, TSI_MB_IOBUF->ioctl_reply, TSI_MB_PLATFORM_IOCTL_REPLY_MAGIC);

	if (reply != 0 || TSI_MB_IOBUF->ioctl_reply != TSI_MB_PLATFORM_IOCTL_REPLY_MAGIC) {
		return TSI_MB_INVAL_PARAMS;
	}

	printk("[GMC-MB] psa-lab: sequence complete (SHM pend/replied + PSA VERSION/CONNECT/CALL)\n");
	return TSI_MB_SUCCESS;
}

int tsi_mailbox_wait_reply(uint8_t slot_idx, int32_t *return_val, uint32_t timeout_ms)
{
	uint32_t waited_ms = 0U;
	uint32_t slot_bit;

	if (slot_idx >= TSI_MB_SLOT_COUNT || return_val == NULL) {
		return TSI_MB_INVAL_PARAMS;
	}

	slot_bit = BIT(slot_idx);

	while (waited_ms < timeout_ms) {
		uint32_t replied = mb_shm->status.replied_slots;

		if ((replied & slot_bit) != 0U) {
			*return_val = mb_shm->slots[slot_idx].reply.return_val;
			printk("[GMC-MB] wait: reply slot=%u return_val=0x%08x (%d) waited=%u ms "
			       "pend=0x%08x replied=0x%08x shm@0x%08x doorbell10=0x%08x\n",
			       slot_idx, (uint32_t)*return_val, *return_val, waited_ms,
			       mb_shm->status.pend_slots, mb_shm->status.replied_slots,
			       TSI_MB_SHM_BASE, tsi_mailbox_read_doorbell());
			if (*return_val == (int32_t)TSI_MB_PSA_PROBE_REPLY_MAGIC) {
				printk("[GMC-MB] wait: probe reply magic OK (0x%08x)\n",
				       TSI_MB_PSA_PROBE_REPLY_MAGIC);
			} else if (mb_shm->slots[slot_idx].msg.call_type ==
				   TSI_MB_MAILBOX_PSA_VERSION &&
				   mb_shm->slots[slot_idx].msg.params.psa_version_params.sid ==
					   TSI_MB_PSA_PROBE_SID) {
				printk("[GMC-MB] wait: probe reply MISMATCH expect=0x%08x got=0x%08x\n",
				       TSI_MB_PSA_PROBE_REPLY_MAGIC, (uint32_t)*return_val);
			}
			return TSI_MB_SUCCESS;
		}

		if ((waited_ms % 500U) == 0U) {
			printk("[GMC-MB] wait: pending slot=%u waited=%u ms pend=0x%08x replied=0x%08x doorbell=0x%08x\n",
			       slot_idx, waited_ms,
			       mb_shm->status.pend_slots, mb_shm->status.replied_slots,
			       tsi_mailbox_read_doorbell());
		}

		k_msleep(1);
		waited_ms++;
	}

	printk("[GMC-MB] wait: timeout slot=%u timeout_ms=%u pend=0x%08x replied=0x%08x doorbell=0x%08x\n",
	       slot_idx, timeout_ms,
	       mb_shm->status.pend_slots, mb_shm->status.replied_slots,
	       tsi_mailbox_read_doorbell());
	return TSI_MB_TIMEOUT;
}
