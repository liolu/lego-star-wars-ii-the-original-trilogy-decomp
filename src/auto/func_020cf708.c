#include "m2c.h"

/* One node of the Huffman work table: entries [0, 1 << bitSize) are leaves (one per
 * symbol value), entries above them are the internal nodes built here. */
typedef struct {
    u32 freq;          /* symbol count (leaf) or sum of both children (internal node) */
    u16 index;         /* own table index */
    s16 parent;        /* parent node index, 0 while the node is still unmerged */
    s16 child[2];      /* left and right child indices */
    u16 codeLen;       /* code length in bits, +1 per merge above it (func_020d0438) */
    u16 subtreeDepth;  /* 0 for a leaf, nonzero for an internal node (callees recurse on it) */
    u32 code;          /* Huffman code bits, filled by func_020d04c0 */
    u8 bit;            /* branch bit from the parent: 0 left, 1 right */
    u8 pad15;
    u16 field16;       /* not used here */
} HuffData;

/* State shared with the tree helpers; the three pointers split the caller's work area. */
typedef struct {
    u8 treeTop;        /* encoded tree size, in 2-byte units minus one (header byte) */
    u8 pad1;
    u16 tableSize;     /* number of leaf entries, 1 << bitSize */
    void *tree;        /* encoded tree bytes, built by HuffMakeHuffTree */
    void *treeCtrl;    /* helper scratch for HuffMakeHuffTree */
    HuffData *table;   /* node table, 2 * tableSize entries */
} HuffWork;

extern HuffWork HuffTreeTop;
extern const HuffData data_020fca40; /* initial value of every table entry */

extern void func_020d0438(s32 arg0, s32 arg1);
extern void func_020d04c0(u16 arg0, u16 arg1);
extern u8 func_020d0534(u16 arg0);
extern void HuffMakeHuffTree(u16 rootNo);

enum {
    WORK_TREE_OFFSET      = 0x3000,      /* 512 HuffData entries (24 bytes each) */
    WORK_TREE_CTRL_OFFSET = 0x3200, /* after 0x200 bytes of encoded tree */
    HUFF_HEADER_TYPE      = 0x20,        /* compression type nibble: Huffman */
    HUFF_HEADER_SIZE      = 4,
};

/* Huffman-compress `size` bytes of src into dst using `bitSize`-bit symbols (8, or 4 for
 * nibbles, low nibble first) and the caller's work area (table, tree, helper scratch).
 * Output: a 32-bit header (size << 8 | 0x20 | bitSize), the encoded tree padded to a word,
 * then the code bit stream, most significant bit first, padded to a word. Each stream
 * word is byte-reversed at the end, so its first code bit is the word's top bit when read
 * as a little-endian u32. Returns the compressed size in bytes, or 0 when the output would
 * reach `size` (no gain); the only caller (func_020733b4) passes 4-bit symbols. */
s32 func_020cf708(u8 *src, u32 size, u8 *dst, u8 bitSize, u8 *work) {
    u32 dstCount;
    u8 tmp;
    u16 nodeCount;
    s32 left, right;
    s32 i;
    s32 j, k;
    u32 bits;
    u32 bitCount;

    /* Split the work area and reset every node. */
    HuffTreeTop.table     = (HuffData *) work;
    HuffTreeTop.tree      = work + WORK_TREE_OFFSET;
    HuffTreeTop.treeCtrl  = work + WORK_TREE_CTRL_OFFSET;
    HuffTreeTop.tableSize = (u16) (1 << bitSize);

    bits      = 0;
    bitCount  = 0;
    nodeCount = HuffTreeTop.tableSize;
    for (i = 0; i < (u16) (HuffTreeTop.tableSize * 2); i++) {
        HuffTreeTop.table[i]       = data_020fca40;
        HuffTreeTop.table[i].index = (u16) i;
    }

    /* Count symbol frequencies. */
    if (bitSize == 8) {
        for (i = 0; i < size; i++) {
            HuffTreeTop.table[src[i]].freq++;
        }
    } else {
        for (i = 0; i < size; i++) {
            u8 hi = (u8) ((src[i] & 0xF0) >> 4);
            u8 lo = (u8) (src[i] & 0x0F);

            HuffTreeTop.table[hi].freq++;
            HuffTreeTop.table[lo].freq++;
        }
    }

    /* Repeatedly merge the two unmerged nodes with the lowest frequencies (left, then
     * right) into a new node, until only one unmerged node, the root, is left. */
    left = right = -1;
    while (1) {
        for (i = 0; i < nodeCount; i++) {
            if (HuffTreeTop.table[i].freq != 0 && HuffTreeTop.table[i].parent == 0) {
                left = i;
                break;
            }
        }
        for (i = (u16) left; i < nodeCount; i++) {
            if (HuffTreeTop.table[i].freq == 0 || HuffTreeTop.table[i].parent != 0) {
                continue;
            }
            if (HuffTreeTop.table[i].freq < HuffTreeTop.table[left].freq) {
                left = i;
            }
        }
        for (i = 0; i < nodeCount; i++) {
            if (HuffTreeTop.table[i].freq == 0 || HuffTreeTop.table[i].parent != 0) {
                continue;
            }
            if (i != left) {
                right = i;
                break;
            }
        }
        for (i = (u16) right; i < nodeCount; i++) {
            if (HuffTreeTop.table[i].freq == 0 || HuffTreeTop.table[i].parent != 0) {
                continue;
            }
            if (HuffTreeTop.table[i].freq < HuffTreeTop.table[right].freq) {
                if (i != left) {
                    right = i;
                }
            }
        }

        if (right < 0) {
            /* Single symbol value: give the root that leaf as both children. Otherwise
             * the last node made is the root. */
            if (nodeCount == HuffTreeTop.tableSize) {
                HuffTreeTop.table[nodeCount].freq         = HuffTreeTop.table[left].freq;
                HuffTreeTop.table[nodeCount].child[0]     = (s16) left;
                HuffTreeTop.table[nodeCount].child[1]     = (s16) left;
                HuffTreeTop.table[nodeCount].subtreeDepth = 1;
                HuffTreeTop.table[left].parent            = (s16) nodeCount;
                HuffTreeTop.table[left].bit               = 0;
                HuffTreeTop.table[left].codeLen           = 1;
            } else {
                nodeCount--;
            }
            break;
        }

        HuffTreeTop.table[nodeCount].freq     = HuffTreeTop.table[left].freq + HuffTreeTop.table[right].freq;
        HuffTreeTop.table[nodeCount].child[0] = (s16) left;
        HuffTreeTop.table[nodeCount].child[1] = (s16) right;
        if (HuffTreeTop.table[left].subtreeDepth > HuffTreeTop.table[right].subtreeDepth) {
            HuffTreeTop.table[nodeCount].subtreeDepth = (u16) (HuffTreeTop.table[left].subtreeDepth + 1);
        } else {
            HuffTreeTop.table[nodeCount].subtreeDepth = (u16) (HuffTreeTop.table[right].subtreeDepth + 1);
        }
        HuffTreeTop.table[left].parent = HuffTreeTop.table[right].parent = (s16) nodeCount;
        HuffTreeTop.table[left].bit                                      = 0;
        HuffTreeTop.table[right].bit                                     = 1;
        func_020d0438((u16) left, (u16) right);
        nodeCount++;
        left = right = -1;
    }

    /* Assign codes from the root, then encode the tree. */
    func_020d04c0(nodeCount, 0);
    func_020d0534(nodeCount);
    HuffMakeHuffTree(nodeCount);
    ((u8 *) HuffTreeTop.tree)[0] = --HuffTreeTop.treeTop;

    *(u32 *) dst = (size << 8) | HUFF_HEADER_TYPE | bitSize;
    dstCount     = HUFF_HEADER_SIZE;

    /* Copy the tree after the header; while padding it to a word, each odd pad byte
     * also bumps the tree-size byte. */
    if (dstCount + (HuffTreeTop.treeTop + 1) * 2 >= size) {
        return 0;
    }
    for (i = 0; i < (u16) ((HuffTreeTop.treeTop + 1) * 2); i++) {
        dst[dstCount++] = ((u8 *) HuffTreeTop.tree)[i];
    }
    while (dstCount & 3) {
        if (dstCount & 1) {
            HuffTreeTop.treeTop++;
            dst[4]++;
        }
        dst[dstCount++] = 0;
    }

    /* Emit each symbol's code, flushing whole bytes as they fill. */
    for (i = 0; i < size; i++) {
        if (bitSize == 8) {
            bits = (bits << HuffTreeTop.table[src[i]].codeLen) | HuffTreeTop.table[src[i]].code;
            bitCount += HuffTreeTop.table[src[i]].codeLen;
            if (dstCount + (bitCount / 8) >= size) {
                return 0;
            }
            for (k = 0; k < bitCount / 8; k++) {
                dst[dstCount++] = (u8) (bits >> (bitCount - (k + 1) * 8));
            }
            bitCount %= 8;
        } else {
            for (k = 0; k < 2; k++) {
                if (k != 0) {
                    tmp = (u8) (src[i] >> 4);
                } else {
                    tmp = (u8) (src[i] & 0x0F);
                }
                bits = (bits << HuffTreeTop.table[tmp].codeLen) | HuffTreeTop.table[tmp].code;
                bitCount += HuffTreeTop.table[tmp].codeLen;
                if (dstCount + (bitCount / 8) >= size) {
                    return 0;
                }
                for (j = 0; j < bitCount / 8; j++) {
                    dst[dstCount++] = (u8) (bits >> (bitCount - (j + 1) * 8));
                }
                bitCount %= 8;
            }
        }
    }

    /* Flush the last partial byte, then pad the stream to a word. */
    if (bitCount != 0) {
        if (dstCount + 1 >= size) {
            return 0;
        }
        dst[dstCount++] = (u8) (bits << (8 - bitCount));
    }
    while (dstCount & 3) {
        dst[dstCount++] = 0;
    }

    /* Byte-reverse each stream word (the tree words are left as they are). */
    for (i = 1 + (HuffTreeTop.treeTop + 1) * 2 / 4; i < (dstCount / 4) + 1; i++) {
        tmp            = dst[i * 4 + 0];
        dst[i * 4 + 0] = dst[i * 4 + 3];
        dst[i * 4 + 3] = tmp;
        tmp            = dst[i * 4 + 1];
        dst[i * 4 + 1] = dst[i * 4 + 2];
        dst[i * 4 + 2] = tmp;
    }
    return dstCount;
}
