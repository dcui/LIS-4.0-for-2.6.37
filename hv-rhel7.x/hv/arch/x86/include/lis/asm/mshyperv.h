#ifndef _ASM_X86_MSHYPER_H
#define _ASM_X86_MSHYPER_H

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/clocksource.h>
#include <linux/nmi.h>
#include <asm/io.h>
#include <lis/asm/hyperv.h>

/*
 * The below CPUID leaves are present if VersionAndFeatures.HypervisorPresent
 * is set by CPUID(HVCPUID_VERSION_FEATURES).
 */
enum hv_cpuid_function {
	HVCPUID_VERSION_FEATURES		= 0x00000001,
	HVCPUID_VENDOR_MAXFUNCTION		= 0x40000000,
	HVCPUID_INTERFACE			= 0x40000001,

	/*
	 * The remaining functions depend on the value of
	 * HVCPUID_INTERFACE
	 */
	HVCPUID_VERSION				= 0x40000002,
	HVCPUID_FEATURES			= 0x40000003,
	HVCPUID_ENLIGHTENMENT_INFO		= 0x40000004,
	HVCPUID_IMPLEMENTATION_LIMITS		= 0x40000005,
};

struct ms_hyperv_info {
	u32 features;
	u32 misc_features;
	u32 hints;
};

extern struct ms_hyperv_info ms_hyperv;

/*
 *  * Declare the MSR used to setup pages used to communicate with the hypervisor.
 *   */
#define HV_X64_MSR_HYPERCALL	0x40000001

union hv_x64_msr_hypercall_contents {
	u64 as_uint64;
	struct {
		u64 enable:1;
		u64 reserved:11;
		u64 guest_physical_address:52;
	};
};

/*
 * TSC page layout.
 */

struct ms_hyperv_tsc_page {
	volatile u32 tsc_sequence;
	u32 reserved1;
	volatile u64 tsc_scale;
	volatile s64 tsc_offset;
	u64 reserved2[509];
};

/*
 * The guest OS needs to register the guest ID with the hypervisor.
 * The guest ID is a 64 bit entity and the structure of this ID is
 * specified in the Hyper-V specification:
 *
 * http://msdn.microsoft.com/en-us/library/windows/hardware/ff542653%28v=vs.85%29.aspx
 *
 * While the current guideline does not specify how Linux guest ID(s)
 * need to be generated, our plan is to publish the guidelines for
 * Linux and other guest operating systems that currently are hosted
 * on Hyper-V. The implementation here conforms to this yet
 * unpublished guidelines.
 *
 *
 * Bit(s)
 * 63 - Indicates if the OS is Open Source or not; 1 is Open Source
 * 62:56 - Os Type; Linux is 0x100
 * 55:48 - Distro specific identification
 * 47:16 - Linux kernel version number
 * 15:0  - Distro specific identification
 *
 * Distro specific identification are defined as:
 *   SuSE SLE      0x10
 *   RHEL          0x20
 *   CentOS        0x21
 *   Oracle (RHCK) 0x22
 *   Oracle (UEK)  0x40
 *   Debian        0x60
 *   Ubuntu        0x80
 */

#define HV_LINUX_VENDOR_ID		0x8100

/*
 * Generate the guest ID based on the guideline described above.
 */

static inline  __u64 generate_guest_id(__u8 d_info1, __u32 kernel_version,
					__u16 d_info2)
{
	__u64 guest_id = 0;

	guest_id = (((__u64)HV_LINUX_VENDOR_ID) << 48);
	guest_id |= (((__u64)(d_info1)) << 48);
	guest_id |= (((__u64)(kernel_version)) << 16);
	guest_id |= ((__u64)(d_info2));

	return guest_id;
}

/* Free the message slot and signal end-of-message if required */
static inline void vmbus_signal_eom(struct hv_message *msg, u32 old_msg_type)
{
	/*
	 * On crash we're reading some other CPU's message page and we need
	 * to be careful: this other CPU may already had cleared the header
	 * and the host may already had delivered some other message there.
	 * In case we blindly write msg->header.message_type we're going
	 * to lose it. We can still lose a message of the same type but
	 * we count on the fact that there can only be one
	 * CHANNELMSG_UNLOAD_RESPONSE and we don't care about other messages
	 * on crash.
	 */
	if (cmpxchg(&msg->header.message_type, old_msg_type,
		    HVMSG_NONE) != old_msg_type)
		return;

	/*
	 * Make sure the write to MessageType (ie set to
	 * HVMSG_NONE) happens before we read the
	 * MessagePending and EOMing. Otherwise, the EOMing
	 * will not deliver any more messages since there is
	 * no empty slot
	 */
	mb();

	if (msg->header.message_flags.msg_pending) {
		/*
		 * This will cause message queue rescan to
		 * possibly deliver another msg from the
		 * hypervisor
		 */
		wrmsrl(HV_X64_MSR_EOM, 0);
	}
}

#define hv_init_timer(timer, tick) wrmsrl(timer, tick)
#define hv_init_timer_config(config, val) wrmsrl(config, val)

void hyperv_callback_vector(void);
#ifdef CONFIG_TRACING
#define trace_hyperv_callback_vector hyperv_callback_vector
#endif
void hyperv_vector_handler(struct pt_regs *regs);
void hv_setup_vmbus_irq(void (*handler)(void));
void hv_remove_vmbus_irq(void);

void hv_setup_kexec_handler(void (*handler)(void));
void hv_remove_kexec_handler(void);
void hv_setup_crash_handler(void (*handler)(struct pt_regs *regs));
void hv_remove_crash_handler(void);

#if IS_ENABLED(CONFIG_HYPERV)
extern struct clocksource *hyperv_cs;
extern void *hv_hypercall_pg;

static inline u64 hv_do_hypercall(u64 control, void *input, void *output)
{
	u64 input_address = input ? virt_to_phys(input) : 0;
	u64 output_address = output ? virt_to_phys(output) : 0;
	u64 hv_status;
	register void *__sp asm(_ASM_SP);

#ifdef CONFIG_X86_64
	if (!hv_hypercall_pg)
		return U64_MAX;

	__asm__ __volatile__("mov %4, %%r8\n"
			     "call *%5"
			     : "=a" (hv_status), "+r" (__sp),
			       "+c" (control), "+d" (input_address)
			     :  "r" (output_address), "m" (hv_hypercall_pg)
			     : "cc", "memory", "r8", "r9", "r10", "r11");
#else
	u32 input_address_hi = upper_32_bits(input_address);
	u32 input_address_lo = lower_32_bits(input_address);
	u32 output_address_hi = upper_32_bits(output_address);
	u32 output_address_lo = lower_32_bits(output_address);

	if (!hv_hypercall_pg)
		return U64_MAX;

	__asm__ __volatile__("call *%7"
			     : "=A" (hv_status),
			       "+c" (input_address_lo), "+r" (__sp)
			     : "A" (control),
			       "b" (input_address_hi),
			       "D"(output_address_hi), "S"(output_address_lo),
			       "m" (hv_hypercall_pg)
			     : "cc", "memory");
#endif /* !x86_64 */
	return hv_status;
}

#define HV_HYPERCALL_RESULT_MASK	GENMASK_ULL(15, 0)
#define HV_HYPERCALL_FAST_BIT		BIT(16)
#define HV_HYPERCALL_VARHEAD_OFFSET	17
#define HV_HYPERCALL_REP_COMP_OFFSET	32
#define HV_HYPERCALL_REP_COMP_MASK	GENMASK_ULL(43, 32)
#define HV_HYPERCALL_REP_START_OFFSET	48
#define HV_HYPERCALL_REP_START_MASK	GENMASK_ULL(59, 48)

/* Fast hypercall with 8 bytes of input and no output */
static inline u64 hv_do_fast_hypercall8(u16 code, u64 input1)
{
	u64 hv_status, control = (u64)code | HV_HYPERCALL_FAST_BIT;
	register void *__sp asm(_ASM_SP);

#ifdef CONFIG_X86_64
	{
		__asm__ __volatile__("call *%4"
				     : "=a" (hv_status), "+r" (__sp),
				       "+c" (control), "+d" (input1)
				     : "m" (hv_hypercall_pg)
				     : "cc", "r8", "r9", "r10", "r11");
	}
#else
	{
		u32 input1_hi = upper_32_bits(input1);
		u32 input1_lo = lower_32_bits(input1);

		__asm__ __volatile__ ("call *%5"
				      : "=A"(hv_status),
					"+c"(input1_lo),
					"+r"(__sp)
				      :	"A" (control),
					"b" (input1_hi),
					"m" (hv_hypercall_pg)
				      : "cc", "edi", "esi");
	}
#endif
		return hv_status;
}

/*
 * Rep hypercalls. Callers of this functions are supposed to ensure that
 * rep_count and varhead_size comply with Hyper-V hypercall definition.
 */
static inline u64 hv_do_rep_hypercall(u16 code, u16 rep_count, u16 varhead_size,
				      void *input, void *output)
{
	u64 control = code;
	u64 status;
	u16 rep_comp;

	control |= (u64)varhead_size << HV_HYPERCALL_VARHEAD_OFFSET;
	control |= (u64)rep_count << HV_HYPERCALL_REP_COMP_OFFSET;

	do {
		status = hv_do_hypercall(control, input, output);
		if ((status & HV_HYPERCALL_RESULT_MASK) != HV_STATUS_SUCCESS)
			return status;

		/* Bits 32-43 of status have 'Reps completed' data. */
		rep_comp = (status & HV_HYPERCALL_REP_COMP_MASK) >>
			HV_HYPERCALL_REP_COMP_OFFSET;

		control &= ~HV_HYPERCALL_REP_START_MASK;
		control |= (u64)rep_comp << HV_HYPERCALL_REP_START_OFFSET;

		touch_nmi_watchdog();
	} while (rep_comp < rep_count);

	return status;
}

void hyperv_init(void);
void hyperv_report_panic(struct pt_regs *regs, long err);
bool hv_is_hypercall_page_setup(void);
void hyperv_cleanup(void);
void hv_print_host_info(void);
#endif

#define hv_get_synint_state(int_num, val) rdmsrl(int_num, val)
#define hv_set_synint_state(int_num, val) wrmsrl(int_num, val)

#define hv_get_simp(val) rdmsrl(HV_X64_MSR_SIMP, val)
#define hv_set_simp(val) wrmsrl(HV_X64_MSR_SIMP, val)

#define hv_get_siefp(val) rdmsrl(HV_X64_MSR_SIEFP, val)
#define hv_set_siefp(val) wrmsrl(HV_X64_MSR_SIEFP, val)
#define hv_get_synic_state(val) rdmsrl(HV_X64_MSR_SCONTROL, val)
#define hv_set_synic_state(val) wrmsrl(HV_X64_MSR_SCONTROL, val)

#define hv_get_vp_index(index) rdmsrl(HV_X64_MSR_VP_INDEX, index)
void hv_register_vmbus_handler(int irq, irq_handler_t handler);
#endif
