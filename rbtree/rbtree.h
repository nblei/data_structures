// eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx

/**
 * A red-black tree is a binary search tree with the following color
 * properties:
 *
 * 1) Every node is colored either red or black
 * 2) The root is black
 * 3) If a node is red, its children must be black
 * 4) Every path from a node to a NULL pointer must contain the same
 *      number of black nodes
 *
 * The height of a red-black tree is at most 2*log(N + 1)
 **/

/**
 * Violating Cases: (lower case letters represent black nodes, upper case RED)
 *
 * 0) "Root Red"
 *      Resolution: Turn the root black
 *
 * 1) "Uncle and parent are Red"
 *                      g
 *                    P   Y
 *                  I
 *     Resolution:  Toggle colors of parent, uncle, grandparent
 *                      G
 *                    p   y
 *                 I
 *     
 * 2) "Uncle is black, child is 'inner'"
 *                       g
 *                   P       y
 *                     I
 *     Resolution:  Rotate to case 3, then apply case 3's resolution
 *                       g
 *                   I       y
 *               P 
 *                       
 * 3) "Uncle is black, child is 'outer'"
 *                      g               |               g
 *                 P         y          |       y               P
 *             I                        |                               I
 *     Resolution: Rotate (grandparent)
 *                      p               |                       p
 *              I               G       |               G               I
 *                                   y  |       y
 **/

/**
 * Insertion Rules:
 * 
 * 1) Every inserted node is Red
 * 2) Insertion follows the rules of a binary search tree
 **/

#ifndef _NBLEI_RBTREE_H_
#define _NBLEI_RBTREE_H_
#include <stdint.h>

typedef void * RBTREE;

struct rbtreeinfo {
        int (*keycopy)(void * dest, void * src);
        int (*keycomp)(void * p, void * q);
        int (*keyfree)(void * p);
};

RBTREE * rb_init(struct rbtreeinfo * info);
int rb_free(RBTREE * tree);
int rb_assert(RBTREE * tree);
int rb_insert(RBTREE * tree, void * key, void * data);
int rb_size(RBTREE * tree);
int rb_has(RBTREE * tree, void * key);


int rb_remove(RBTREE * tree, void * key);

#endif

