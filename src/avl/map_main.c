#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "avl_map.h"

struct _map_data_t {
  int content;
};
struct _map_key_t {
  char* content;
};
map_data_t* make_data(int i) {
  map_data_t* data = malloc(sizeof *data);
  data->content = i;
  return data;
}
map_key_t* make_key(char* key) {
  map_key_t* ret = malloc(sizeof *ret);
  ret->content = key;
  return ret;
}
char* data_as_string(map_data_t* data) {
  char* buf = malloc(20 * sizeof(char)); /* 20 char is enough to old 2**64. */
  sprintf(buf, "%d", data->content);
  return buf;
}
char* key_as_string(map_key_t* key) {
  return key->content;
}

int key_compare(map_key_t* k1, map_key_t* k2) {
  return strcmp(k1->content, k2->content);
}



int main () {

  srand(10);

  
  printf("First version:\n");
  map = avl_map_update(map, make_key("First"), make_data(1));
  map = avl_map_update(map, make_key("Second"), make_data(2));
  map = avl_map_update(map, make_key("Fourth"), make_data(3));
  map = avl_map_update(map, make_key("Fifth"), make_data(4));
  map = avl_map_update(map, make_key("Sixth"), make_data(5));
  avl_map_dump(map);
  printf("Now updating First, Second and Sixth:\n");
  map = avl_map_update(map, make_key("First"), make_data(19));
  map = avl_map_update(map, make_key("Second"), make_data(41));
  map = avl_map_update(map, make_key("Sixth"), make_data(25));
  avl_map_dump(map);
  avl_map_t* backup = map;
  printf("Now removing First, Fifth and Second\n");
  map_data_t* data = NULL;
  map = avl_map_remove(map, make_key("First"), &data);
  map = avl_map_remove(map, make_key("Fifth"), &data);
  map = avl_map_remove(map, make_key("Second"), &data);
  avl_map_dump(map);
  printf("Verification of immutability: The old map is now:\n");
  avl_map_dump(backup);

  printf("\n\n");
  
  return 0;
}
