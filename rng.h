#ifndef __RNG_H__
#define __RNG_H__

int rng_initialized(void);
void init_genrand64(unsigned long long seed);
unsigned long long genrand64_int64(void);

#endif
