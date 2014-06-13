#ifndef ZOCLE_SYSTEM_LOCKER_H
#define ZOCLE_SYSTEM_LOCKER_H

static inline void
zc_locker_init(int lock) 
{
	lock = 0;
}


static inline void
zc_locker_lock(int lock) 
{
	while (__sync_lock_test_and_set(&lock, 1)) {}
}

static inline void
zc_locker_unlock(int lock) 
{
	__sync_lock_release(&lock);
}

#endif
