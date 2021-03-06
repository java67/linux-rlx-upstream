/*
 * Realtek Semiconductor Corp.
 *
 * This file define the irq handler for RLX CPU interrupts.
 *
 * Tony Wu (tonywu@realtek.com.tw)
 * Feb. 28, 2008
 */

#include <linux/irq.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>

#include <irq_vec.h>
#include <rlxregs.h>
//#include <asm/system.h>
//#include <net/rtl/rtl_types.h>
#include <bspchip.h>

static void unmask_rlx_vec_irq(struct irq_data* d)
{
	set_lxc0_estatus(0x10000 << (d->irq - BSP_IRQ_LOPI_BASE));
	irq_enable_hazard();
}

static void mask_rlx_vec_irq(struct irq_data* d)
{
	clear_lxc0_estatus(0x10000 << (d->irq - BSP_IRQ_LOPI_BASE));
	irq_disable_hazard();
}

static struct irq_chip rlx_vec_irq_controller = {
	.name	= "RLX LOPI",
	.irq_ack		= mask_rlx_vec_irq,
	.irq_mask		= mask_rlx_vec_irq,
	.irq_mask_ack	        = mask_rlx_vec_irq,
	.irq_unmask		= unmask_rlx_vec_irq,
	.irq_eoi		= unmask_rlx_vec_irq,
};

//static struct irq_desc *rlx_vec_irq_desc;

void __init rlx_vec_irq_init(int irq_base)
{
	int i;
	extern char rlx_vec_dispatch;

	/* Mask interrupts. */
	clear_lxc0_estatus(EST0_IM);
	clear_lxc0_ecause(ECAUSEF_IP);

	//	rlx_vec_irq_desc = irq_desc + irq_base;

	for (i = irq_base; i < irq_base + BSP_IRQ_LOPI_NUM; i++)
		irq_set_chip_and_handler(i, &rlx_vec_irq_controller, handle_percpu_irq);

	write_lxc0_intvec(&rlx_vec_dispatch);

	#if 1
	/* enable global interrupt mask */
	REG32(BSP_GIMR) = BSP_TC0_IE | BSP_UART0_IE ;

	#ifdef CONFIG_SERIAL_RTL8198_UART1
	REG32(BSP_GIMR)	|= BSP_UART1_IE;
	#endif
	
	#if defined(CONFIG_RTL8192CD)
	#if (defined(CONFIG_RTL_8196C) || defined(CONFIG_RTL_8196CT) || defined(CONFIG_RTL_8196CS) || !defined(CONFIG_RTL_92D_DMDP))
	REG32(BSP_GIMR) |= (BSP_PCIE_IE);
	#endif
	#if defined(CONFIG_RTL_DUAL_PCIESLOT_BIWLAN_D) || (defined(CONFIG_RTL_8198)&&defined(CONFIG_RTL_92D_SUPPORT))
	REG32(BSP_GIMR) |= (BSP_PCIE2_IE);
	#endif
	#endif
	
	#if defined(CONFIG_USB)
	REG32(BSP_GIMR) |= BSP_USB_H_IE;
	#endif
	
	#if defined(CONFIG_RTL_819X) || defined(CONFIG_RTL_ICTEST_SWITCH) || defined(CONFIG_RTL_865X_ETH)
	REG32(BSP_GIMR) |= (BSP_SW_IE);
	#endif
	
	#if defined(CONFIG_RTL_NFBI_MDIO)
	REG32(BSP_GIMR) |= BSP_NFBI_IE;
	#endif

	#ifdef CONFIG_RTL_8198_NFBI_BOARD
	setup_reboot_addr(0x80700000);
	#endif
	
	#if defined(CONFIG_RTK_VOIP)
	REG32(BSP_GIMR) |= (BSP_PCM_IE | BSP_I2S_IE);
	REG32(BSP_GIMR) |= (BSP_GPIO_ABCD_IE | BSP_GPIO_EFGH_IE); 
	#endif
	#endif

}

asmlinkage void rlx_do_lopi_IRQ(int irq_offset)
{
	unsigned int pending;
	unsigned int cause;
	unsigned int status;

	cause = read_lxc0_ecause();
	status = read_lxc0_estatus();
	pending = cause & status & EST0_IM;
	
	if (pending & (_ULCAST_(1) << (irq_offset + 16))) {
		do_IRQ(BSP_IRQ_LOPI_BASE + irq_offset);
	} else {
	#if defined(CONFIG_RTK_VOIP) || defined(CONFIG_RTL_819X)
		spurious_interrupt(SPURIOS_INT_LOPI);
	#else
		spurious_interrupt();
	#endif
	}
}
