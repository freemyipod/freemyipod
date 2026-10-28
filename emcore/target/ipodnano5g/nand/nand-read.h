/* Copyright (C) 2009 Michael Sparmann
 * Copyright (C) 2026 Viktor Krapivin
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Shared S5L87xx controller implementation. Caller serializes all requests. */
/* ==== Page read ==== */
/* Read the pages in nand_rows/nand_cemasks/nand_dmalist. Single-page reads
 * return per-sector status through secstat and may return NAND_ERASED. */
/* data is used for cache maintenance; DMA addresses are already in the list.
 * On fault or timeout, reset and re-identify the chips. */
static int fmss_read(unsigned int n, void *data, uint8_t *secstat)
{
    const unsigned int page_bytes = nand_geom.page_size;
    uint32_t result;
    int rc;

    FMSS_WR(P_ROWS, NAND_DMA_ADDR(nand_rows));
    FMSS_WR(P_CEMASKS, NAND_DMA_ADDR(nand_cemasks));
    FMSS_WR(P_COUNT, n);
    FMSS_WR(P_META, NAND_DMA_ADDR(nand_meta));
    FMSS_WR(P_DMALIST, NAND_DMA_ADDR(nand_dmalist));
    FMSS_WR(P_STATS, NAND_DMA_ADDR(nand_hist));
    FMSS_WR(P_ECCBUF, secstat ? NAND_DMA_ADDR(secstat) : 0);  /* 0 = no status bytes */
    FMSS_WR(P_CHUNKS, page_bytes / 2048);
    FMSS_WR(P_TIMING, nand_timing);
    FMSS_WR(P_ECCMODE, nand_ecc_mode);
    FMSS_WR(P_ECCCFG, nand_ecc_cfg);
    FMSS_WR(P_HDRLEN, nand_ecc_hdrlen);
    FMSS_WR(P_PARLEN, nand_ecc_parlen);
    FMSS_WR(FMSS_PROG, NAND_DMA_ADDR(fmss_prog_read));

    invalidate_dcache_range((uint32_t)data, n * page_bytes);
    invalidate_dcache_range((uint32_t)nand_meta, n * NAND_META_SIZE);
    if (secstat)
        invalidate_dcache_range((uint32_t)secstat, (nand_geom.page_size / 512));
    invalidate_dcache_range((uint32_t)nand_hist, sizeof(nand_hist));
    clean_dcache_range((uint32_t)nand_rows, n * sizeof(uint32_t));
    clean_dcache_range((uint32_t)nand_cemasks, n * sizeof(uint32_t));
    clean_dcache_range((uint32_t)nand_dmalist,
                        n * (page_bytes / 2048) * sizeof(uint32_t));

    nand_pages_read += n;
    rc = fmss_start();
    if (!rc)
        rc = fmss_wait();
    if (rc)
    {
        nand_reset_chips();
        return NAND_ERR_TIMEOUT;
    }

    /* nothing may have pulled the DMA targets into the cache meanwhile */
    invalidate_dcache_range((uint32_t)data, n * page_bytes);
    invalidate_dcache_range((uint32_t)nand_meta, n * NAND_META_SIZE);
    if (secstat)
        invalidate_dcache_range((uint32_t)secstat, (nand_geom.page_size / 512));

    /* Result word (sequencer r6): bit 30 = some sector uncorrectable,
     * bit 29 = some sector erased, bits 0..15 = corrected-bit counts seen */
    result = FMSS_RD(FMSS_RESULT);
    if (result & FMSS_RESULT_UECC)
    {
        /* An erased page whose blank sectors picked up a few flipped bits
         * decodes as erased + up to two uncorrectable sectors */
        if (secstat)
        {
            unsigned int i, blank = 0, bad = 0;
            for (i = 0; i < (nand_geom.page_size / 512); i++)
            {
                if (secstat[i] == 0xfe)  /* erased sector */
                    blank++;
                else if (secstat[i] == 0xff)  /* uncorrectable sector */
                    bad++;
            }
            if (bad < 3 && blank + bad == (nand_geom.page_size / 512))
                return NAND_ERASED;
        }
        return NAND_ERR_ECC;
    }
    /* NAND_ERASED requires every sector erased and none corrected. */
    if (secstat && (result & FMSS_RESULT_ERASED)
        && !(result & FMSS_RESULT_NCORR))
        return NAND_ERASED;
    return NAND_OK;
}

/* Request list entry i: page `page` (row in the chip) on CE `bank`, data to
 * data + i * page size as 2 KiB DMA chunks */
static void nand_set_request(unsigned int i, unsigned int bank, uint32_t page,
                             uint8_t *data)
{
    const unsigned int page_bytes = nand_geom.page_size;
    unsigned int c;

    nand_cemasks[i] = 1u << nand_geom.ce[bank];
    nand_rows[i] = page;
    for (c = 0; c < page_bytes / 2048; c++)
        nand_dmalist[i * (page_bytes / 2048) + c] =
            NAND_DMA_ADDR(data + i * page_bytes + c * 2048);
}

/* Read one page with sector status. Erased data and metadata become 0xff.
 * buf must be cache-line aligned. */
static int nand_read_page_once(unsigned int bank, uint32_t page, uint8_t *buf,
                               void *meta)
{
    int rc;

    nand_set_request(0, bank, page, buf);
    rc = fmss_read(1, buf, nand_secstat);
    if (rc == NAND_ERASED)
    {
        memset(buf, 0xff, nand_geom.page_size);
        memset(nand_meta, 0xff, NAND_META_SIZE);
    }
    if (meta)
        memcpy(meta, nand_meta, NAND_META_SIZE);
    return rc;
}

/* Retry once after a timeout or ECC failure. fmss_read already resets the
 * chips on timeout; ECC failures need an explicit reset here. */
int nand_read_page(unsigned int bank, uint32_t page, void *data, void *meta)
{
    uint8_t *buf = data;
    int rc;

    if (!nand_phys_ready || bank >= nand_geom.banks)
        return NAND_ERR_NOCHIP;
    if ((uintptr_t)buf & (CACHEALIGN_SIZE - 1))
        buf = nand_bounce;

    rc = nand_read_page_once(bank, page, buf, meta);
    if (rc < 0)
    {
        if (rc == NAND_ERR_ECC && nand_reset_chips())
            return NAND_ERR_TIMEOUT;
        rc = nand_read_page_once(bank, page, buf, meta);
    }
    if (buf != data)
        memcpy(data, buf, nand_geom.page_size);
    return rc;
}

/* Read multiple pages without retries. The FTL retries failures page by page. */
int nand_read_pages(unsigned int n, const uint8_t *bank, const uint32_t *page,
                    void *data, void *meta)
{
    unsigned int i;
    int rc;

    if (!nand_phys_ready)
        return NAND_ERR_NOCHIP;
    if (n == 0 || n > NAND_MAX_PAGES_PER_READ
        || ((uintptr_t)data & (CACHEALIGN_SIZE - 1)))
        return NAND_ERR_UNSUPPORTED;
    for (i = 0; i < n; i++)
    {
        if (bank[i] >= nand_geom.banks)
            return NAND_ERR_NOCHIP;
        nand_set_request(i, bank[i], page[i], data);
    }
    rc = fmss_read(n, data, NULL);
    if (rc == NAND_OK && meta)
        memcpy(meta, nand_meta, n * NAND_META_SIZE);
    return rc;
}
