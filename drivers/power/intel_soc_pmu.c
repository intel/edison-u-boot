#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <power/intel_soc_pmu.h>

static struct mrst_pmu_reg *const pmu_reg =
    (struct mrst_pmu_reg *)0xFF11D000;

static int _pmu_read_status(int type)
{
    u32 temp;
    union pmu_pm_status result;

    temp = readl(&pmu_reg->pm_sts);

    /* extract the busy bit */
    result.pmu_status_value = temp;

    if (type == PMU_BUSY_STATUS)
        return result.pmu_status_parts.pmu_busy;
    else if (type == PMU_MODE_ID)
        return result.pmu_status_parts.mode_id;

    return 0;
}

int _pmu2_wait_not_busy(void)
{
    int pmu_busy_retry = PMU2_BUSY_TIMEOUT;

    /* wait 500ms that the latest pmu command finished */
    do {
        if (_pmu_read_status(PMU_BUSY_STATUS) == 0)
            return 0;

        udelay(1);
    } while (--pmu_busy_retry);

    printf("pmu2 busy!\n");

    return -EBUSY;
}

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
    int status;
    struct pmu_ss_states cur_pmssc;

    cur_pmssc.pmu2_states[0] = D0I0_MASK;
    cur_pmssc.pmu2_states[1] = D0I0_MASK;
    cur_pmssc.pmu2_states[2] = D0I0_MASK;
    cur_pmssc.pmu2_states[3] = D0I0_MASK;

    /* Issue the pmu command to PMU 2
     * flag is needed to distinguish between
     * S0ix vs interactive command in pmu_sc_irq()
     */
    status = pmu_issue_interactive_command(&cur_pmssc, 0);

    if (unlikely(status != PMU_SUCCESS)) {  /* pmu command failed */
        printf("Failed to Issue a PM command to PMU2\n");
        goto unlock;
    }

    if (_pmu2_wait_not_busy()) {
        printf("PMU BUSY!!!!\n");
    }

unlock:
    return status;
}

static void pmu_write_subsys_config(struct pmu_ss_states *pm_ssc)
{
    /* South complex in Penwell has multiple registers for
     * PM_SSC, etc.
     */
    writel(pm_ssc->pmu2_states[0], &pmu_reg->pm_ssc[0]);
    writel(pm_ssc->pmu2_states[1], &pmu_reg->pm_ssc[1]);
    writel(pm_ssc->pmu2_states[2], &pmu_reg->pm_ssc[2]);
    writel(pm_ssc->pmu2_states[3], &pmu_reg->pm_ssc[3]);
}

int pmu_issue_interactive_command(struct pmu_ss_states *pm_ssc, int ioc)
{
    u32 tmp;
    u32 command;

    if (_pmu2_wait_not_busy()) {
        printf("SCU BUSY. Operation not permitted\n");
        return PMU_FAILED;
    }

    /* enable interrupts in PMU2 so that interrupts are
     * propagated when ioc bit for a particular set
     * command is set
     */
    /* Enable the hardware interrupt */
    tmp = readl(&pmu_reg->pm_ics);
    tmp |= 0x100;/* Enable interrupts */
    writel(tmp, &pmu_reg->pm_ics);

    /* Configure the sub systems for pmu2 */
    pmu_write_subsys_config(pm_ssc);

    command = (ioc) ? INTERACTIVE_IOC_VALUE : INTERACTIVE_VALUE;

    /* send interactive command to SCU */
    writel(command, &pmu_reg->pm_cmd);

    return 0;
}
