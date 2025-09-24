#include "bstree.h"
#include "linked_list.h"
#include "graph.h"
#include <stdbool.h>
#include <string.h>

#define TEST_EDGE_ADDITIONS

Graph_t *Graph_Init(Data_Initializer_cb initializer, Data_Uninitializer_cb uninitializer, Comparison_cb vertex_data_cmp_cb, size_t vertex_data_size) {
  Graph_t *ret;
  if (NULL==initializer) {
    if (NULL!=uninitializer) {
      return false;
    }
  }
  if (0UL == vertex_data_size)
    return false;
  if (NULL == vertex_data_cmp_cb)
    return false;
  ret = malloc(sizeof(Graph_t));
  if (ret == NULL)
    return NULL;

  ret->vertex_ct = 0UL;
  ret->vertices = NULL;
  ret->data_initer = initializer;
  ret->data_uniniter = uninitializer;
  ret->vertex_data_map = BinaryTree_Create(malloc, free, NULL, NULL, vertex_data_cmp_cb, vertex_data_size+sizeof(void*), NULL);
  ret->vertex_data_size = vertex_data_size;
  return ret;
}

bool Graph_Add_Vertex(Graph_t *graph, const void *vertex_data) {
  if (!graph || !vertex_data)
    return false;
  uint8_t *vmap_entry = malloc(sizeof(uint8_t)*(graph->vertex_data_size + sizeof(void*)));
  memcpy(vmap_entry, vertex_data, graph->vertex_data_size);
  memset(&vmap_entry[graph->vertex_data_size], 0, sizeof(void*));
  if (BinaryTree_Contains(graph->vertex_data_map, vmap_entry)) {
    free(vmap_entry);
    return false;
  }
  int newnode_idx = graph->vertex_ct;
  GraphNode_t *graphnodes, *new;
  assert(NULL != (graphnodes = realloc(graph->vertices, sizeof(GraphNode_t)*(newnode_idx+1))));
  graph->vertices = graphnodes;

  
  new = &graphnodes[newnode_idx];
  *(int*)(&vmap_entry[graph->vertex_data_size]) = newnode_idx;
  new->idx = newnode_idx;
  new->adj_list = LL_INIT(GraphEdge);
  new->data = malloc(graph->vertex_data_size);
  if (graph->data_initer)
    graph->data_initer(new->data, vertex_data);
  else
    memcpy(new->data, vertex_data, graph->vertex_data_size);
  assert(BinaryTree_Insert(graph->vertex_data_map, vmap_entry));
  ++(graph->vertex_ct);
  free(vmap_entry);
  return true;
}


bool Graph_Add_Edge(Graph_t *graph, int src_vertex, int dst_vertex, int weight) {
  if (!graph)
    return false;
  if (src_vertex < 0 || (unsigned)src_vertex >= graph->vertex_ct)
    return false;
  if (dst_vertex < 0 || (unsigned)dst_vertex >= graph->vertex_ct)
    return false;
  GraphNode_t *src = graph->vertices + src_vertex;
  GraphEdge_LL_t *adjs = &(src->adj_list);
  GraphEdge_t edge = (GraphEdge_t){.weight = weight, .dst_idx = dst_vertex};
  if (adjs->nmemb==0UL) {
    LL_NODE_PREPEND(GraphEdge, adjs, edge);
    return true;
  }
  if (dst_vertex < adjs->head->data.dst_idx) {
    LL_NODE_PREPEND(GraphEdge, adjs, edge);
    return true;
  }
  LL_NODE_VAR_INITIALIZER(GraphEdge, node);

#ifdef TEST_EDGE_ADDITIONS
  LL_NODE_VAR_INITIALIZER(GraphEdge, prev) = NULL;
#endif
  LL_FOREACH(LL_NO_NODE_VAR_INITIALIZER(GraphEdge, node), node, adjs) {
    if (dst_vertex > node->data.dst_idx)  {
#ifdef TEST_EDGE_ADDITIONS
      prev = node;
#endif
      continue;
    }
    if (dst_vertex == node->data.dst_idx)
      return false;  // return false. Make caller use update weight to update existing edge's weight
    break;
  }
  // if node is NULL, then foreach never preemptively broke, so idx of dst_vertex 
  // is largest of the bunch. Append it as new tail using LL_APPEND
  if (!node) {
#ifdef TEST_EDGE_ADDITIONS
    assert(prev == adjs->tail); 
#endif
    LL_NODE_APPEND(GraphEdge, adjs, edge);
    return true;
  }

#ifndef TEST_EDGE_ADDITIONS
  // Otherwise, foreach broke at node whose dst_idx > insert edge vertex's idx,
  // so we need to prepend insert node data. Since LL is single link, we can
  // just alloc new and move node's orig data and next ptr to new, and then
  // assign insert data at node and assign new to node->next. This preserves
  // the link for link, prev->next := node, while also preserving sorted order.
  GraphEdge_LL_Node_t *new = malloc(sizeof(GraphEdge_LL_Node_t));
  new->data = node->data;
  new->next = node->next;
  node->data = edge;
  node->next = new;
  ++(adjs->nmemb);
  if (adjs->tail == node) {
    adjs->tail = new;
  }
#else
  GraphEdge_LL_Node_t *insert = malloc(sizeof(*insert));
  insert->next = node;
  prev->next = insert;
  insert->data = edge;
  ++(adjs->nmemb);
#endif
  return true;
}




bool Graph_Update_Edge(Graph_t *graph, int src_vertex, int dst_vertex, int new_weight) {
  if (!graph)
    return false;
  if (src_vertex < 0 || (unsigned)src_vertex >= graph->vertex_ct)
    return false;
  if (dst_vertex < 0 || (unsigned)dst_vertex >= graph->vertex_ct)
    return false;
  GraphEdge_LL_t *adjs = &(graph->vertices[src_vertex].adj_list);
  if (adjs->nmemb==0UL)
    return false;
  int diff;
  LL_FOREACH(LL_NODE_VAR_INITIALIZER(GraphEdge, node), node, adjs) {
    diff = dst_vertex - node->data.dst_idx;
    if (0 < diff) {
      continue;
    } else if (0 > diff) {
      return false;
    }
    // ELSE: diff==0 aka: target edge found
    node->data.weight = new_weight;
    return true;
  }
  return false;
}

void Graph_Close(Graph_t *graph) {
  size_t vct = graph->vertex_ct;
  GraphNode_t *curvert;
  GraphEdge_LL_t *adjs;
  for (unsigned i = 0; i < vct; ++i) {
    curvert = &(graph->vertices[i]);
    adjs = &(curvert->adj_list);
    LL_CLOSE(GraphEdge, adjs);
    assert(adjs->nmemb == 0UL && adjs->tail == adjs->head && adjs->head == NULL);
    if (graph->data_uniniter) {
      graph->data_uniniter(curvert->data);
    }
    free(curvert->data);
  }
  free(graph->vertices);
  BinaryTree_Destroy(graph->vertex_data_map);
  free(graph);
}


GraphEdge_LL_t *Graph_Get_Vertex_Adjacents(Graph_t *graph, int vertex_id) {
  if (!graph)
    return NULL;
  if ((unsigned)vertex_id >= graph->vertex_ct)
    return NULL;
  return &(graph->vertices[vertex_id].adj_list);
}

GraphNode_t *Graph_Get_Vertex(Graph_t *graph, const void *vertdata) {
  if (!graph || !vertdata)
    return NULL;
  
  uint8_t *vmap_entry = BinaryTree_Retrieve(graph->vertex_data_map, (void*)vertdata);
  if (!vmap_entry)
    return NULL;
  vmap_entry += graph->vertex_data_size;
  return &(graph->vertices[*(int*)vmap_entry]);
}
