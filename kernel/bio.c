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

#define NBUCKET 13
#define HASH(dev,blockno) ((((dev)<<27)|(blockno))%NBUCKET)

struct {
  //struct spinlock lock;
  struct buf buf[NBUF];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct spinlock lock[NBUCKET];
  struct buf hashbuf[NBUCKET];
  struct spinlock eviction_lock;
} bcache;

void
binit(void)
{
  struct buf *b;
  for(int i=0;i<NBUCKET;i++){
  	initlock(&bcache.lock[i],"bcache_lock");
	//bcache.head[i].prev=&bcache.head[i];
	bcache.hashbuf[i].next=0;
	//bcache.head[i].next=&bcache.head[i];
  }
  initlock(&bcache.eviction_lock,"bcache_eviction");
  //initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  //bcache.head.prev = &bcache.head;
  //bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.hashbuf[0].next;
    bcache.hashbuf[0].next=b;
    initsleeplock(&b->lock, "buffer");
    b->refcnt=0;
    b->timestamp=0;
    //bcache.head[0].next->prev = b;
    //bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b=0;
  int bucket=HASH(dev,blockno);
  acquire(&bcache.lock[bucket]);

  // Is the block already cached?
  for(b = bcache.hashbuf[bucket].next; b; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      /*acquire(&tickslock);
      b->timestamp=ticks;
      release(&tickslock);*/
      release(&bcache.lock[bucket]);
      //printf("cpu%d: acquire lock %p at line %d\n", cpuid(), &bcache.lock[bucket], __LINE__);
      acquiresleep(&b->lock);
      return b;
    }
  }
  
  int hold=-1;
  release(&bcache.lock[bucket]);
  acquire(&bcache.eviction_lock);
  for(b=bcache.hashbuf[bucket].next;b;b=b->next){
  	if(b->dev==dev&&b->blockno==blockno){
		acquire(&bcache.lock[bucket]);
		b->refcnt++;
		/*acquire(&tickslock);
		b->timestamp=ticks;
		release(&tickslock);*/
		release(&bcache.lock[bucket]);
		release(&bcache.eviction_lock);
		acquiresleep(&b->lock);
		return b;
	}
  }
  
  struct buf* before_least=0;
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(int i=0;i<NBUCKET;i++){
	int update=0;
	acquire(&bcache.lock[i]);
	for(b=&bcache.hashbuf[i];b->next;b=b->next){
		if(b->next->refcnt==0&&(!before_least||b->next->timestamp<before_least->next->timestamp)){
			before_least=b;
			update=1;
		}
	}
	if(update==0) release(&bcache.lock[i]);
	else{
		if(hold!=-1){
			release(&bcache.lock[hold]);
			//printf("cpu%d: acquire lock %p at line %d\n", cpuid(), &bcache.lock[hold], __LINE__);
		}
		hold=i;
	}
  }
  if(!before_least) panic("bget:no buffers");
  b=before_least->next;
     if(hold!=bucket){
//     printf("bget: hold=%d, bucket=%d, b=%p\n", hold, bucket, b);	     
         before_least->next=b->next;
	 release(&bcache.lock[hold]);
					     
	 acquire(&bcache.lock[bucket]);
         b->next = bcache.hashbuf[bucket].next;
	 bcache.hashbuf[bucket].next = b;
     }
      
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      /*acquire(&tickslock);
      b->timestamp=ticks;
      release(&tickslock);*/
      release(&bcache.lock[bucket]);
      release(&bcache.eviction_lock);
      acquiresleep(&b->lock);
      return b;
  

  //panic("bget: no buffers");
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
  int bucket=HASH(b->dev,b->blockno);
  acquire(&bcache.lock[bucket]);
  b->refcnt--;
  if(b->refcnt==0){
//  acquire(&tickslock);
  b->timestamp=ticks;
//  release(&tickslock);
  }
  release(&bcache.lock[bucket]);
  
}

void
bpin(struct buf *b) {
  int bucket=HASH(b->dev,b->blockno);
  acquire(&bcache.lock[bucket]);
  b->refcnt++;
  release(&bcache.lock[bucket]);
}

void
bunpin(struct buf *b) {
  int bucket=HASH(b->dev,b->blockno);
  acquire(&bcache.lock[bucket]);
  b->refcnt--;
  release(&bcache.lock[bucket]);
}


