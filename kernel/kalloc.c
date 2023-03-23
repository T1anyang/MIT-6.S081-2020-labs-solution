// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  unsigned int freenum;
} kmem[NCPU];

void
kinit()
{
  for(int i = 0; i < NCPU; i++){
    initlock(&kmem[i].lock, "kmem");
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  push_off();
  int cpu = cpuid();
  pop_off();
  acquire(&kmem[cpu].lock);
  r->next = kmem[cpu].freelist;
  kmem[cpu].freelist = r;
  kmem[cpu].freenum++;
  release(&kmem[cpu].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int cpu = cpuid();
  pop_off();
  acquire(&kmem[cpu].lock);
  r = kmem[cpu].freelist;
  if(r){
    kmem[cpu].freelist = r->next;
    kmem[cpu].freenum--;
  }
  release(&kmem[cpu].lock);

  int target = (cpu + 7) % NCPU;
  while(!r){
    if(target == cpu){
      break;
    }
    acquire(&kmem[target].lock);
    int stealnum = kmem[target].freenum > 2048? kmem[target].freenum/2 : kmem[target].freenum > 1024? 1024: kmem[target].freenum;
    if(stealnum){
      struct run* tmp = kmem[target].freelist;
      struct run* prev = 0;
      for(int i = 0; i < stealnum; i++){
        if(!tmp){
          panic("kalloc");
        }
        prev = tmp;
        tmp = tmp->next;
      }
      r = kmem[target].freelist;
      kmem[target].freelist = tmp;
      kmem[target].freenum -= stealnum;
      release(&kmem[target].lock);
      acquire(&kmem[cpu].lock);
      prev->next = kmem[cpu].freelist;
      kmem[cpu].freelist = r->next;
      kmem[cpu].freenum += stealnum - 1;
      release(&kmem[cpu].lock);
    }
    else{
      release(&kmem[target].lock);
    }
    target--;
    if(target < 0){
      target += NCPU;
    }
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
