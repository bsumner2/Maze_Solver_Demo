/******************************************************************************\
|*************************** Author: Burt O Sumner ****************************|
|******** Copyright 2025 (C) Burt O Sumner | All Rights Reserved **************|
\******************************************************************************/
#ifndef _GRAPH_H_
#define _GRAPH_H_

#include "bstree.h"

#include "linked_list.h"
#include "gba_util_macros.h"
#include <assert.h>
#include <stdbool.h>
typedef void (*Data_Initializer_cb)(void *dst, const void *src);
typedef void (*Data_Uninitializer_cb)(void*);
typedef struct s_graph Graph_t;
typedef struct s_graphnode GraphNode_t;

struct s_graph {
  Data_Initializer_cb data_initer;
  Data_Uninitializer_cb data_uniniter;
  GraphNode_t *vertices;
  BinaryTree_t *vertex_data_map;
  size_t vertex_ct, vertex_data_size;
};


typedef struct s_graphedge {
  int dst_idx, weight;
} GraphEdge_t;

LL_DECL(GraphEdge_t, GraphEdge);

struct s_graphnode {
  int idx;
  void *data;
  GraphEdge_LL_t adj_list;
};
Graph_t *Graph_Init(Data_Initializer_cb initializer, Data_Uninitializer_cb uninitializer, Comparison_cb data_cmp_cb, size_t vertex_data_size);

bool Graph_Add_Vertex(Graph_t *graph, const void *vertex_data);

bool Graph_Add_Edge(Graph_t *graph, int src_vertex, int dst_vertex, int weight);

STAT_INLN bool Graph_Add_TwoWay_Edge(Graph_t *graph, int vertex_a, int vertex_b, int weight) {
  if (!Graph_Add_Edge(graph, vertex_a, vertex_b, weight))
    return false;
  // if add edge from a to b yielded true, then add edge from b to a MUST ALSO WORK. otherwise, force exit for testing
  assert(Graph_Add_Edge(graph, vertex_b, vertex_a, weight));
  return true;
}

GraphEdge_LL_t *Graph_Get_Vertex_Adjacents(Graph_t *graph, int vertex_id);

GraphNode_t *Graph_Get_Vertex(Graph_t *graph, const void *vertdata);
bool Graph_Update_Edge(Graph_t *graph, int src_vertex, int dst_vertex, int new_weight);
void Graph_Close(Graph_t *graph);


#endif  /* _GRAPH_H_ */
