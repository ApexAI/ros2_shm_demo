# For Shared Memory demonstration
#
# Note that we cannot use unbounded strings like 
# string data
# for shared memory transfer
#
# string[256] data
# is possible but the generated type is more cumbersome to use
#
# A generic char array allows passing arbitrary data.
#
char[256] data
uint8 size
uint64 counter

uint8 MAX_SIZE=255