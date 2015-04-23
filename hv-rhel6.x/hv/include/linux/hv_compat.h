
#ifndef _HV_COMPAT_H
#define _HV_COMPAT_H

#include <linux/version.h>

/* LIS 4.0 only includes the below for <=2.6.35 RHEL kernels, but actually
 * upstream Linux 2.6.37 also needs the below.
 */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 37)

#define CN_KVP_IDX	0x9
#define CN_KVP_VAL	0x1

#define CN_VSS_IDX	0xA
#define CN_VSS_VAL	0x1


#ifdef __KERNEL__

#include <linux/rcupdate.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <asm/pgtable_types.h>
#include <net/arp.h>
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_dbg.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_driver.h>
#include <scsi/scsi_eh.h>
#include <scsi/scsi_host.h>


#define CN_KVP_IDX	0x9

#define CN_VSS_IDX	0xA
#define CN_VSS_VAL	0x1

#define HV_DRV_VERSION	"4.0.4"

#if defined(RHEL_RELEASE_VERSION) && (RHEL_RELEASE_CODE <= 1540)
#ifdef CONFIG_MEMORY_HOTPLUG
#undef CONFIG_MEMORY_HOTPLUG
#endif
#endif

#ifndef pr_warn
#define pr_warn(fmt, arg...) printk(KERN_WARNING fmt, ##arg)
#endif

#ifndef HV_STATUS_INSUFFICIENT_BUFFERS
#define HV_STATUS_INSUFFICIENT_BUFFERS	19
#endif

#ifndef RNDIS_STATUS_NETWORK_CHANGE
#define RNDIS_STATUS_NETWORK_CHANGE 0x40010018
#endif

#ifndef NETIF_F_HW_VLAN_CTAG_TX
#define NETIF_F_HW_VLAN_CTAG_TX 0
#endif

#ifndef DID_TARGET_FAILURE
#define DID_TARGET_FAILURE	0x10
#endif


#if defined(RHEL_RELEASE_VERSION) && (RHEL_RELEASE_CODE < 1539)
static inline struct page *skb_frag_page(const skb_frag_t *frag)
{
	return frag->page;
}

static inline unsigned int skb_frag_size(const skb_frag_t *frag)
{
	return frag->size;
}
#endif

#if defined(RHEL_RELEASE_VERSION) && (RHEL_RELEASE_CODE < 1540)
#define hid_err(x, y)
#endif

#define blk_queue_max_segments(a, b)

extern bool using_null_legacy_pic;

/* Upstream 2.6.37 already has vzalloc() */
#if 0
static inline void *vzalloc(unsigned long size)
{
	void *ptr;
	ptr = vmalloc(size);
	memset(ptr, 0, size);
	return ptr;
}
#endif
#if defined(RHEL_RELEASE_VERSION) && (RHEL_RELEASE_CODE < 1538)
#define NETIF_F_RXCSUM 0
#endif

#if defined(RHEL_RELEASE_VERSION) && (RHEL_RELEASE_CODE < 1542)
static inline void
skb_set_hash(struct sk_buff *skb, __u32 hash, int type)
{
#if defined(RHEL_RELEASE_VERSION) && (RHEL_RELEASE_CODE > 1542)
        skb->rxhash = hash;
#endif
}
#endif

bool netvsc_set_hash(u32 *hash, struct sk_buff *skb);
static inline __u32
skb_get_hash(struct sk_buff *skb)
{
#if defined(RHEL_RELEASE_VERSION) && (RHEL_RELEASE_CODE > 1542)
        return skb->hash;
#else
	__u32 hash;
	if (netvsc_set_hash(&hash, skb))
		return hash;
	return 0;
#endif
}

/* Upstream 2.6.37 already has pm_wakeup_event() */
#if 0
static inline void pm_wakeup_event(struct device *dev, unsigned int msec)
{
}
#endif

#if defined(RHEL_RELEASE_VERSION) && (RHEL_RELEASE_CODE < 1540)
static inline int kstrtouint(const char *s, unsigned int base, unsigned int *res)
{
	int result;
	char *endbufp = NULL;

	result = (int)simple_strtoul(s, &endbufp, 10);
	return result;
}

#endif

#define PTE_SHIFT ilog2(PTRS_PER_PTE)

static inline void reinit_completion(struct completion *x)
{
	x->done = 0;
}


static inline int page_level_shift(int level)
{
        return (PAGE_SHIFT - PTE_SHIFT) + level * PTE_SHIFT;
}

static inline unsigned long page_level_size(int level)
{
	return 1UL << page_level_shift(level);
}

static inline unsigned long page_level_mask(int level)
{
	return ~(page_level_size(level) - 1);
}

static inline phys_addr_t slow_virt_to_phys(void *__virt_addr)
{
	unsigned long virt_addr = (unsigned long)__virt_addr;
	phys_addr_t phys_addr;
	unsigned long offset;
	int level;
	unsigned long pmask;
	pte_t *pte;

	pte = lookup_address(virt_addr, &level);
	BUG_ON(!pte);
	pmask = page_level_mask(level);
	offset = virt_addr & ~pmask;
	phys_addr = (phys_addr_t)pte_pfn(*pte) << PAGE_SHIFT;
	return (phys_addr | offset);
}

#if defined(RHEL_RELEASE_VERSION) && (RHEL_RELEASE_CODE < 1540)
/*
 * For Hyper-V devices we use the device guid as the id.
 * This was introduced in Linux 3.2 (/include/linux/mod_devicetable.h)
 */
struct hv_vmbus_device_id {
	__u8 guid[16];
	unsigned long driver_data;
};
#endif


/* Upstream 2.6.37 already has netdev_err()/dbg() and netif_notify_peers() */
#if 0
#ifndef netdev_err
static inline void netdev_err(struct net_device *net, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vprintk(fmt, args);
	va_end(args);
}

#endif

#ifndef netdev_dbg
#if defined(DEBUG)
#define netdev_dbg(dev, fmt, ...)  netdev_err(dev, fmt, ...)
#else
#define netdev_dbg(__dev, format, args...)                      \
({                                                              \
	if (0)                                                  \
		netdev_err(__dev, format, ##args); \
	0;                                                      \
})

#endif
#endif


#if defined(RHEL_RELEASE_VERSION) && (RHEL_RELEASE_CODE == 1536) && \
LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 32)
static inline void  netif_notify_peers(struct net_device *net)
{
	struct in_device *idev;

	rcu_read_lock();
	if (((idev = __in_dev_get_rcu(net)) != NULL) &&
		idev->ifa_list != NULL) {
		arp_send(ARPOP_REQUEST, ETH_P_ARP,
		idev->ifa_list->ifa_address, net,
		idev->ifa_list->ifa_address, NULL,
		net->dev_addr, NULL);
	}
	rcu_read_unlock();
}

#endif
#endif
#endif
#endif
#endif
