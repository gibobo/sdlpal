//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Portions based on PalLibrary by Lou Yihua <louyihua@21cn.com>.
// Copyright (c) 2006-2007, Lou Yihua.
//
// Ported to C from C++ and modified for compatibility with Big-Endian
// by Wei Mingzhi <whistler_wmz@users.sf.net>.
//

#include "util.h"
#include <stdlib.h>
#include <string.h>

// 常數定義
#define MAX_TREE_NODES 641    // 總節點數 (0x280 + 1)
#define MAX_LEAF_NODES 321    // 葉節點數 (0x140 + 1)
#define ROOT_NODE_VALUE 0x280 // 樹根節點值
#define MAX_WEIGHT 0x8000     // 最大權重值
#define END_MARKER 0xfff      // 結束標記

// Huffman 樹節點結構
typedef struct HuffmanNode {
   unsigned short weight;      // 節點權重
   unsigned short value;       // 節點值
   struct HuffmanNode *parent; // 父節點
   struct HuffmanNode *left;   // 左子節點
   struct HuffmanNode *right;  // 右子節點
} HuffmanNode;

// Huffman 樹結構
typedef struct HuffmanTree {
   HuffmanNode *root;    // 根節點
   HuffmanNode **leaves; // 葉節點陣列
} HuffmanTree;

static unsigned char LOOKUP_TABLE_1[256] = {
    0x3f, 0x0b, 0x17, 0x03, 0x2f, 0x0a, 0x16, 0x00, 0x2e, 0x09, 0x15, 0x02, 0x2d, 0x01, 0x08, 0x00,
    0x3e, 0x07, 0x14, 0x03, 0x2c, 0x06, 0x13, 0x00, 0x2b, 0x05, 0x12, 0x02, 0x2a, 0x01, 0x04, 0x00,
    0x3d, 0x0b, 0x11, 0x03, 0x29, 0x0a, 0x10, 0x00, 0x28, 0x09, 0x0f, 0x02, 0x27, 0x01, 0x08, 0x00,
    0x3c, 0x07, 0x0e, 0x03, 0x26, 0x06, 0x0d, 0x00, 0x25, 0x05, 0x0c, 0x02, 0x24, 0x01, 0x04, 0x00,
    0x3b, 0x0b, 0x17, 0x03, 0x23, 0x0a, 0x16, 0x00, 0x22, 0x09, 0x15, 0x02, 0x21, 0x01, 0x08, 0x00,
    0x3a, 0x07, 0x14, 0x03, 0x20, 0x06, 0x13, 0x00, 0x1f, 0x05, 0x12, 0x02, 0x1e, 0x01, 0x04, 0x00,
    0x39, 0x0b, 0x11, 0x03, 0x1d, 0x0a, 0x10, 0x00, 0x1c, 0x09, 0x0f, 0x02, 0x1b, 0x01, 0x08, 0x00,
    0x38, 0x07, 0x0e, 0x03, 0x1a, 0x06, 0x0d, 0x00, 0x19, 0x05, 0x0c, 0x02, 0x18, 0x01, 0x04, 0x00,
    0x37, 0x0b, 0x17, 0x03, 0x2f, 0x0a, 0x16, 0x00, 0x2e, 0x09, 0x15, 0x02, 0x2d, 0x01, 0x08, 0x00,
    0x36, 0x07, 0x14, 0x03, 0x2c, 0x06, 0x13, 0x00, 0x2b, 0x05, 0x12, 0x02, 0x2a, 0x01, 0x04, 0x00,
    0x35, 0x0b, 0x11, 0x03, 0x29, 0x0a, 0x10, 0x00, 0x28, 0x09, 0x0f, 0x02, 0x27, 0x01, 0x08, 0x00,
    0x34, 0x07, 0x0e, 0x03, 0x26, 0x06, 0x0d, 0x00, 0x25, 0x05, 0x0c, 0x02, 0x24, 0x01, 0x04, 0x00,
    0x33, 0x0b, 0x17, 0x03, 0x23, 0x0a, 0x16, 0x00, 0x22, 0x09, 0x15, 0x02, 0x21, 0x01, 0x08, 0x00,
    0x32, 0x07, 0x14, 0x03, 0x20, 0x06, 0x13, 0x00, 0x1f, 0x05, 0x12, 0x02, 0x1e, 0x01, 0x04, 0x00,
    0x31, 0x0b, 0x11, 0x03, 0x1d, 0x0a, 0x10, 0x00, 0x1c, 0x09, 0x0f, 0x02, 0x1b, 0x01, 0x08, 0x00,
    0x30, 0x07, 0x0e, 0x03, 0x1a, 0x06, 0x0d, 0x00, 0x19, 0x05, 0x0c, 0x02, 0x18, 0x01, 0x04, 0x00};

static unsigned char LOOKUP_TABLE_2[16] = {
    0x08, 0x05, 0x06, 0x04, 0x07, 0x05, 0x06, 0x03, 0x07, 0x05, 0x06, 0x04, 0x07, 0x04, 0x05, 0x03};

// 從位元流中讀取單一位元
static inline int read_bit(const unsigned char *data, unsigned int pos) {
  return (data[pos >> 3] >> (pos & 7)) & 1;
}

// 調整 Huffman 樹
static void adjust_tree(HuffmanTree *tree, const unsigned short leaf_value) {
   HuffmanNode *node = tree->leaves[leaf_value];
   while (node->value != ROOT_NODE_VALUE) {
      HuffmanNode *next = node + 1;
      while (next->weight == node->weight && next->value <= ROOT_NODE_VALUE)
         next++;
      next--; // 回退到最後一個權重相同的節點

      if (next != node) {
         // 交換節點
         HuffmanNode *node_parent = node->parent;
         node->parent = next->parent;
         next->parent = node_parent;

         if (node->value > MAX_LEAF_NODES - 1) {
            node->left->parent = next;
            node->right->parent = next;
         } else
            tree->leaves[node->value] = next;

         if (next->value > MAX_LEAF_NODES - 1) {
            next->left->parent = node;
            next->right->parent = node;
         } else
            tree->leaves[next->value] = node;

         // 交換指標而非複製結構
         HuffmanNode temp = *node;
         *node = *next;
         *next = temp;
         node = next;
      }
      node->weight++;
      node = node->parent;
   }
   node->weight++;
}

// 構建 Huffman 樹
static void build_tree(HuffmanTree *tree, HuffmanNode *nodes, HuffmanNode **leaves) {
   tree->root = nodes;
   tree->leaves = leaves;

   // 初始化葉節點
   for (int i = 0; i < MAX_LEAF_NODES; i++)
      leaves[i] = &nodes[i];

   // 初始化所有節點
   for (int i = 0; i < MAX_TREE_NODES; i++) {
      nodes[i].value = i;
      nodes[i].weight = 1;
      nodes[i].parent = NULL;
      nodes[i].left = NULL;
      nodes[i].right = NULL;
   }

   nodes[ROOT_NODE_VALUE].parent = &nodes[ROOT_NODE_VALUE];
   for (int i = 0, j = MAX_LEAF_NODES; j < MAX_TREE_NODES; i += 2, j++) {
      nodes[j].left = &nodes[i];
      nodes[j].right = &nodes[i + 1];
      nodes[i].parent = nodes[i + 1].parent = &nodes[j];
      nodes[j].weight = nodes[i].weight + nodes[i + 1].weight;
   }
}

int YJ2_Decompress(const void *source, void *destination, int dest_size) {
   if (!source || !destination)
      return -1;

   const unsigned char *src = (const unsigned char *)source + 4;
   unsigned int length = *(const unsigned int *)source;
   if (length > dest_size)
      return -1;

   unsigned char *dest = (unsigned char *)destination;
   unsigned int bit_pos = 0, bytes_written = 0;

   // 初始化樹結構
   HuffmanNode nodes[MAX_TREE_NODES];
   HuffmanNode *leaves[MAX_LEAF_NODES];
   HuffmanTree tree;
   build_tree(&tree, nodes, leaves);

   while (1) {
      // 遍歷樹直到葉節點
      HuffmanNode *node = tree.root + ROOT_NODE_VALUE;
      while (node->value > MAX_LEAF_NODES - 1)
         node = read_bit(src, bit_pos++) ? node->right : node->left;

      // 檢查並調整權重
      if (tree.root[ROOT_NODE_VALUE].weight == MAX_WEIGHT) {
         for (int i = 0; i < MAX_LEAF_NODES; i++)
            if (leaves[i]->weight & 1)
               adjust_tree(&tree, i);
         for (int i = 0; i < MAX_TREE_NODES; i++)
            nodes[i].weight >>= 1;
      }

      unsigned short value = node->value;
      int copy_len = value - 0xfd;
      adjust_tree(&tree, value);

      // 處理解壓數據
      if (value > 0xff) {
         unsigned int temp = 0;
         for (int i = 0; i < 8; i++)
            temp |= read_bit(src, bit_pos++) << i;

         unsigned int lower_bits = temp & 0xff;
         int extra_bits = LOOKUP_TABLE_2[lower_bits & 0xf];
         for (int i = 8; i < extra_bits + 6; i++)
            temp |= read_bit(src, bit_pos++) << i;
         temp >>= extra_bits;

         unsigned int offset = (temp & 0x3f) | ((unsigned int)LOOKUP_TABLE_1[lower_bits] << 6);
         if (offset == END_MARKER)
            break;

         for (int i = 0; i < copy_len; i++)
           dest[bytes_written + i] = dest[bytes_written + i - offset - 1];
      } else {
         copy_len = 1;
         dest[bytes_written] =(unsigned char)value;
      }
      bytes_written += copy_len;
   }
   return length;
}