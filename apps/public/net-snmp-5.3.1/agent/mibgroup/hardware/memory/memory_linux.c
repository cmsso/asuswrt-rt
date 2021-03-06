#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/hardware/memory.h>

#include <unistd.h>
#include <fcntl.h>

/*
 * Try to use an initial size that will cover default cases. We aren't talking
 * about huge files, so why fiddle about with reallocs?
 * I checked /proc/meminfo sizes on 3 different systems: 598, 644, 654
 */
#define MEMINFO_INIT_SIZE   768
#define MEMINFO_STEP_SIZE   256
#define MEMINFO_FILE   "/proc/meminfo"

    /*
     * Load the latest memory usage statistics
     */
int netsnmp_mem_arch_load( netsnmp_cache *cache, void *magic ) {
    int          statfd;
    static char *buff  = NULL;
    static int   bsize = 0;
    static int   first = 1;
    ssize_t      bytes_read;
    char        *b;
    unsigned long memtotal = 0,  memfree = 0, memshared = 0,
                  buffers = 0,   cached = 0,
                  swaptotal = 0, swapfree = 0;

    netsnmp_memory_info *mem;

    if ((statfd = open(MEMINFO_FILE, O_RDONLY, 0)) == -1) {
        snmp_log_perror(MEMINFO_FILE);
        return -1;
    }
    if (bsize == 0) {
        bsize = MEMINFO_INIT_SIZE;
        buff = malloc(bsize);
        if (NULL == buff) {
            snmp_log(LOG_ERR, "malloc failed\n");
            return -1;
        }
    }
    while ((bytes_read = read(statfd, buff, bsize)) == bsize) {
        b = realloc(buff, bsize + MEMINFO_STEP_SIZE);
        if (NULL == b) {
            snmp_log(LOG_ERR, "malloc failed\n");
            return -1;
        }
        buff = b;
        bsize += MEMINFO_STEP_SIZE;
        DEBUGMSGTL(("mem", "/proc/meminfo buffer increased to %d\n", bsize));
        close(statfd);
        statfd = open(MEMINFO_FILE, O_RDONLY, 0);
        if (statfd == -1) {
            snmp_log_perror(MEMINFO_FILE);
            return -1;
        }
    }
    close(statfd);
    if (bytes_read <= 0) {
        snmp_log_perror(MEMINFO_FILE);
    }

        /*
         * Overall Memory statistics
         */

    b = strstr(buff, "MemTotal: ");
    if (b) 
        sscanf(b, "MemTotal: %lu", &memtotal);
    else {
        if (first)
            snmp_log(LOG_ERR, "No MemTotal line in /proc/meminfo\n");
    }
    b = strstr(buff, "MemFree: ");
    if (b) 
        sscanf(b, "MemFree: %lu", &memfree);
    else {
        if (first)
            snmp_log(LOG_ERR, "No MemFree line in /proc/meminfo\n");
    }
    b = strstr(buff, "MemShared: ");
    if (b)
        sscanf(b, "MemShared: %lu", &memshared);
    else {
        if (first)
            if (0 == netsnmp_os_prematch("Linux","2.4"))
                snmp_log(LOG_ERR, "No MemShared line in /proc/meminfo\n");
    }
    b = strstr(buff, "Buffers: ");
    if (b)
        sscanf(b, "Buffers: %lu", &buffers);
    else {
        if (first)
            snmp_log(LOG_ERR, "No Buffers line in /proc/meminfo\n");
    }
    b = strstr(buff, "Cached: ");
    if (b)
        sscanf(b, "Cached: %lu", &cached);
    else {
        if (first)
            snmp_log(LOG_ERR, "No Cached line in /proc/meminfo\n");
    }
    b = strstr(buff, "SwapTotal: ");
    if (b)
        sscanf(b, "SwapTotal: %lu", &swaptotal);
    else {
        if (first)
            snmp_log(LOG_ERR, "No SwapTotal line in /proc/meminfo\n");
    }
    b = strstr(buff, "SwapFree: ");
    if (b)
        sscanf(b, "SwapFree: %lu", &swapfree);
    else {
        if (first)
            snmp_log(LOG_ERR, "No SwapFree line in /proc/meminfo\n");
    }


    mem = netsnmp_memory_get_byIdx( -1, 1 );  /* Memory info */
    if (!mem) {
        snmp_log_perror("No Memory info entry");
    } else {
        mem->units = 1024;
        mem->size = memtotal;
        mem->free = memfree;
        mem->other = memshared;
    }

    mem = netsnmp_memory_get_byIdx( -2, 1 );  /* Swap info */
    if (!mem) {
        snmp_log_perror("No Swap info entry");
    } else {
        mem->units = 1024;
        mem->size = swaptotal;
        mem->free = swapfree;
    }

    mem = netsnmp_memory_get_byIdx( -3, 1 );  /* Buffer info */
    if (!mem) {
        snmp_log_perror("No Buffer info entry");
    } else {
        mem->units = 1024;
        mem->size = buffers;
        mem->other = cached;
    }

    /*
     * XXX - TODO: extract individual memory/swap information
     *    (Into separate netsnmp_memory_info data structures)
     */

    first = 0;
    return 0;
}
