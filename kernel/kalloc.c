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

uint64 KALLOCBASE;

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  struct spinlock countslock;
  uint8* counts;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kmem.countslock, "kmem counts");
  KALLOCBASE = (uint64)end + 8*PGSIZE;
  kmem.counts = (uint8*)end;
  freerange((void*)KALLOCBASE, (void*)PHYSTOP);
}

void
kacquirelock()
{
  acquire(&kmem.countslock);
}

void
kreleaselock()
{
  release(&kmem.countslock);
}

uint32
pa2idx(uint64 pa)
{
  return (pa - KALLOCBASE) / PGSIZE;
}

int
kaddpacount(uint64 pa)
{
  uint32 idx = pa2idx(pa);
  if(idx >= KALLOCBASE - (uint64)end)
    panic("kaddpacount");
  if(kmem.counts[idx] == 0xff || kmem.counts[idx] == 0){
    return 0;
  }
  kmem.counts[idx]++;
  return kmem.counts[idx];
}

int
ksubpacount(uint64 pa)
{
  uint32 idx = pa2idx(pa);
  if(idx >= KALLOCBASE - (uint64)end)
    panic("kaddpacount");
  if(kmem.counts[idx] <= 1){
    return -1;
  }
  kmem.counts[idx]--;
  return kmem.counts[idx];
}

uint8
kpacount(uint64 pa)
{
  uint32 idx = pa2idx(pa);
  if(idx >= KALLOCBASE - (uint64)end)
    panic("kaddpacount");
  return kmem.counts[idx];
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    uint32 idx = pa2idx((uint64)p);
    kmem.counts[idx] = 1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (uint64)pa < KALLOCBASE || (uint64)pa >= PHYSTOP)
    panic("kfree");

  uint32 idx = pa2idx((uint64)pa);
  if(idx >= KALLOCBASE - KERNBASE)
    panic("kfree");
  if(!kmem.counts[idx])
    panic("kfree");
  acquire(&kmem.lock);
  kmem.counts[idx]--;
  release(&kmem.lock);
  if(!kmem.counts[idx]){
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    uint32 idx = pa2idx((uint64)r);
    kmem.counts[idx] = 1;
  }
  return (void*)r;
}
