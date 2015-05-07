obj-y					+= prm_common.o cm_common.o
obj-$(CONFIG_ARCH_OMAP2)		+= cm2xxx.o prm2xxx.o prm2xxx_3xxx.o
obj-$(CONFIG_ARCH_OMAP3)		+= prm2xxx_3xxx.o prm3xxx.o
am33xx-43xx-prcm-common			+= prm33xx.o
obj-$(CONFIG_SOC_AM33XX)		+= $(am33xx-43xx-prcm-common)
obj-$(CONFIG_SOC_AM43XX)		+= $(am33xx-43xx-prcm-common)
obj-$(CONFIG_SOC_TI81XX)		+= $(am33xx-43xx-prcm-common)
omap4-5-prcm-common			+= prcm_mpu44xx.o prm44xx.o \
					   prminst44xx.o
obj-$(CONFIG_ARCH_OMAP4)		+= $(omap4-5-prcm-common)
obj-$(CONFIG_SOC_OMAP5)			+= $(omap4-5-prcm-common)
obj-$(CONFIG_SOC_DRA7)			+= $(omap4-5-prcm-common)
obj-$(CONFIG_SOC_AM43XX)		+= $(omap4-5-prcm-common)
