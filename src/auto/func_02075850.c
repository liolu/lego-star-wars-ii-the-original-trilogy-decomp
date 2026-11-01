#include "m2c.h"

/* Growable array of 32-bit elements (same layout as in func_02075698). */
typedef struct {
    u32 *data;    /* element storage */
    u32 size;     /* live elements */
    u32 capacity; /* allocated elements */
} Vec;

/* Temporary replacement storage built while the array grows. */
typedef struct {
    u32 *data;     /* storage set by func_02075b34 */
    u32 size;      /* live elements in the buffer */
    u32 capacity;  /* allocated elements, set by func_02075b34 */
    u32 *cap_ref;  /* &owner->capacity */
    u32 start;     /* index of the first live element inside data[] */
} Buf;

/* One-byte empty object: always zeroed, never read. */
typedef struct {
    u8 b;
} Tag;

enum {
    ELEM_BYTES = 4,
};

u32 func_02075784(Vec *, u32);
void func_02075b34(Buf *, u32, Tag);
void *func_02075b00(Buf *);
void *memcpy(void *, const void *, int);
void *memset(void *, int, int);

/* Takes the address of a zeroed one-byte compound literal so its dead store
 * keeps its own stack slot, like the empty temporaries in the original. */
static inline void tag_use(const Tag *t) {
    (void) t;
} // FAKE: no-op use keeps the dead stack tag stores

static inline void tag_store(void) {
    tag_use(&(Tag) {0}); // FAKE: see tag_use
}

/* Number of live elements, computed as end - begin. */
static inline s32 vsize(Vec *v) {
    u32 *begin = v->data;
    u32 *end   = begin + v->size;
    tag_use(&(Tag) {0}); // FAKE: see tag_use
    return end - begin;
}

/* Slow path of func_02075698: append `count` zeroed elements to v when its
 * capacity is too small.  Steps:
 *  1. func_02075784 picks the new capacity for a shortfall of
 *     size + count - capacity elements;
 *  2. func_02075b34 allocates new_cap elements into the temporary buffer;
 *  3. the new elements are zeroed at buffer index old size, then the old
 *     elements are copied in front of them and the old storage is zeroed;
 *  4. v and the buffer swap capacity, storage and size, and func_02075b00
 *     clears the buffer and frees the old storage (func_02075b7c). */
void func_02075850(Vec *v, u32 count) {
    Buf buf;
    u32 new_cap;
    u32 old_size;
    u32 *old_data;
    s32 old_count;

    buf.data     = 0;
    buf.size     = 0;
    buf.capacity = 0;
    buf.start    = 0;
    buf.cap_ref  = &v->capacity;
    new_cap      = func_02075784(v, v->size + count - v->capacity);
    {
        Tag tag;
        tag.b = 0;
        tag_store();
        func_02075b34(&buf, new_cap, tag);
    }

    /* Zero the new elements after the slots reserved for the old ones. */
    old_size  = v->size;
    buf.start = old_size;
    tag_store();
    memset(buf.data + old_size + buf.size, 0, count * ELEM_BYTES);
    buf.size += count;

    /* Copy the old elements in front of them and zero the old storage. */
    old_data  = v->data;
    old_count = vsize(v);
    buf.start -= old_count;
    memcpy(buf.data + buf.start, old_data, old_count * ELEM_BYTES);
    memset(old_data, 0, old_count * ELEM_BYTES);
    buf.size += old_count;
    tag_store();
    v->size = 0;

    /* Swap the buffer into v; the buffer then owns the old storage. */
    {
        u32 *pc = &buf.capacity; // FAKE: pointer locals keep the swap's address register
        u32 *pv = &v->capacity;
        u32 t   = *pv;
        *pv     = *pc;
        *pc     = t;
    }
    {
        u32 *t   = v->data;
        v->data  = buf.data;
        buf.data = t;
    }
    {
        u32 t    = v->size;
        v->size  = buf.size;
        buf.size = t;
    }
    func_02075b00(&buf);
}
