#ifndef _LL_H_
#define _LL_H_
#include <stddef.h>
#include <stdlib.h>

#define LL_DECL(data_typename, ll_typename_prefix) \
  typedef struct s_##data_typename##_ll_node { \
    data_typename data; \
    struct s_##data_typename##_ll_node *next; \
  } ll_typename_prefix##_LL_Node_t; \
  typedef struct s_##data_typename##_ll { \
    ll_typename_prefix##_LL_Node_t *head; \
    ll_typename_prefix##_LL_Node_t *tail; \
    size_t nmemb; \
  } ll_typename_prefix##_LL_t

#define LL_INIT(ll_typename_prefix) ((ll_typename_prefix##_LL_t){NULL, NULL, 0UL})

#define LL_NO_NODE_VAR_INITIALIZER(ll_typename_prefix, varname) varname
#define LL_NODE_VAR_INITIALIZER(ll_typename_prefix, varname) ll_typename_prefix##_LL_Node_t *varname
#define LL_FOREACH(var_initializer, var, ll) \
  for (var_initializer=ll->head; NULL!=var; var = var->next)

#define LL_INIT_NODE(nodep, insert_data, nextp) \
  nodep->data = insert_data; \
  nodep->next = nextp

#define LL_NODE_APPEND(ll_typename_prefix, ll, insert_data) \
    do if (ll->nmemb++==0) { \
      ll_typename_prefix##_LL_Node_t *node = ll->head = ll->tail = \
          (ll_typename_prefix##_LL_Node_t*)malloc(sizeof(ll_typename_prefix##_LL_Node_t)); \
      node->data = insert_data; \
      node->next = NULL; \
    } else {\
      ll_typename_prefix##_LL_Node_t *node = ll->tail->next = \
          (ll_typename_prefix##_LL_Node_t*)malloc(sizeof(ll_typename_prefix##_LL_Node_t)); \
      node->data = insert_data; \
      node->next = NULL; \
      ll->tail = node; \
    } while (0)


#define LL_NODE_PREPEND(ll_typename_prefix, ll, insert_data) \
  do if (ll->nmemb++==0) { \
    ll_typename_prefix##_LL_Node_t *node = ll->head = ll->tail = \
      (ll_typename_prefix##_LL_Node_t*)malloc(sizeof(ll_typename_prefix##_LL_Node_t)); \
    node->data = insert_data; \
    node->next = NULL; \
  } else { \
    ll_typename_prefix##_LL_Node_t *node = (ll_typename_prefix##_LL_Node_t*)malloc(sizeof(ll_typename_prefix##_LL_Node_t)); \
    node->data = insert_data; \
    node->next = ll->head; \
    ll->head = node; \
  } while (0)

#define LL_HEAD_RM(ll_typename_prefix, ll, dest) \
  do { \
    if (ll->nmemb==0) \
      break; \
    if (--(ll->nmemb)==0) { \
      dest = ll->head->data; \
      free((void*)(ll->head)); \
      ll->head = ll->tail = NULL; \
    } else { \
      ll_typename_prefix##_LL_Node_t *rm = ll->head; \
      dest = rm->data; \
      ll->head = rm->next; \
      free((void*)rm); \
    } \
  } while (0)

#define LL_TAIL_RM(ll_typename_prefix, ll, dest) \
  do { \
    if (ll->nmemb==0) break; \
    if (--(ll->nmemb)==0) { \
      dest = ll->tail->data; \
      free((void*)(ll->tail)); \
      ll->head=ll->tail=NULL; \
    } else {\
      ll_typename_prefix##_LL_Node_t *rm = ll->tail, *newtail=ll->head, *nxt; \
      dest = rm->data; \
      while ((nxt=newtail->next)!=rm) newtail = nxt; \
      ll->tail = newtail; \
      newtail->next = NULL; \
      free((void*)rm); \
    } \
  } while (0)

#define LL_CLOSE(ll_typename_prefix, ll) \
  do { \
    size_t nm = ll->nmemb; \
    if (!nm) { \
      assert(NULL==ll->head && NULL == ll->tail); \
      break; \
    } \
    for (ll_typename_prefix##_LL_Node_t *node = ll->head, *nxt=NULL; node; node = nxt) { \
      nxt = node->next; \
      free(node); \
      --nm; \
    } \
    assert(nm == 0UL); \
    ll->nmemb = 0UL; \
    ll->head = ll->tail = NULL; \
  } while (0)


#endif  /* _LL_H_ */
