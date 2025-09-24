/******************************************************************************\
|*************************** Author: Burt O Sumner ****************************|
|******** Copyright 2025 (C) Burt O Sumner | All Rights Reserved **************|
\******************************************************************************/
#ifndef _BSTREE_H_
#define _BSTREE_H_

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#include <stdbool.h>
#endif  /* C++ name mangler guard & diffs in std header names for C vs C++ */

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


 



#define BinaryTree_Create_StaticDataInsertion(\
    bst_alloc_cb, bst_free_cb,data_cmp_cb, return_errcode)\
  BinaryTree_Create(bst_alloc_cb,\
                    bst_free_cb,\
                    NULL,\
                    NULL,\
                    data_cmp_cb,\
                    (size_t)0,\
                    return_errcode)

#define BinaryTree_Create_DynamicDataInsertion(\
    bst_and_data_alloc_cb, bst_and_data_free_cb, data_init_cb,\
    data_uninit_cb, data_cmp_cb, data_alloc_size, return_errcode)\
  BinaryTree_Create(bst_and_data_alloc_cb,\
                    bst_and_data_free_cb,\
                    data_init_cb,\
                    data_uninit_cb,\
                    data_cmp_cb,\
                    data_alloc_cb,\
                    return_errcode)


BinaryTree_t *BinaryTree_Create(Alloc_cb allocator_callback,
    Dealloc_cb deallocator_callback,
    Initializer_cb data_init_callback,
    Uninitializer_cb data_uninit_callback,
    Comparison_cb data_comparison_callback,
    size_t data_alloc_size,
    int *return_errcode);

bool BinaryTree_Insert(BinaryTree_t *tree, const void *data);
bool BinaryTree_Remove(BinaryTree_t *tree, const void *data);
bool BinaryTree_Contains(BinaryTree_t *tree, const void *data);
void BinaryTree_Destroy(BinaryTree_t *tree);
void *BinaryTree_Remove_Minimum(BinaryTree_t *tree);
void *BinaryTree_Retrieve(BinaryTree_t *tree, void *key);
size_t BinaryTree_Element_Count(BinaryTree_t *tree);
BinaryTreeNode_t *BinaryTree_Get_Root(BinaryTree_t *tree);
void BinaryTree_RemoveAll(BinaryTree_t *tree);

#ifdef __cplusplus
}
#endif  /* C++ name mangler guard */

#endif  /* _BSTREE_H_ */
