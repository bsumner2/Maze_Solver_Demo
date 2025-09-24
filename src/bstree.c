/******************************************************************************\
|*************************** Author: Burt O Sumner ****************************|
|******** Copyright 2025 (C) Burt O Sumner | All Rights Reserved **************|
\******************************************************************************/
#include "bstree.h"
#include <stdlib.h>
#include <string.h>

#define NODE_HEIGHT(root) ((root!=NULL) ? ( root->height) : (-1))
#define NODE_BALANCE_FACT(root)\
  ((root!=NULL) ? (NODE_HEIGHT(root->l) - NODE_HEIGHT(root->r)) : 0)

struct s_bst {
  BinaryTreeNode_t *root;
  size_t nmemb;
  Alloc_cb alloc_cb;
  Dealloc_cb dealloc_cb;
  Initializer_cb init_cb;
  Uninitializer_cb uninit_cb;
  Comparison_cb cmp_cb;
  size_t alloc_size;
};

enum e_binary_tree_errcode {
  E_BST_ERR_NONE=0,
  E_BST_ERR_STATIC_BUT_INITIALIZER=1,
  E_BST_ERR_STATIC_BUT_UNINITIALIZER=2,
  E_BST_ERR_NO_CMP_FUNC=4,
};


const char *BinaryTree_GetError(int errcode) {
  switch (errcode) {
  case E_BST_ERR_NONE:
    return "No error occurred.";
  case E_BST_ERR_STATIC_BUT_INITIALIZER:
    return "Data alloc size set to zero (implying static data insertion), but "
      "still provided a data initializer. Please provide a data size if you "
      "mean to use dynamic data insertion. Otherwise, leave initializer and "
      "uninitializer NULL., along with data_alloc_size = 0 for static data "
      "insertion";
  case E_BST_ERR_STATIC_BUT_UNINITIALIZER:
    return "Data alloc size set to zero (implying static data insertion), but "
      "still provided a data uninitializer. Please provide a data size if you "
      "mean to use dynamic data insertion. Otherwise, leave initializer and "
      "uninitializer NULL, along with data_alloc_size = 0 for static data "
      "insertion.";
  case E_BST_ERR_STATIC_BUT_INITIALIZER|E_BST_ERR_STATIC_BUT_UNINITIALIZER:
    return "Data alloc size set to zero (implying static data insertion), but "
      "still provided a data initializer and uninitializer. Please provide a "
      "data size if you mean to use dynamic data insertion. Otherwise, leave "
      "initializer and uninitializer NULL, along with data_alloc_size = 0 for "
      "static data insertion.";
  case E_BST_ERR_NO_CMP_FUNC:
    return "No comparison function provided.";
  default:
    return "Unknown error code.";
  }
}

static int BinaryTreeNode_HibbardDelete(BinaryTreeNode_t ***stack,
                                        BinaryTreeNode_t *root,
                                        Dealloc_cb dealloc,
                                        Uninitializer_cb uninit,
                                        int top,
                                        bool dynamic_data);

static BinaryTreeNode_t *BinaryTreeNode_LL_Rotate(BinaryTreeNode_t *root);
static BinaryTreeNode_t *BinaryTreeNode_LR_Rotate(BinaryTreeNode_t *root);
static BinaryTreeNode_t *BinaryTreeNode_RR_Rotate(BinaryTreeNode_t *root);
static BinaryTreeNode_t *BinaryTreeNode_RL_Rotate(BinaryTreeNode_t *root);

static void BinaryTreeNode_Destroy(BinaryTreeNode_t *node,
                                   Dealloc_cb dealloc,
                                   Uninitializer_cb uninit,
                                   bool dynamic_data);

static void BinaryTreeNode_Recalc_Height(BinaryTreeNode_t *root) {
  int lh, rh;
  if (!root)
    return;
  lh = NODE_HEIGHT(root->l), rh = NODE_HEIGHT(root->r);
  root->height = 1 + ((lh > rh) ? lh : rh);
}

static BinaryTreeNode_t *BinaryTreeNode_Create(
    const void *data,
    Alloc_cb allocator,
    Initializer_cb init,
    size_t data_size) {
  BinaryTreeNode_t *ret;
  void *newdata;
  if (0!=data_size) {
    newdata = allocator(data_size);
    if (NULL == newdata)
      return NULL;
    if (NULL!=init) {
      init(newdata, data);
    } else {
      memcpy(newdata, data, data_size);
    }
  } else {
    newdata = (void*)data;
  }
  ret = (BinaryTreeNode_t*)allocator(sizeof(BinaryTreeNode_t));
  ret->data = newdata;
  ret->l = NULL;
  ret->r = NULL;
  ret->height = 0;
  return ret;
}





void BinaryTreeNode_Destroy(BinaryTreeNode_t *node,
                                   Dealloc_cb dealloc,
                                   Uninitializer_cb uninit,
                                   bool dynamic_data) {
  if (NULL == node)
    return;
  if (!dynamic_data) {
    dealloc(node);
    return;
  }
  if (NULL != uninit)
    uninit(node->data);
  dealloc(node->data);
  dealloc(node);
}


int BinaryTreeNode_HibbardDelete(BinaryTreeNode_t ***stack,
                                 BinaryTreeNode_t *root,
                                 Dealloc_cb dealloc,
                                 Uninitializer_cb uninit,
                                 int top,
                                 bool dynamic_data) {
  BinaryTreeNode_t *cur;
  for (cur = *(stack[++top] = &(root->r))
        ; NULL!=cur->l
        ; cur = *(stack[++top] = &(cur->l)))
    continue;
  /* Now we're at the minimum of the root's RHS subtree */

  *stack[top--] = cur->r;  /* Tethered minimum node's replacement, so now we 
                            * can safely dealloc cur without having to worry 
                            * about dysjointed subtree */

  if (!dynamic_data) {
    root->data = cur->data;
    dealloc(cur);
  } else {
    if (NULL != uninit)
      uninit(root->data);
    dealloc(root->data);
    root->data = cur->data;
    dealloc(cur); 
  }
  return top;
}



BinaryTreeNode_t *BinaryTreeNode_LL_Rotate(BinaryTreeNode_t *root) {
  BinaryTreeNode_t *newroot = root->l;
  root->l = newroot->r;
  BinaryTreeNode_Recalc_Height(root);
  newroot->r = root;
  BinaryTreeNode_Recalc_Height(newroot);
  return newroot;
}

BinaryTreeNode_t *BinaryTreeNode_LR_Rotate(BinaryTreeNode_t *root) {
  BinaryTreeNode_t *newroot = root->l->r;
  root->l->r = newroot->l;
  BinaryTreeNode_Recalc_Height(root->l);
  newroot->l = root->l;
  root->l = newroot->r;
  BinaryTreeNode_Recalc_Height(root);
  newroot->r = root;
  BinaryTreeNode_Recalc_Height(newroot);
  return newroot;
}


BinaryTreeNode_t *BinaryTreeNode_RR_Rotate(BinaryTreeNode_t *root) {
  BinaryTreeNode_t *newroot = root->r;
  root->r = newroot->l;
  BinaryTreeNode_Recalc_Height(root);
  newroot->l = root;
  BinaryTreeNode_Recalc_Height(newroot);
  return newroot;
}

BinaryTreeNode_t *BinaryTreeNode_RL_Rotate(BinaryTreeNode_t *root) {
  BinaryTreeNode_t *newroot = root->r->l;
  root->r->l = newroot->r;
  BinaryTreeNode_Recalc_Height(root->r);
  newroot->r = root->r;
  root->r = newroot->l;
  BinaryTreeNode_Recalc_Height(root);
  newroot->l = root;
  BinaryTreeNode_Recalc_Height(newroot);
  return newroot;
}




BinaryTree_t *BinaryTree_Create(Alloc_cb allocator_callback,
    Dealloc_cb deallocator_callback, 
    Initializer_cb data_init_callback,
    Uninitializer_cb data_uninit_callback, 
    Comparison_cb data_comparison_callback,
    size_t data_alloc_size, int *return_errcode) {
  BinaryTree_t *ret;
  int err = 0;
  if (NULL==allocator_callback) {
    allocator_callback = malloc;
  } else if (NULL==deallocator_callback) {
    deallocator_callback = free;
  }
  if (!data_alloc_size) {
    if (NULL!=data_init_callback) {
      err |= E_BST_ERR_STATIC_BUT_INITIALIZER;
    } else if (NULL!=data_uninit_callback) {
      err |= E_BST_ERR_STATIC_BUT_UNINITIALIZER;
    }
  }
  if (NULL==data_comparison_callback) {
    err |= E_BST_ERR_NO_CMP_FUNC;
  }
  if (0!=err) {
    if (NULL != return_errcode)
      *return_errcode = err;
    return NULL;
  }
  ret = (BinaryTree_t*)allocator_callback(sizeof(BinaryTree_t));

  *ret = (BinaryTree_t){
    .root = NULL,
    .nmemb = (size_t)0,
    .alloc_cb = allocator_callback,
    .dealloc_cb = deallocator_callback,
    .init_cb = data_init_callback,
    .uninit_cb = data_uninit_callback,
    .cmp_cb = data_comparison_callback,
    .alloc_size = data_alloc_size
  };
  if (NULL != return_errcode)
    *return_errcode = 0;
  return ret;
}

bool BinaryTree_Insert(BinaryTree_t *tree, const void *data) {
  if (NULL == tree || NULL == data) {
    return false;
  }
  Comparison_cb cmp = tree->cmp_cb;
  size_t nmemb = tree->nmemb;
  int val;
  if ((size_t)2 > nmemb) {
    if (!nmemb) {
      tree->nmemb = (size_t)1;
      tree->root = BinaryTreeNode_Create(data, tree->alloc_cb, tree->init_cb, tree->alloc_size);
      return true;
    } else {
      val = cmp(data, tree->root->data);
      if (0==val) {
        return false;
      } else if (0>val) {
        tree->nmemb = (size_t)2;
        tree->root->l = BinaryTreeNode_Create(data, tree->alloc_cb, tree->init_cb, tree->alloc_size);
        tree->root->height = 1;
        return true;
      } else {
        tree->nmemb = (size_t)2;
        tree->root->r = BinaryTreeNode_Create(data, tree->alloc_cb, tree->init_cb, tree->alloc_size);
        tree->root->height = 1;
        return true;
      }
    }
  }
  BinaryTreeNode_t **stack[tree->root->height+2];
  int top = -1;
  stack[++top] = &tree->root;
  for (BinaryTreeNode_t *cur = tree->root; NULL!=cur; ) {
    val = cmp(data, cur->data);
    if (0 == val) {
      return false;
    } else if (0 > val) {
      stack[++top] = &cur->l;
      cur = cur->l;
    } else {
      stack[++top] = &cur->r;
      cur = cur->r;
    }
  }
  ++tree->nmemb;
  *stack[top--] = BinaryTreeNode_Create(data,
                                             tree->alloc_cb,
                                             tree->init_cb,
                                             tree->alloc_size);

  for (BinaryTreeNode_t **curp, *cur = *(curp = stack[top--])
          ; ; cur = *(curp = stack[top--])) {
    val = NODE_BALANCE_FACT(cur);
    if (1 < val) {
      if (cmp(data, cur->l->data) < 0) {
        cur = BinaryTreeNode_LL_Rotate(cur);
      } else {
        cur = BinaryTreeNode_LR_Rotate(cur);
      }
    } else if (-1 > val) {
      if (cmp(data, cur->r->data) > 0) {
        cur = BinaryTreeNode_RR_Rotate(cur);
      } else {
        cur = BinaryTreeNode_RL_Rotate(cur);
      }
    } else {
      BinaryTreeNode_Recalc_Height(cur);
    }
    *curp = cur;
    if (-1 == top)
      break;
  }
  return true;
}

bool BinaryTree_Remove(BinaryTree_t *tree, const void *data) {
  if (NULL == tree  || NULL == data) {
    return false;
  }
  Comparison_cb cmp = tree->cmp_cb;
  size_t nmemb = tree->nmemb;
  bool dynamic_data = 0!=tree->alloc_size;
  if ((size_t)2 > nmemb) {
    if ((size_t)0 == nmemb)
      return false;
    if (0!=cmp(data, tree->root->data))
      return false;
    BinaryTreeNode_Destroy(tree->root, 
                           tree->dealloc_cb,
                           tree->uninit_cb,
                           dynamic_data);
    tree->root = NULL;
    --tree->nmemb;
    return true;
  }
  BinaryTreeNode_t **stack[tree->root->height+2], **curp, *cur;
  int top = -1, val;
  for (val = cmp(data, (cur = *(stack[++top] = &(tree->root)))->data)
      ; 0!=val
      ; val = cmp(data, cur->data)) {
    if (0 > val) {
      cur = *(stack[++top] = &(cur->l));
    } else {
      cur = *(stack[++top] = &(cur->r));
    }
    if (NULL == cur) {
      return false;
    }
  }
  --tree->nmemb;
  if (NULL == cur->r) {
    *stack[top--] = cur->l;
    BinaryTreeNode_Destroy(cur, 
                           tree->dealloc_cb,
                           tree->uninit_cb,
                           dynamic_data);
    if (-1 == top)
      return true;
  } else if (NULL == cur->l) {
    *stack[top--] = cur->r;
    BinaryTreeNode_Destroy(cur, 
                           tree->dealloc_cb,
                           tree->uninit_cb,
                           dynamic_data);
    if (-1 == top)
      return true;
  } else {
    top = BinaryTreeNode_HibbardDelete(stack,
                                       cur,
                                       tree->dealloc_cb,
                                       tree->uninit_cb, 
                                       top,
                                       dynamic_data);
  }


  for (cur = *(curp = stack[top--]); ; cur = *(curp = stack[top--])) {
    val = NODE_BALANCE_FACT(cur);
    if (1 < val) {
      if (0 <= NODE_BALANCE_FACT(cur->l))
        cur = BinaryTreeNode_LL_Rotate(cur);
      else
        cur = BinaryTreeNode_LR_Rotate(cur);
    } else if (-1 > val) {
      if (0 >= NODE_BALANCE_FACT(cur->r))
        cur = BinaryTreeNode_RR_Rotate(cur);
      else
        cur = BinaryTreeNode_RL_Rotate(cur);
    } else {
      BinaryTreeNode_Recalc_Height(cur);
    }
    *curp = cur;
    if (-1 == top)
      break;
  }

  return true;

}

bool BinaryTree_Contains(BinaryTree_t *tree, const void *data) {
  if (NULL == tree || NULL == data)
    return false;
  Comparison_cb cmp = tree->cmp_cb;
  int val;
  for (BinaryTreeNode_t *root = tree->root
      ; NULL != root
      ; root = (0 > val) ? root->l : root->r) {
    if (0 == (val = cmp(data, root->data)))
      return true;
  }
  return false;
}

void BinaryTree_RemoveAll(BinaryTree_t *tree) {
  Uninitializer_cb uninit;
  Dealloc_cb dealloc;
  size_t nmemb;
  bool dynamic_data;
  if (NULL == tree) return;
  uninit = tree->uninit_cb;
  dealloc = tree->dealloc_cb;
  nmemb = tree->nmemb;
  dynamic_data = 0!=tree->alloc_size;
  if ((size_t)0 == nmemb)
    return;
  BinaryTreeNode_t *stack[tree->nmemb], *cur;
  int top = -1;
  stack[++top] = tree->root;
  do {
    cur = stack[top];
    if (NULL == cur) {
      --top;
      continue;
    }
    stack[top] = cur->l;
    stack[++top] = cur->r;
    BinaryTreeNode_Destroy(cur, dealloc, uninit, dynamic_data);
  } while (-1 < top);
  tree->nmemb = (size_t)0;
  tree->root = NULL;
}

void BinaryTree_Destroy(BinaryTree_t *tree) {
  if (NULL == tree)
    return;
  Dealloc_cb dealloc = tree->dealloc_cb;
  if (0 == tree->nmemb) {

    dealloc(tree);
    return;
  }
  BinaryTree_RemoveAll(tree);
  if (NULL == dealloc)
    free(tree);
  else
    dealloc(tree);
}

void *BinaryTree_Remove_Minimum(BinaryTree_t *tree) {
  Dealloc_cb dealloc;
  void *ret;
  if (NULL == tree)
    return NULL;
  dealloc = tree->dealloc_cb;
  if (NULL == dealloc)
    dealloc = free;
  if ((size_t)2 > tree->nmemb) {
    if ((size_t)0 == tree->nmemb)
      return NULL;
    ret = tree->root->data;
    dealloc(tree->root);
    tree->root = NULL;
    tree->nmemb = 0;
    return ret;
  }
  if (NULL == tree->root->l) {
    BinaryTreeNode_t *newroot = tree->root->r;
    ret = tree->root->data;
    dealloc(tree->root);
    tree->root = newroot;
    --tree->nmemb;
    return ret;
  }

  BinaryTreeNode_t *stack[tree->root->l->height+3], *root = tree->root;
  int top = -1;
  for (stack[++top] = root; NULL!=root->l; stack[++top] = root = root->l)
    continue;
  stack[top] = root->r;
  ret = root->data;
  dealloc(root);
  root = stack[top--];
  --tree->nmemb;
  for (int bal; -1 < top; ) {
    stack[top]->l = root;
    root = stack[top--];
    bal = NODE_BALANCE_FACT(root);
    if (1 < bal) {
      if (0 <= NODE_BALANCE_FACT(root->l))
        root = BinaryTreeNode_LL_Rotate(root);
      else
        root = BinaryTreeNode_LR_Rotate(root);
    } else if (-1 > bal) {
      if (0 >= NODE_BALANCE_FACT(root->r))
        root = BinaryTreeNode_RR_Rotate(root);
      else
        root = BinaryTreeNode_RL_Rotate(root);
    } else {
      BinaryTreeNode_Recalc_Height(root);
    }
  }
  tree->root = root;
  return ret;
}

void *BinaryTree_Retrieve(BinaryTree_t *tree, void *key) {
  if (NULL == tree || NULL == key)
    return NULL;
  Comparison_cb cmp = tree->cmp_cb;
  int val;
  for (BinaryTreeNode_t *root = tree->root
      ; NULL!=root
      ; root = 0 > val ? root->l : root->r) {
    val = cmp(key, root->data);
    if (0==val)
      return root->data;
  }
  return NULL;

}

size_t BinaryTree_Element_Count(BinaryTree_t *tree) {
  if (NULL == tree)
    return (size_t)0;
  return tree->nmemb;

}

BinaryTreeNode_t *BinaryTree_Get_Root(BinaryTree_t *tree) {
  if (NULL == tree)
    return NULL;
  return tree->root;
}
