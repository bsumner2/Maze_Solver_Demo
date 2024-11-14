#ifndef _BSTREE_H_
#define _BSTREE_H_
#include <stddef.h>
#include <stdbool.h>

typedef void* (*Alloc_cb)(size_t);
typedef void (*Initializer_cb)(void *dst_data, const void *src_data);
typedef int (*Comparison_cb)(const void*, const void*);
typedef void (*Dealloc_cb)(void*);
typedef void (*Uninitializer_cb)(void*);
typedef struct s_bst BinaryTree_t;

typedef struct s_bst_node {
  void *data;
  int height;
  struct s_bst_node *l, *r;
} BinaryTreeNode_t;
 

const char *BinaryTree_Strerror(void);

int BinaryTree_Initialize(BinaryTree_t *static_inst, 
    Alloc_cb allocator_callback,
    Dealloc_cb deallocator_callback, 
    Initializer_cb data_init_callback,
    Uninitializer_cb data_uninit_callback, 
    Comparison_cb data_comparison_callback,
    size_t data_alloc_size);

BinaryTree_t *BinaryTree_Create(Alloc_cb allocator_callback,
    Dealloc_cb deallocator_callback, 
    Initializer_cb data_init_callback,
    Uninitializer_cb data_uninit_callback, 
    Comparison_cb data_comparison_callback,
    size_t data_alloc_size);;

int BinaryTree_Insert(BinaryTree_t *tree, void *data);
int BinaryTree_Remove(BinaryTree_t *tree, void *data);
bool BinaryTree_Contains(BinaryTree_t *tree, void *data);
void BinaryTree_Uninitialize(BinaryTree_t *static_inst);
void BinaryTree_Destroy(BinaryTree_t *tree);
void *BinaryTree_Remove_Minimum(BinaryTree_t *tree);
void *BinaryTree_Retrieve(BinaryTree_t *tree, void *key);
size_t BinaryTree_Element_Count(BinaryTree_t *tree);
BinaryTreeNode_t *BinaryTree_Get_Root(BinaryTree_t *tree);


#endif  /* _BSTREE_H_ */
