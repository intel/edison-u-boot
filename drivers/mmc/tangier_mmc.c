#include <asm/types.h>
#include <linux/types.h>
#include <common.h>
#include <malloc.h>
#include <part.h>
#include <mmc.h>
#include <sdhci.h>
#include <asm/arch/mmc.h>

int tangier_sdhci_init(u32 regbase, int index, int bus_width)
{
	struct sdhci_host *host = NULL;

	host = (struct sdhci_host *)malloc(sizeof(struct sdhci_host));
	if (!host) {
		printf("sdhci__host malloc fail!\n");
		return 1;
	}

	memset(host, 0x00, sizeof(struct sdhci_host));

	host->name = "tangier_sdhci";
	host->ioaddr = (void *)regbase;

	host->quirks = SDHCI_QUIRK_NO_HISPD_BIT | SDHCI_QUIRK_BROKEN_VOLTAGE |
	    SDHCI_QUIRK_BROKEN_R1B | SDHCI_QUIRK_32BIT_DMA_ADDR |
	    SDHCI_QUIRK_WAIT_SEND_CMD;
	host->voltages = MMC_VDD_165_195;	//MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;
	host->version = sdhci_readw(host, SDHCI_HOST_VERSION);

	host->index = index;

	host->host_caps = MMC_MODE_HC;

	add_sdhci(host, 200000000, 400000);

	return 0;
}
