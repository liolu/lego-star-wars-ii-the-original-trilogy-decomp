#include "m2c.h"

/* One-byte empty object (allocator-like tag): always zeroed, passed by value
 * to the storage helper, never read. */
typedef struct Tag {
    u8 b;
} Tag;

/* Growable array of 32-bit elements (also used by func_02075064/func_02075710). */
typedef struct Vec {
    s32 *begin; /* element storage */
    s32 size;   /* live elements */
    s32 cap;    /* allocated elements */
} Vec;

/* Temporary replacement storage built while the array grows. */
typedef struct Buf {
    s32 *begin;    /* storage set by func_02075b34 */
    s32 size;      /* live elements in the buffer */
    s32 cap;       /* allocated elements, set by func_02075b34 */
    s32 *allocref; /* &owner->cap (owner's capacity/allocator slot) */
    s32 pos;       /* index of the first live element inside begin[] */
} Buf;

enum {
    ELEM_BYTES = 4,
};

u32 func_02075784(Vec *v, u32 n, s32 *val);
void func_02075b34(Buf *b, u32 n, Tag a);
void *func_02075b00(Buf *b);
void *memcpy(void *, const void *, int);
void *memset(void *, int, int);

/* Takes the address of a zeroed one-byte compound literal so its (dead)
 * store keeps its own stack slot, like the empty temporaries in the original. */
static inline void tag_use(const Tag *t) {
    (void) t;
} // FAKE: no-op use keeps the dead stack tag stores

/* Element distance end - start. */
static inline s32 ptr_dist(const s32 *start, const s32 *end) {
    return end - start;
}

/* Slow path of appending `count` copies of *value to v (caller func_02075710
 * passes count 1 when the array is full).  Steps:
 *  1. func_02075784 picks the new capacity for a shortfall of
 *     size + count - cap elements: cap + max(shortfall, 3 * (cap + 1) / 5),
 *     or (cap + 1) / 2 for larger arrays, 0x3fffffff at most (overflow is
 *     reported through func_02075b1c);
 *  2. func_02075b34 allocates new_cap elements (new_cap * 4 bytes through
 *     func_0205e824, aborts on failure) into the temporary buffer;
 *  3. the copies are written at buffer index old size, then the old elements
 *     are copied in front of them and the old storage is zeroed;
 *  4. v and the buffer swap storage, size and capacity, and func_02075b00
 *     clears the buffer and hands the old storage to func_0205e84c. */
void func_0207599c(Vec *v, u32 count, s32 *value) {
    Buf b;
    s32 *dst;
    s32 *old_begin;
    s32 old_size, old_bytes, front, left;
    s32 tmp;
    s32 *cp;
    u32 new_cap;

    b.begin    = 0;
    b.size     = 0;
    b.cap      = 0;
    b.pos      = 0;
    b.allocref = &v->cap;
    new_cap    = func_02075784(v, v->size + count - v->cap, value);
    {
        Tag t;
        t.b = 0;
        tag_use(&(Tag) {0}); // FAKE: see tag_use
        func_02075b34(&b, new_cap, t);
    }

    /* Fill the new copies after the slots reserved for the old elements. */
    b.pos = v->size;
    tag_use(&(Tag) {0}); // FAKE: see tag_use
    dst = b.begin + b.pos + b.size;
    for (left = count; left != 0; left--) {
        *dst++ = *value;
    }
    b.size += count;

    /* Move the old elements in front of the copies and zero the old storage. */
    old_begin = v->begin;
    old_size  = ptr_dist(old_begin, old_begin + v->size);
    tag_use(&(Tag) {0}); // FAKE: see tag_use
    old_bytes = old_size * ELEM_BYTES;
    front     = b.pos - old_size;
    b.pos     = front;
    memcpy(b.begin + front, old_begin, old_bytes);
    memset(old_begin, 0, old_bytes);
    b.size += old_size;
    tag_use(&(Tag) {0}); // FAKE: see tag_use
    v->size = 0;

    /* Swap the buffer into v; the buffer then owns the old storage. */
    cp       = &b.cap; // FAKE: pointer local forces address-register swap
    tmp      = v->cap;
    v->cap   = *cp;
    *cp      = tmp;
    tmp      = (s32) v->begin;
    v->begin = b.begin;
    b.begin  = (s32 *) tmp;
    tmp      = v->size;
    v->size  = b.size;
    b.size   = tmp;
    func_02075b00(&b);
}
