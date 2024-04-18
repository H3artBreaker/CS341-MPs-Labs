/**
 * mad_mad_access_patterns
 * CS 341 - Spring 2024
 * collaborated with edwins3
 */
#include "tree.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses fseek() and fread() to access the data.

  ./lookup1 <data_file> <word> [<word> ...]
*/


static FILE* file;
void binary_search(char *word, uint32_t offset) {
  if (offset == 0) {
    printNotFound(word);
    return;
  }
  BinaryTreeNode* temp = calloc(1,sizeof(BinaryTreeNode));
  fseek(file, offset, SEEK_SET);
  fread(temp, 1, sizeof(BinaryTreeNode), file);
  fseek(file, offset+ sizeof(BinaryTreeNode), SEEK_SET);
  char buf[64];
  fread(buf,1,63,file);
  if(strcmp(word, buf) == 0) {
    printFound(word,temp->count,temp->price);
    return;
  } else if (strcmp(word, buf) < 0) {
    binary_search(word, temp->left_child);
  } else {
    binary_search(word, temp->right_child);
  }
  free(temp);
}


int main(int argc, char **argv) {
  if (argc <=2) {
    printArgumentUsage();
    exit(1);
  }
  file = fopen(argv[1], "rb");
  if(!file) {
    openFail(argv[1]);
    exit(2);
  }
  char buf[BINTREE_ROOT_NODE_OFFSET + 1];
  buf[BINTREE_ROOT_NODE_OFFSET ]= 0;
  fread(buf,1,BINTREE_ROOT_NODE_OFFSET,file);
  if(strcmp("BTRE",buf)) {
    formatFail(argv[1]);
    fclose(file);
    exit(2);
  }

  for(int i = 2; i < argc; i++){
    binary_search(argv[i],BINTREE_ROOT_NODE_OFFSET);
  }
  fclose(file);
  return 0;
}