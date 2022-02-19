/**
 * vector
 */
#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>

struct sstring {
    vector * vector;
    size_t len;
};

sstring *cstr_to_sstring(const char *input) {
    // your code goes here
    sstring * s = malloc(sizeof(sstring));
    vector * v = char_vector_create();
    const char * ptr = input;
    while (*ptr)
    {
        vector_push_back(v, (void *) ptr);
        ptr++;
    }
    s->vector = v;
    s->len = vector_size(v);
    return s;
}

char *sstring_to_cstr(sstring *input) {
    // your code goes here
    if(input->len == 0) return NULL;
    char* result = malloc(input->len + 1);
    for(size_t i = 0; i < input->len; i++) {
        result[i] = *((char * )vector_get(input->vector, i));
    }
    result[input->len] = '\0';
    return result;
}

int sstring_append(sstring *this, sstring *addition) {
    // your code goes here
    for(size_t i = 0; i < vector_size(addition->vector); i++) {
        vector_push_back(this->vector, vector_get(addition->vector, i));
    }
    this->len = vector_size(this->vector);
    return vector_size(this->vector);
}

vector *sstring_split(sstring *this, char delimiter) {
    // your code goes here
    size_t len = this->len;
    vector* result = string_vector_create();
    vector* cur = char_vector_create();

    for (size_t i = 0; i < len; i++)
    {
        char * c = vector_get(this->vector, i);
        if (*c == delimiter)
        {
            char * string  =calloc(vector_size(cur) + 1, sizeof(char));
            for (size_t i = 0; i < vector_size(cur); i++)
            {
                string[i] = *((char*) vector_get(cur, i));
            }
            vector_clear(cur);
            vector_push_back(result, (void*) string);
            free(string);
        } else {
            vector_push_back(cur, c);
        }
    }
    //push the last part
    char * string  =calloc(vector_size(cur) + 1, sizeof(char));
    for (size_t i = 0; i < vector_size(cur); i++)
    {
        string[i] = *((char*) vector_get(cur, i));
    }
    vector_push_back(result, (void*) string);
    free(string);
    vector_destroy(cur);
    return result;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here
    size_t t_index = 0;
    int replace = 0;
    size_t r_index = offset;
    for(size_t i = offset; i < this->len; i++) {
        if (*((char*) vector_get(this->vector, i)) == target[t_index]) {
            t_index++;
            if(t_index + 1 == strlen(target)) {
                replace = 1;
                break;
            }
        } else {
            t_index = 0;
            r_index = i + 1;
        }
    }
    if(replace) {
        for (size_t i = 0; i < strlen(substitution); i++) {
            //need to shift by r index
            size_t i_act = i + r_index;
            if(i >= strlen(target)) {
                //printf("inserting %c at index %zu\n",*((char*)(substitution + i)), i_act);
                vector_insert(this->vector, i_act , (void*)(substitution + i));
            } else {
                //printf("changing the original %c to %c at index  %zu \n", *((char*) vector_get(this->vector, i_act)), substitution[i], i_act);
                vector_set(this->vector, i_act, (char *) (substitution + i));
            }

        }
        if (strlen(target) > strlen(substitution))
        {
            size_t index = strlen(substitution) + r_index;
            for (size_t i = strlen(substitution); i < strlen(target); i++)
            {
             //need to shift by r index
             vector_erase(this->vector, index);
            }
        }
        //need to update length
        this->len = vector_size(this->vector);
        return 0;
    }
    return -1;
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
    char* result = malloc(end - start + 1);
    for (size_t i = start; i < (size_t)end; i++)
    {
        result[i - start] = *((char*) vector_get(this->vector, i));
    }
    result[end - start] = '\0';
    return result;
}

void sstring_destroy(sstring *this) {
    // your code goes here
    vector_destroy(this->vector);
    free(this);
    this = NULL;
}
