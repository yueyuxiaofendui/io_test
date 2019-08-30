#include <ofs_io.h>

using namespace std;

static inline uint64_t do_seed(uint64_t x, uint64_t m)
{
    return (x < m) ? x + m : x;
}

static inline uint64_t __rand64(struct rand_state *state)
{
	uint64_t xval;

	xval = ((state->s1 <<  1) ^ state->s1) >> 53;
	state->s1 = ((state->s1 & 18446744073709551614ULL) << 10) ^ xval;

	xval = ((state->s2 << 24) ^ state->s2) >> 50;
	state->s2 = ((state->s2 & 18446744073709551104ULL) <<  5) ^ xval;

	xval = ((state->s3 <<  3) ^ state->s3) >> 23;
	state->s3 = ((state->s3 & 18446744073709547520ULL) << 29) ^ xval;

	xval = ((state->s4 <<  5) ^ state->s4) >> 24;
	state->s4 = ((state->s4 & 18446744073709420544ULL) << 23) ^ xval;

	xval = ((state->s5 <<  3) ^ state->s5) >> 33;
	state->s5 = ((state->s5 & 18446744073701163008ULL) <<  8) ^ xval;

	return (state->s1 ^ state->s2 ^ state->s3 ^ state->s4 ^ state->s5);
}

void init_rand_seed(struct rand_state *_state, uint64_t seed)
{
    int cranks = 6;
#define LCG64(x, seed)  ((x) * 6906969069ULL ^ (seed))

    _state->s1 = do_seed(LCG64((2^31) + (2^17) + (2^7), seed), 1); 
	_state->s2 = do_seed(LCG64(_state->s1, seed), 7);
	_state->s3 = do_seed(LCG64(_state->s2, seed), 15);
	_state->s4 = do_seed(LCG64(_state->s3, seed), 33);
	_state->s5 = do_seed(LCG64(_state->s4, seed), 49);

    while (cranks--)
		__rand64(_state);
    return;
}

int rand_rw(unsigned int numratio, struct aio_data *io_u)
{
	int res = 0;
	uint64_t r = __rand64(&io_u->rwmix_state);
	uint64_t v_rand = 100 * (r / (RAND64_MAX + 1.0));
	v_rand += 1;
	if(v_rand <= numratio)
		res = 1;
	return res;
}

uint64_t get_offset(struct aio_data *io_u)
{
    uint64_t _offset;
    uint64_t r;
    uint64_t io_max = io_u->max_blocks;
    r = __rand64(&io_u->random_state);
    _offset = io_max * (r / (RAND64_MAX + 1.0));
	_offset *= 4096;
    return _offset;
}

/**
static inline unsigned int __rand32(struct rand88_state *state)
{
#define TAUSWORTHE(s,a,b,c,d) ((s&c)<<d) ^ (((s <<a) ^ s)>>b)

	state->s1 = TAUSWORTHE(state->s1, 13, 19, 4294967294UL, 12);
	state->s2 = TAUSWORTHE(state->s2, 2, 25, 4294967288UL, 4);
	state->s3 = TAUSWORTHE(state->s3, 3, 11, 4294967280UL, 17);

	return (state->s1 ^ state->s2 ^ state->s3);
}

void init_rand_seed(struct rand88_state *_state, uint64_t seed)
{
    int cranks = 6;

#define LCG(x, seed)  ((x) * 69069 ^ (seed))

	_state->s1 = do_seed(LCG((2^31) + (2^17) + (2^7), seed), 1);
	_state->s2 = do_seed(LCG(_state->s1, seed), 7);
	_state->s3 = do_seed(LCG(_state->s2, seed), 15);

	while (cranks--)
		__rand32(_state);
	return;
}

uint64_t get_offset_me(struct aio_data *io_u)
{
	uint64_t io_max = io_u->max_blocks;
	uint64_t rand_seed = rand() % io_max;
	return rand_seed * 4096;
}
**/