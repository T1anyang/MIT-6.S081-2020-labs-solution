// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 7

struct {
  // struct spinlock lock;
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf heads[NBUCKET];
  struct buf buckets[NBUCKET][NBUF];
  struct spinlock bucketlocks[NBUCKET]; // lock per bucket
} bcache;

void
binit(void)
{
  struct buf *b;

  // initlock(&bcache.lock, "bcache");

  // Create linked list of all buckets
  for (int i = 0; i < NBUCKET; i++) {
    initlock(&bcache.bucketlocks[i], "bcache");

    // Create linked list of buffers
    bcache.heads[i].prev = &bcache.heads[i];
    bcache.heads[i].next = &bcache.heads[i];
    for(b = bcache.buckets[i]; b < bcache.buckets[i]+NBUF; b++){
      b->next = bcache.heads[i].next;
      b->prev = &bcache.heads[i];
      initsleeplock(&b->lock, "buffer");
      bcache.heads[i].next->prev = b;
      bcache.heads[i].next = b;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint i = blockno % NBUCKET;
  acquire(&bcache.bucketlocks[i]);

  // Is the block already cached?
  for(b = bcache.heads[i].next; b != &bcache.heads[i]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucketlocks[i]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.heads[i].prev; b != &bcache.heads[i]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.bucketlocks[i]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint i = b->blockno % NBUCKET;
  acquire(&bcache.bucketlocks[i]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.heads[i].next;
    b->prev = &bcache.heads[i];
    bcache.heads[i].next->prev = b;
    bcache.heads[i].next = b;
  }
  
  release(&bcache.bucketlocks[i]);
}

void
bpin(struct buf *b) {
  uint i = b->blockno % NBUCKET;
  acquire(&bcache.bucketlocks[i]);
  b->refcnt++;
  release(&bcache.bucketlocks[i]);
}

void
bunpin(struct buf *b) {
  uint i = b->blockno % NBUCKET;
  acquire(&bcache.bucketlocks[i]);
  b->refcnt--;
  release(&bcache.bucketlocks[i]);
}


