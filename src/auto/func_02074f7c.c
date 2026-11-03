#include "m2c.h"

typedef struct Node Node;
struct Node {
    u8 pad0[0x28];
    s32 key;
    u8 pad2c[8];
    Node *prev;
    Node *next;
};

/* Insert n into the list at *head, kept in ascending key order: n goes in
 * front of the first node whose key is >= n->key, else at the tail. */
void func_02074f7c(Node **head, Node *n) {
    Node *cur = *head;
    if (cur == NULL) {
        n->next = NULL;
        n->prev = NULL;
        *head   = n;
        return;
    }
    while (1) {
        if (n->key <= cur->key) {
            n->next   = cur;
            n->prev   = cur->prev;
            cur->prev = n;
            if (n->prev != NULL) {
                n->prev->next = n;
            } else {
                *head = n;
            }
            return;
        }
        if (cur->next == NULL) {
            n->prev   = cur;
            n->next   = NULL;
            cur->next = n;
            return;
        }
        cur = cur->next;
    }
}
