// eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx
#include "rbtree.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define RBT_RED   1
#define RBT_BLACK 2
#define RBT_LEFT  0
#define RBT_RIGHT 1
#define _RB_TREE_VALID 0x158df3
// NULL nodes are black
#define is_red(node) ( ((node) != NULL) && ((node)->color == RBT_RED) )

struct rb_tree {
        struct rbtreeinfo info;
        struct rb_node * root;
        uint32_t valid;
};

typedef enum { ROT_LEFT , ROT_RIGHT } rotation_t;

struct rb_node {
        uint8_t  color;
        void * key; void *   data;
        struct rb_node * link[2];
};

RBTREE * rb_init(struct rbtreeinfo * info)
{
        struct rb_tree * rv = (struct rb_tree *)malloc(sizeof * rv);
        if (rv) {
                rv->root = NULL;
                rv->valid = _RB_TREE_VALID;
                memcpy(&(rv->info), info, sizeof *info);
        }
        return (RBTREE *)rv;
}


struct rb_node * rotation(struct rb_node * root, int direction)
{
        // Root is really grandparent of the problem node
        struct rb_node * parent   = root->link[direction ^ 1];
        root->link[direction ^ 1] = parent->link[direction];
        parent->link[direction]   = root;

        // Structre is fixed, now change colors:
        root->color   = RBT_RED;
        parent->color = RBT_BLACK;

        // Parent is the new root of the subtree
        return parent;
}

struct rb_node * double_rotation(struct rb_node * root, int dir)
{
        root->link[dir ^ 1] = rotation(root->link[dir ^ 1], dir ^ 1);
        return rotation(root, dir);
}

int _rb_assert(struct rb_node * root)
{
        if (root == NULL) return 1;

        int lh, rh;
        struct rb_node * ln = root->link[0];
        struct rb_node * rn = root->link[1];

        // Check for red children of red node
        if (is_red(root))
                if (is_red(ln) || is_red(rn)) {
                        fprintf(stderr, "Red Node Violation\n");
                        return 0;
                }

        lh = _rb_assert(ln);
        rh = _rb_assert(rn);

        // Check for BST violations
        // TODO: Use comparator
        if ((ln && ln->key >= root->key) || (rn && rn->key <= root->key)) {
                fprintf(stderr, "BST Violation");
                return 0;
        }

        // Check blackheight violation
        if (lh != 0 && rh != 0 && lh != rh) {
                fprintf(stderr, "Black Height Violation");
                return 0;
        }

        if (lh != 0 && rh != 0)
                return is_red(root) ? lh : lh + 1;
        else
                return 0;
}

int rb_assert(RBTREE * tree)
{
        return _rb_assert( ((struct rb_tree *)tree)->root);
}

struct rb_node * rb_make_node(struct rb_tree * tree, void * key, void * data)
{
        (void)data; //TODO
        struct rb_node * rv = (struct rb_node *)calloc(1, sizeof *rv);
        if (rv == NULL)
                return NULL;

        rv->color = RBT_RED;
        tree->info.keycopy(&(rv->key), &key);
        return rv;
}

struct rb_node * rb_insert_node(struct rb_tree * tree, struct rb_node * root,
                void * key, void * data)
{
        if (root == NULL) {
                root = rb_make_node(tree, key, data);
        }
        else {
                int dir;
                //dir = root->key < key ? 1 : 0;
                dir = tree->info.keycomp(root->key, key) < 0 ? 1 : 0;

                root->link[dir] = rb_insert_node(tree, root->link[dir], key, data);

                if (is_red(root->link[dir])) {
                        if (is_red(root->link[dir ^ 1])) {
                                // Case 1
                                root->color = RBT_RED;
                                root->link[0]->color = RBT_BLACK;
                                root->link[1]->color = RBT_BLACK;
                        }
                        else if (is_red(root->link[dir]->link[dir])) {
                                // Case 3
                                root = rotation(root, dir ^ 1);
                        }
                        else if (is_red(root->link[dir]->link[dir ^ 1])) {
                                // Case 2
                                root = double_rotation(root, dir ^ 1);
                        }
                }
        }
        return root;
}

int rb_insert(RBTREE * t, void * key, void * data)
{
        struct rb_tree * tree = (struct rb_tree *)t;
        if (tree->valid != _RB_TREE_VALID) return -1;

        if (tree->root == NULL) {
                tree->root = rb_make_node(tree, key, data);
                if (tree->root == NULL)
                        return -1;
        }
        else {
                struct rb_node head = { 0 }; // False root}
                struct rb_node *g, *t; // Grandparent & parent
                struct rb_node *p, *q; // Iterator & parent
                int dir = 0, last;

                /* Set Up Helpers */
                t = &head;
                g = p = NULL;
                q = t->link[1] = tree->root;

                /* Search down the tree */
                for (;;) {
                        if (q == NULL) {
                                /* Insert new node at the bottom */
                                p->link[dir] = q = rb_make_node(tree, key, data);
                                if (q == NULL)
                                        return -1;
                        }
                        else if (is_red(q->link[0]) && is_red(q->link[1])) {
                                // Color Flip
                                q->color = RBT_RED;
                                q->link[0]->color = RBT_BLACK;
                                q->link[1]->color = RBT_BLACK;
                        }

                        /* Fix Red Violation */
                        if (is_red(q) && is_red(p)) {
                                int dir2 = t->link[1] == g ? 1 : 0;
                                if (q == p->link[last]) {
                                        t->link[dir2] = rotation(g, last ^ 1);
                                }
                                else {
                                        t->link[dir2] = double_rotation(g, last ^ 1);
                                }
                        }

                        /* Stop if found */
                        if (tree->info.keycomp(q->key, key) == 0) {
                                break;
                        }
                        last = dir;
                        dir = tree->info.keycomp(q->key, key) < 0 ? 1 : 0;

                        /* Update helpers */
                        if (g != NULL) {
                                t = g;
                        }
                        g = p, p = q;
                        q = q->link[dir];
                }
                /* update root */
                tree->root = head.link[1];
        }
        tree->root->color = RBT_BLACK;
        return 0;
}

int rb_size_node(struct rb_node * root)
{
        if (root == NULL)
                return 0;
        int l = rb_size_node(root->link[0]);
        int r = rb_size_node(root->link[1]);
        return 1 + l + r;
}

int rb_size(RBTREE * t)
{
        if (t == NULL)
                return 0;

        struct rb_tree * tree = (struct rb_tree *)t;
        if (tree->valid != _RB_TREE_VALID) return -1;
        return rb_size_node(tree->root);
}

int rb_has_node(struct rb_tree * tree, struct rb_node * root, void * key) {
        if (root == NULL)
                return 0;

        int comp = tree->info.keycomp(key, root->key);
        if (comp == 0)
                return 1;
        else if (comp < 0)
                return rb_has_node(tree, root->link[0], key);
        else
                return rb_has_node(tree, root->link[1], key);
}

int rb_has(RBTREE * t, void * key)
{
        struct rb_tree * tree = (struct rb_tree *)t;
        if (tree->valid != _RB_TREE_VALID) {
                errno = EINVAL;
                return -1;
        }
        return rb_has_node(tree, tree->root, key);
}

void rb_free_node(struct rb_tree * tree, struct rb_node * root)
{
        if (root == NULL)
                return;

        rb_free_node(tree, root->link[0]);
        rb_free_node(tree, root->link[1]);
        tree->info.keyfree(root->key);
        free(root);
}

int rb_free(RBTREE * t)
{
        struct rb_tree * tree = (struct rb_tree *)t;
        if (tree->valid != _RB_TREE_VALID) {
                errno = EINVAL;
                return -1;
        }
        rb_free_node(tree, tree->root);
        free(t);
        return 0;
}

struct rb_node * rb_remove_node(struct rb_tree * tree, struct rb_node * root,
                void * key, int * done)
{
        //TODO
        return root;
}

int rb_remove(RBTREE * t, void * key)
{
        struct rb_tree * tree = (struct rb_tree *)t;
        if (tree->valid != _RB_TREE_VALID) {
                errno = EINVAL;
                return -1;
        }

        if (tree->root == NULL)
                return 0;

        struct rb_node head = { 0 };  // False root
        struct rb_node * q, * p, * g; // Heleprs
        struct rb_node * f = NULL;    // found item
        int dir = 1;

        // Set up helpers
        q = &head;
        g = p = NULL;
        q->link[1] = tree->root;

        // Search and push down a red
        while (q->link[dir] != NULL)
        {
                int last = dir;

                // Update Helpers
                g = p, p = q;
                q = q->link[dir];
                int comp = tree->info.keycomp(q->key, key);
                dir = comp < 0;

                // Save found node
                if (comp == 0) {
                        f = q;
                }

                // Push the red node down
                if (!is_red(q) && !is_red(q->link[dir])) {
                        if (is_red(q->link[dir ^ 1])) {
                                p = p->link[last] = rotation(q, dir);
                        }

                        else if (!is_red(q->link[dir ^ 1])) {
                                // Sibling
                                struct rb_node * s = p->link[last ^ 1];
                                
                                if (s != NULL) {
                                        if (!is_red(s->link[last ^ 1]) &&
                                                        !is_red(s->link[last])) {
                                                // Color Flip
                                                p->color = RBT_BLACK;
                                                s->color = RBT_RED;
                                                q->color = RBT_RED;
                                        }
                                        else {
                                                int dir2 = g->link[1] == p;

                                                if (is_red(s->link[last])) {
                                                        g->link[dir2] = double_rotation(p, last);
                                                }
                                                else if (is_red(s->link[last ^ 1])) {
                                                        g->link[dir2] = rotation(p, last);
                                                }
                                                // ensure correct coloring
                                                q->color = RBT_RED;
                                                g->link[dir2]->color = RBT_RED;
                                                g->link[dir2]->link[0]->color = RBT_BLACK;
                                                g->link[dir2]->link[1]->color = RBT_BLACK;
                                        }
                                }
                        }
                }
        }
        
        // Replace and remove if found
        int rv = 0;
        if (f != NULL) {
                tree->info.keycopy(&(f->key), &(q->key));
                p->link[p->link[1] == q] = q->link[q->link[0] == NULL];
                tree->info.keyfree(q->key);
                free(q);
                rv = 1;
        }

        // Update root and make it black
        tree->root = head.link[1];
        if (tree->root != NULL)
                tree->root->color = RBT_BLACK;

        return rv;
}

