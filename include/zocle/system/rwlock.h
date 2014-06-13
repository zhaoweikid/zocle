#ifndef ZOCLE_SYSTEM_RWLOCK_H
#define ZOCLE_SYSTEM_RWLOCK_H

typedef struct zc_rwlock_t {
	int write;
	int read;
}zcRWLock;

static inline void
zc_rwlock_init(struct zc_rwlock_t *lock) 
{
	lock->write = 0;
	lock->read = 0;
}

static inline void
zc_rwlock_rlock(struct zc_rwlock_t *lock) 
{
	for (;;) {
		while(lock->write) {
			__sync_synchronize();
		}
		__sync_add_and_fetch(&lock->read,1);
		if (lock->write) {
			__sync_sub_and_fetch(&lock->read,1);
		} else {
			break;
		}
	}
}

static inline void
zc_rwlock_wlock(struct zc_rwlock_t *lock) 
{
	while (__sync_lock_test_and_set(&lock->write,1)) {}
	while(lock->read) {
		__sync_synchronize();
	}
}

static inline void
zc_rwlock_wunlock(struct zc_rwlock_t *lock) 
{
	__sync_lock_release(&lock->write);
}

static inline void
zc_rwlock_runlock(struct zc_rwlock_t *lock) 
{
	__sync_sub_and_fetch(&lock->read,1);
}

#endif
