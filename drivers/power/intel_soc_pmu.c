#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <power/intel_soc_pmu.h>

    (struct mrst_pmu_reg *)0xFF11D000;

{
	
	
	
	
	    /* extract the busy bit */ 
	    result.pmu_status_value = temp;
	
		
	
	else if (type == PMU_MODE_ID)
		
	



{
	
	
	    /* wait 500ms that the latest pmu command finished */ 
	    do {
		
			
		
	
	
	



/**
 * This function set all devices in d0i0 and deactivates pmu driver.
 * The function is used before IFWI update as it needs devices to be
 * in d0i0 during IFWI update. Reboot is needed to work pmu
 * driver properly again. After calling this function and IFWI
 * update, system is always rebooted as IFWI update function,
 * intel_scu_ipc_medfw_upgrade() is called from mrst_emergency_reboot().
 */ 
int pmu_set_devices_in_d0i0(void) 
{
	
	
	
	
	
	
	
	    /* Issue the pmu command to PMU 2
	     * flag is needed to distinguish between
	     * S0ix vs interactive command in pmu_sc_irq()
	     */ 
	    status = pmu_issue_interactive_command(&cur_pmssc, 0);
	
		
		
	
	
		
	




{
	
	    /* South complex in Penwell has multiple registers for
	     * PM_SSC, etc.
	     */ 
	    writel(pm_ssc->pmu2_states[0], &pmu_reg->pm_ssc[0]);
	
	
	

{
	
	
	
		
		
	
	
	    /* enable interrupts in PMU2 so that interrupts are
	     * propagated when ioc bit for a particular set
	     * command is set
	     */ 
	    /* Enable the hardware interrupt */ 
	    tmp = readl(&pmu_reg->pm_ics);
	
	
	
	    /* Configure the sub systems for pmu2 */ 
	    pmu_write_subsys_config(pm_ssc);
	
	
	    /* send interactive command to SCU */ 
	    writel(command, &pmu_reg->pm_cmd);
	


