include $(M)/overrides.mk

# The 2 definitons is to cope with the LIS 4.0 code:
# we try to make as less change as possible to the LIS 4.0 code.
ccflags-y  += -DRHEL_RELEASE_VERSION -DRHEL_RELEASE_CODE=0

obj-m	+= hv_vmbus.o
obj-m	+= hv_storvsc.o
obj-m	+= hv_netvsc.o
obj-m	+= hv_utils.o
obj-m	+= hid-hyperv.o
obj-m	+= hyperv_fb.o
obj-m	+= hv_balloon.o
obj-m	+= hyperv-keyboard.o

hv_vmbus-y := vmbus_drv.o \
		 hv.o connection.o channel.o \
		 channel_mgmt.o ring_buffer.o
hv_utils-y := hv_util.o hv_kvp.o hv_snapshot.o hv_fcopy.o

hv_storvsc-y := storvsc_drv.o
hv_netvsc-y := netvsc_drv.o netvsc.o rndis_filter.o
hyperv_keyboard-y := hyperv-keyboard.o
