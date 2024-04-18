/**
 * mad_mad_access_patterns
 * CS 341 - Spring 2024
 * collaborated with edwins3
 */
#include "tree.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses mmap to access the data.

  ./lookup2 <data_file> <word> [<word> ...]
*/

static char* file;
void binary_search(char *word, uint32_t offset) {
  if (offset == 0) {
    printNotFound(word);
    return;
  }
  BinaryTreeNode *temp = (BinaryTreeNode*)(file+offset);
  char buf[64];
  memset(buf,0,64);
  strncpy(buf,(char*)(temp+1),60);

  if(strcmp(word, buf) == 0) {
    printFound(word,temp->count,temp->price);
  } else if (strcmp(word, buf) < 0) {
    binary_search(word, temp->left_child);
  } else {
    binary_search(word, temp->right_child);
  }
}


int main(int argc, char **argv) {
  if (argc <=2) {
    printArgumentUsage();
    exit(1);
  }
  int fd = open(argv[1], O_RDONLY);
  if(fd == -1) {
    openFail(argv[1]);
    close(fd);
    exit(2);
  }
  struct stat s;
  stat(argv[1], &s);
  file = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (file == (void*) -1) {
    mmapFail(argv[1]);
    close(fd);
    exit(2);
  }
  char buf[BINTREE_ROOT_NODE_OFFSET+2];
  strncpy(buf, file,BINTREE_ROOT_NODE_OFFSET);
  if(strcmp("BTRE", buf)) {
    formatFail(argv[1]);
    close(fd);
    exit(2);
  }
  for(int i = 2; i < argc; i++){
    binary_search(argv[i],BINTREE_ROOT_NODE_OFFSET);
  }
  close(fd);
  return 0;
}