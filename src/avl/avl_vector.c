
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "avl.h"
#include "avl_vector.h"

#define MAX(x,y) x < y ? y : x

struct _avl_vector_t {
  avl_tree* vector;
  int max_index;
  char* (*data_as_string)(void* data);
};

/*******************
 *   Boxing & co   *
 *******************/
typedef struct _avl_vector_data {
  void* data;
  int index;
} _avl_vector_data_t;

_avl_vector_data_t* make_vector_data(void* data, int index) {
  _avl_vector_data_t* ret = malloc(sizeof *ret);
  ret->data = data;
  ret->index = index;
  return ret;
}
_avl_vector_data_t* clone_vector_data(_avl_vector_data_t* data) {
    _avl_vector_data_t* ret = malloc(sizeof *ret);
    ret->data = data->data;
    ret->index = data->index;
    return ret;
}
int _vector_compare (void* d1, void* d2) {
  if (((_avl_vector_data_t*)d1)->index == (((_avl_vector_data_t*)d2)->index)) return 0;
  if (((_avl_vector_data_t*)d1)->index < (((_avl_vector_data_t*)d2)->index)) return -1;
  return 1;
}

/************************
 *   User side boxing   *
 ************************/
/* int box */
int_box_t* make_int_box(int i) {
  int_box_t* box = malloc(sizeof(*box));
  *box = i;
  return box;
}
char* int_box_as_string(void* data) {
  char* buf = malloc(20 * sizeof(char)); /* 20 char is enough to hold 2**64. */
  sprintf(buf, "%d", *((int_box_t*)data));
  return buf;
}

/* char* box */
string_box_t* make_string_box(char* str) {
  string_box_t* t = malloc(sizeof *t);
  *t = strdup(str);
  return t;
}
char* string_box_as_string(void* box) {
  return strdup(*((string_box_t*)box));
}


/*********************************
 * Vector manipulation functions *
 *********************************/
avl_vector_t* avl_vector_create(char* (*data_as_string)(void* data)) {
  avl_vector_t* ret = malloc(sizeof *ret);
  ret->vector = avl_make_empty_tree(_vector_compare);
  ret->max_index = -1;
  ret->data_as_string = data_as_string;
  return ret;
}


int avl_vector_size (const avl_vector_t* vec) {
  return vec->max_index + 1; /* +1 because the array is 0-indexed */
}


avl_vector_t* avl_vector_update(const avl_vector_t* vec, int index,
				void* data) {
  avl_vector_t* new = malloc(sizeof *new);
  if (index > vec->max_index) {
    new->max_index = index;
  } else {
    new->max_index = vec->max_index;
  }

  _avl_vector_data_t* boxed_data = make_vector_data(data, index);

  new->vector = avl_insert(vec->vector, boxed_data);
  new->data_as_string = vec->data_as_string;
  
  return new;
}
avl_vector_t* avl_vector_update_mutable(avl_vector_t* vec, int index,
					void* data) {
  avl_vector_t* tmp = avl_vector_update(vec,index,data);
  avl_vector_unref(vec);
  return tmp;
}

void* avl_vector_lookup(const avl_vector_t* vec, int index) {
  _avl_vector_data_t* tmp = make_vector_data(NULL, index);
  _avl_vector_data_t* data = avl_search(vec->vector, tmp);
  free(tmp);
  if (data) {
    return data->data;
  } else {
    return NULL;
  }
}

avl_vector_t* avl_vector_push(const avl_vector_t* vec, void* data) {
  int index = vec->max_index + 1;
  _avl_vector_data_t* boxed_data = make_vector_data(data, index);

  avl_vector_t* new = malloc(sizeof *new);
  new->max_index = index;
  new->vector = avl_insert(vec->vector, boxed_data);
  new->data_as_string = vec->data_as_string;

  return new;
}
avl_vector_t* avl_vector_push_mutable(avl_vector_t* vec, void* data) {
  avl_vector_t* tmp = avl_vector_push(vec,data);
  avl_vector_unref(vec);
  return tmp;
}

avl_vector_t* avl_vector_pop(const avl_vector_t* vec, void** data) {
  int index = vec->max_index;
  if (index >= 0) {
    _avl_vector_data_t* tmp = make_vector_data(NULL, index);

    avl_vector_t* new = malloc(sizeof *new);
    new->max_index = index - 1;
    new->data_as_string = vec->data_as_string;

    void* return_data = NULL;

    new->vector = avl_remove(vec->vector, tmp, &return_data);
    free(tmp);
  
    if (return_data) {
      *data = ((_avl_vector_data_t*)return_data)->data;
    } else {
      *data = NULL;
    }
    return new;
  } else { /* empty vector */
    avl_vector_t* new = avl_vector_create(vec->data_as_string);
    *data = NULL;
    return new;
  }
}
avl_vector_t* avl_vector_pop_mutable(avl_vector_t* vec, void** data) {
  avl_vector_t* tmp = avl_vector_pop(vec, data);
  avl_vector_unref(vec);
  return tmp;
}


void _merge_vector_internal(avl_tree* ret, avl_node* orig, int shift) {
  if (orig != NULL) {
    _avl_vector_data_t* data =
      make_vector_data(((_avl_vector_data_t*)orig->data)->data,
		    shift + ((_avl_vector_data_t*)orig->data)->index);
    
    avl_insert_mutable(ret, data);
    _merge_vector_internal(ret, orig->sons[0], shift);
    _merge_vector_internal(ret, orig->sons[1], shift);
  }
}


avl_vector_t* avl_vector_merge (const avl_vector_t* vec_front,
				const avl_vector_t* vec_tail) {
  avl_vector_t* new = avl_vector_create(vec_front->data_as_string);
  new->data_as_string = vec_front->data_as_string;
  _merge_vector_internal(new->vector, vec_front->vector->root, 0);
  _merge_vector_internal(new->vector, vec_tail->vector->root,
			 vec_front->max_index+1);
  new->max_index = vec_front->max_index + vec_tail->max_index + 1;
  return new;
}
avl_vector_t* avl_vector_merge_mutable(avl_vector_t* vec_front,
				       avl_vector_t* vec_tail) {
  avl_vector_t* tmp = avl_vector_merge(vec_front, vec_tail);
  avl_vector_unref(vec_front);
  avl_vector_unref(vec_tail);
  return tmp;
}


void _split_vector_internal(avl_node* node, int index,
			    avl_vector_t* vec_out1, avl_vector_t* vec_out2) {
  if (node) {
    _avl_vector_data_t* data = clone_vector_data(node->data);
    if (data->index <= index) {
      avl_insert_mutable(vec_out1->vector, data);
      vec_out1->max_index = MAX(vec_out1->max_index, data->index);
    } else {
      data->index -= index+1;
      avl_insert_mutable(vec_out2->vector, data);
      vec_out2->max_index = MAX(vec_out2->max_index, data->index);
    }
    _split_vector_internal(node->sons[0], index, vec_out1, vec_out2);
    _split_vector_internal(node->sons[1], index, vec_out1, vec_out2);
  }
}

/* split ==> [0..index] [index+1..end] */
int avl_vector_split(const avl_vector_t* vec_in, int index,
		     avl_vector_t** vec_out1, avl_vector_t** vec_out2) {
  *vec_out1 = avl_vector_create(vec_in->data_as_string);
  *vec_out2 = avl_vector_create(vec_in->data_as_string);
  if (vec_in->max_index >= 0) {
    _split_vector_internal(vec_in->vector->root, index, *vec_out1, *vec_out2);
    return 1;
  } else {
    return 0;
  }
}
int avl_vector_split_mutable(avl_vector_t* vec_in, int index,
			     avl_vector_t** vec_out1, avl_vector_t** vec_out2) {
  int ret = avl_vector_split(vec_in, index, vec_out1, vec_out2);
  avl_vector_unref(vec_in);
  return ret;
}

void avl_vector_unref(avl_vector_t* vec) {
  if (vec) {
    avl_erase_tree(vec->vector);
    free(vec);
  }
}

void avl_vector_dump(const avl_vector_t* vec) {
  printf("[ ");
  for (int i = 0; i <= vec->max_index; i++) {
    void* data = avl_vector_lookup(vec, i);
    if (data) {
      char* data_string = (*vec->data_as_string)(data);
      printf("%s, ", data_string);
      free(data_string);
    } else {
      printf("_, ");
    }
  }
  if (vec->max_index >= 0) {
    printf("\b\b ");
  }
  printf("]\n");
}
