/*
 * bus-scan.c -- Incremental bus scan based on self ID packets
 *
 * Copyright (C) 2003 Kristian Høgsberg <krh@bitplanet.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>
#include <asm/byteorder.h>

#define PORT_NOT_PRESENT  0x0
#define PORT_DISCONNECTED 0x1
#define PORT_PARENT       0x2
#define PORT_CHILD        0x3

#define SELF_ID_MORE_PACKETS 0x00000001
#define SELF_ID_LINK_ON      0x00400000

#define SPEED_100  0x00
#define SPEED_200  0x01
#define SPEED_400  0x02
#define SPEED_BPHY 0x03
#define SPEED_800  0x03
#define SPEED_1600 0x04
#define SPEED_3200 0x05

struct device {
  struct device *next;
  unsigned long long guid;
  int ref_count;
  nodeid_t node_id;
};

struct device *new_device_list;

struct device *device_create(nodeid_t node_id)
{
  struct device *device;

  device = malloc(sizeof *device);
  memset(device, 0, sizeof *device);
  device->next = new_device_list;
  new_device_list = device;
  device->ref_count = 1;
  device->node_id = node_id;
  
  printf("created new device (%p)\n", device);


  return device;
}

void device_init(struct device *device, raw1394handle_t handle)
{
  quadlet_t guid_low, guid_high;

  raw1394_read(handle, device->node_id | 0xffc0, CSR_REGISTER_BASE + 0x40c,
	       sizeof guid_high, &guid_high);
  raw1394_read(handle, device->node_id | 0xffc0, CSR_REGISTER_BASE + 0x410,
	       sizeof guid_low, &guid_low);

  device->guid = ((unsigned long long) __be32_to_cpu(guid_high) << 32) | 
    __be32_to_cpu(guid_low);

  printf("initialized device (%p), guid=0x%016llx\n", device, device->guid);
}

void device_unref(struct device *device)
{
  printf("unreffing device, guid=0x%016llx\n", device->guid);

  device->ref_count--;
  if (device->ref_count == 0)
    free(device);
}


struct node {
  unsigned short node_id;
  struct port *ports;
  unsigned active_port_count;
  unsigned port_count;
  unsigned link_on : 1;
  unsigned b_path : 1;
  unsigned phy_speed : 2; /* As in the self ID packet. */
  unsigned max_speed : 3; /* Minimum of all phy-speeds and port speeds
			   * on the path from the local node to this
			   * node. */

  struct device *device;
};

struct port {
  struct node *node;
  char index;
  unsigned speed : 3; /* S100, S200, ... S3200 */
};

struct graph {
  struct port ports[126];
  struct node nodes[63];
  int local_id;
};

static quadlet_t *
count_ports(quadlet_t *sid, int *total_port_count,
	    int *active_port_count, int *child_port_count)
{
  int port_type, shift;

  *total_port_count = 0;
  *active_port_count = 0;
  *child_port_count = 0;

  shift = 6;

  while (1) {
    port_type = (*sid >> shift) & 0x03;
    switch (port_type) {
    case PORT_CHILD:
      (*child_port_count)++;
    case PORT_PARENT:
      (*active_port_count)++;
    case PORT_DISCONNECTED:  
      (*total_port_count)++;
    case PORT_NOT_PRESENT:
      break;
    }

    shift -= 2;
    if (shift == 0) {
      if (!(*sid & SELF_ID_MORE_PACKETS))
	return sid + 1;
      shift = 16;
      sid++;
    }
  }
}

int get_port_type(quadlet_t *sid, int port_index)
{
  int index, shift;

  index = (port_index + 5) / 8;
  shift = 16 - ((port_index + 5) & 7) * 2;
  return (sid[index] >> shift) & 0x03;
}

struct stack_element {
  int parent_index;
  struct node *node;
};

void
build_graph(struct graph *graph, quadlet_t *self_ids,
	    int self_id_count, nodeid_t local_id)
{
  struct port *next_port;
  struct node *node;
  struct stack_element stack[16], *child, *sp;
  quadlet_t *sid, *next_sid;
  int i, port_index, parent_index;
  int port_count, active_port_count, child_port_count;

  parent_index = 0; /* Just to shut up gcc. */
  sp = stack;
  sid = self_ids;
  next_port = graph->ports;
  node = graph->nodes;
  graph->local_id = local_id;

  while (sid < self_ids + self_id_count) {
    next_sid = count_ports(sid, &port_count,
			   &active_port_count, &child_port_count);

    node->node_id = node - graph->nodes;
    node->device = NULL;
    node->link_on = (*sid & SELF_ID_LINK_ON) > 0;
    node->port_count = port_count;
    node->active_port_count = active_port_count;
    node->ports = next_port;
    next_port += active_port_count;
    child = sp - child_port_count;
    
    for (port_index = 0, i = 0; port_index < port_count; port_index++) {
      switch (get_port_type(sid, port_index)) {
      case PORT_PARENT:
	/* Who's your daddy?  We dont know the index of the parent
	 * node at this time, so we just remember the index in the
	 * node->ports array where the parent node should be.  Later,
	 * when we handle the parent node, we fix up the reference.
	 */
	node->ports[i].index = port_index;
	parent_index = i++;
	break;

      case PORT_CHILD: 
	node->ports[i].index = port_index;
	node->ports[i++].node = child->node;
	/* Fix up parent reference for this child node. */
	child->node->ports[child->parent_index].node = node;
	child++;
	break;
      }
    }

    /* Pop the child nodes from the stack and push this new node. */
    sp -= child_port_count;
    sp->parent_index = parent_index;
    sp->node = node;
    sp++;

    sid = next_sid;
    node++;
  }
}

void
report_lost_nodes(struct node *node, struct node *prev)
{
  struct node *child;
  int i;

  printf("lost node, node_id=0x%x\n", node->node_id);

  if (node->device)
    device_unref(node->device);

  for (i = 0; i < node->active_port_count; i++) {
    child = node->ports[i].node;
    if (child != prev)
      report_lost_nodes(child, node);
  }
}

void
report_found_nodes(struct node *node, struct node *prev)
{
  struct node *child;
  int i;

  printf("found node, node_id=0x%x, link %s\n",
	 node->node_id, node->link_on ? "active" : "not active");

  /* We create the device here, but defer the initialization for
   * later, since this would run in interrupt context. */
  if (node->link_on)
    node->device = device_create(node->node_id);

  for (i = 0; i < node->active_port_count; i++) {
    child = node->ports[i].node;
    if (child != prev)
      report_found_nodes(child, node);
  }
}

void
compare_graphs(struct node *node0, struct node *prev0,
	       struct node *node1, struct node *prev1)
{
  static struct port bogus_port = { NULL, 100, 0 };
  struct port *port0, *port1;
  int i, j;

  printf("updating device 0x%016llx, node id: %d -> %d\n",
	 node0->device ? node0->device->guid : 0,
	 node0->node_id, node1->node_id);
  node1->device = node0->device;
  if (node1->device)
    node1->device->node_id = node1->node_id;

  i = 0;
  j = 0;

  while (i < node0->active_port_count || j < node1->active_port_count) {
    if (i < node0->active_port_count)
      port0 = &node0->ports[i];
    else
      port0 = &bogus_port;

    if (j < node1->active_port_count) 
      port1 = &node1->ports[j];
    else
      port1 = &bogus_port;

    if (port0->index < port1->index) {
      report_lost_nodes(port0->node, node0);
      i++;
    }
    else if (port0->index > port1->index) {
      report_found_nodes(port1->node, node1);
      j++;
    }
    else {
      if (port0->node != prev0)
	compare_graphs(port0->node, node0, port1->node, node1);
      i++;
      j++;
    }
  }
}

/*
 * Compute the longest path by recursively traversing the topology
 * tree.  For any node, the longest path either passes through the
 * node, is fully contained in one of its subtrees, or neither.  Thus,
 * this function
 * 
 * 1) returns the lenght of the path to the most distant leaf, and 
 * 2) updates *max_length to be the length of the longest path
 *    contained in the subtree, if this length exceeds the previous value
 *    of *max_length.
 *
 * To implement this, we call recursively on all subtrees.  We now
 * have the length of the longest path contained in any of the
 * subtrees, which we compare with the length of the longest path
 * through this node.
 *
 * Computing this function at the root of the tree gives us the
 * longest path in the topology tree.  Complexity is linear in the
 * number of nodes.
 */

static int
get_longest_path(struct node *node, struct node *prev, int *max_length)
{
  int i, d, leaf_distance0, leaf_distance1;

  leaf_distance0 = 0;
  leaf_distance1 = 0;

  for (i = 0; i < node->active_port_count; i++) {
    if (node->ports[i].node == prev)
      continue;

    d = get_longest_path(node->ports[i].node, node, max_length) + 1;
    if (d > leaf_distance0) {
      leaf_distance1 = leaf_distance0;
      leaf_distance0 = d;
    }
    else if (d > leaf_distance1)
      leaf_distance1 = d;
  }

  d = leaf_distance0 + leaf_distance1;
  if (d > *max_length)
    *max_length = d;

  return leaf_distance0;
}

static inline int
min(int a, int b)
{
  return a < b ? a : b;
}

static void
compute_speed_map(struct node *node, struct node *prev,
		  int max_speed, int b_path)
{
  int i, max_port_speed;

  if (node->phy_speed != SPEED_BPHY) {
    b_path = 0;
    max_speed = min(max_speed, node->phy_speed);
  }
  else {
    for (i = 0; i < node->active_port_count; i++) {
      if (node->ports[i].node == prev) {
	max_speed = min(max_speed, node->ports[i].speed);
	break;
      }
    }
  }

  node->max_speed = max_speed;
  node->b_path = b_path;

  for (i = 0; i < node->active_port_count; i++) {
    if (node->ports[i].node == prev)
      continue;

    if (node->phy_speed == SPEED_BPHY)
      max_port_speed = min(max_speed, node->ports[i].speed);
    else
      max_port_speed = max_speed;

    compute_speed_map(node->ports[i].node, node, max_port_speed, b_path);
  }
}

static void
get_graph_metrics(struct graph *graph)
{
  int max_path_length;
  struct node *local_node;

  local_node = &graph->nodes[graph->local_id];

  /* It doesn't matter which node we use for root when computing the
   * max path length.  */
  max_path_length = 0;
  get_longest_path(local_node, NULL, &max_path_length);
  printf("max path length: %d\n", max_path_length);

  compute_speed_map(local_node, NULL, SPEED_3200, 1);

  /* power management */
}

void
print_graph(struct graph *graph, struct node *node, struct node *prev,
	    int indent)
{
  static const char space[] = "          ";
  const char *s;
  int i;

  s = space + sizeof(space) - 1 - indent;
  printf("%snode %d, ports %d, active ports %d", 
	 s, node->node_id, node->port_count, node->active_port_count);
  if (node->device != NULL)
    printf(" (guid 0x%016llx)\n", node->device->guid);
  else
    printf(" (no guid)\n");

  for (i = 0; i < node->active_port_count; i++)
    if (node->ports[i].node != prev)
      print_graph(graph, node->ports[i].node, node, indent + 2);
}

struct graph graphs[2];
int current_graph;

int update_topology(raw1394handle_t handle)
{
  struct graph *graph, *other;
  nodeid_t local_id;
  quadlet_t tm_header[3], self_ids[0x100];
  int self_id_count, i;

  local_id = raw1394_get_local_id(handle);
  
  raw1394_read(handle, local_id, CSR_REGISTER_BASE + CSR_TOPOLOGY_MAP,
	       sizeof tm_header, tm_header);

  self_id_count = __be32_to_cpu(tm_header[2]) & 0xffff;
  raw1394_read(handle, local_id,
	       CSR_REGISTER_BASE + CSR_TOPOLOGY_MAP + sizeof tm_header,
	       self_id_count * 4, self_ids);

  printf("bus reset, new local node id %d, read %d self ids:\n",
	 local_id & 0x3f, self_id_count);
  for (i = 0; i < self_id_count; i++) {
    self_ids[i] = __be32_to_cpu(self_ids[i]);
    printf("  %d: 0x%08x\n", i, self_ids[i]);
  }
  printf("\n");

  graph = &graphs[current_graph];
  other = &graphs[current_graph ^ 1];
  current_graph ^= 1;

  build_graph(graph, self_ids, self_id_count, local_id & 0x3f);
  get_graph_metrics(graph);

  if (other->local_id != -1)
    compare_graphs(other->nodes + other->local_id, NULL,
		   graph->nodes + graph->local_id, NULL);
  else
    report_found_nodes(graph->nodes + graph->local_id, NULL);

  printf("new topology:\n");
  print_graph(graph, graph->nodes + graph->local_id, NULL, 0);

  return 0;
}

int bus_reset_handler(raw1394handle_t handle, unsigned int generation)
{
  raw1394_update_generation(handle, generation);
  update_topology(handle);
}

int main(int argc, char *argv[])
{
  raw1394handle_t handle;
  struct device *device;

  printf("incremental bus scan - userspace demo code.\n"
	 "sizeof graphs: %d bytes - "
	 "this data structure should be added to hpsb_host\n\n",
	 sizeof graphs);

  handle = raw1394_new_handle();
  if (handle == NULL) {
    perror("raw1394_new_handle");
    return EXIT_FAILURE;
  }

  if (raw1394_set_port(handle, 0) < 0) {
    perror("raw1394_set_port");
    return EXIT_FAILURE;
  }

  raw1394_set_bus_reset_handler(handle, bus_reset_handler);

  graphs[1].local_id = -1;

  update_topology(handle);

  while (1) {
    /* Process new devices found during last topology compare.  This
     * part should run in a kernel thread, while the topology compare
     * should run from an interrupt routine. */
    while (new_device_list != NULL) {
      device = new_device_list;
      new_device_list = device->next;
      device_init(device, handle);
    }

    printf("\n\n");

    raw1394_loop_iterate(handle);
  }

  return 0;
}
