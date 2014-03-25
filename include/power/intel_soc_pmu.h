#ifndef _MID_PMU_H_
#define _MID_PMU_H_

#define PMU2_BUSY_TIMEOUT           500000
    
#define INTERACTIVE_VALUE   0X00002201
#define INTERACTIVE_IOC_VALUE   0X00002301
    
#define D0I0_MASK       0
#define D0I1_MASK       1
#define D0I2_MASK       2
#define D0I3_MASK       3
    
#define PMU_SUCCESS         0
#define PMU_FAILED          -1
#define PMU_BUSY_STATUS         0
#define PMU_MODE_ID         1
enum pmu_ss_state { SS_STATE_D0I0 = 0, SS_STATE_D0I1 =
	    1, SS_STATE_D0I2 = 2, SS_STATE_D0I3 = 3 
};
 struct pmu_ss_states {
	unsigned long pmu1_states;
	unsigned long pmu2_states[4];
};
 union pmu_pm_status {
	struct {
		u32 pmu_rev:8;
		u32 pmu_busy:1;
		u32 mode_id:4;
		u32 Reserved:19;
	} pmu_status_parts;
	u32 pmu_status_value;
};
 struct mrst_pmu_reg {
	u32 pm_sts;		/* 0x00 */
	u32 pm_cmd;		/* 0x04 */
	u32 pm_ics;		/* 0x08 */
	u32 _resv1;
	u32 pm_wkc[2];		/* 0x10 */
	u32 pm_wks[2];		/* 0x18 */
	u32 pm_ssc[4];		/* 0x20 */
	u32 pm_sss[4];		/* 0x30 */
	u32 pm_wssc[4];	/* 0x40 */
	u32 pm_c3c4;		/* 0x50 */
	u32 pm_c5c6;		/* 0x54 */
	u32 pm_msic;		/* 0x58 */
};
 int pmu_set_devices_in_d0i0(void);
int pmu_issue_interactive_command(struct pmu_ss_states *pm_ssc, int ioc);
 
#endif /*  */
