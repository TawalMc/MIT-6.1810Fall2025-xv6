// #define LAB_LOCK 1

// Mutual exclusion lock.
struct spinlock {
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
#ifdef LAB_LOCK
  int nts;
  int n;
#endif
};


#ifdef LAB_LOCK
// Reader-writer lock.
struct rwspinlock {
  // Replace this with your implementation.
  uint8 readers; // count readers holding the lock
  uint8 writer; // 0|1 indicating if a writer holds the locks
  uint8 pending_writers;

  struct spinlock l;
};
#endif
