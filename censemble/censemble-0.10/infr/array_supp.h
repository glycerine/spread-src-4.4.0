/**************************************************************/
/* ARRAY_SUPP.H */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/

#ifdef ARRAY_LENGTH_DEBUG
#define array_set_length(a, len) (a)->length = len
#else
#define array_set_length(a, len) NOP
#endif

#define ARRAY_CREATE(_name) \
array_of(_name) _name ## _array_create(len_t len) { \
    array_of(_name) a1 ; \
    a1 = sys_alloc(sizeof(*a1) + len * sizeof(a1->body[0])) ; \
    array_set_length(a1, len) ; \
    return a1 ; \
}

#define MATRIX_CREATE(_name) \
matrix_of(_name) _name ## _matrix_create(len_t len) { \
    matrix_of(_name) a1 ; \
    a1 = sys_alloc(sizeof(*a1) + len * sizeof(a1->body[0])) ; \
    array_set_length(a1, len) ; \
    return a1 ; \
}

#ifdef MINIMIZE_CODE
#define ARRAY_TO_STRING(_name)
#define ARRAY_PTR_TO_STRING(_name)
#else
#define ARRAY_TO_STRING(_name) \
string_t _name ## _array_to_string(array_of(_name) a0, len_t len) { \
    string_t ret ; \
    ofs_t ofs ; \
    string_array_t a1 ; \
    string_t tmp ; \
    assert_array_length(a0, len) ; \
    a1 = string_array_create(len) ; \
    for (ofs=0;ofs<len;ofs++) { \
      array_set(a1, ofs, _name ## _to_string(array_get(a0, ofs))) ; \
    } ; \
    tmp = util_string_concat("|", a1, len) ; \
    ret = sys_sprintf("[%s]", tmp) ; \
    string_free(tmp) ; \
    array_free(a1) ; \
    return ret ; \
}

#define ARRAY_PTR_TO_STRING(_name) \
string_t _name ## _array_to_string(array_of(_name) a0, len_t len) { \
    string_t ret ; \
    string_t tmp ; \
    ofs_t ofs ; \
    string_array_t a1 ; \
    assert_array_length(a0, len) ; \
    a1 = string_array_create(len) ; \
    for (ofs=0;ofs<len;ofs++) { \
      array_set(a1, ofs, _name ## _to_string(&array_get(a0, ofs))) ; \
    } ; \
    tmp = util_string_concat("|", a1, len) ; \
    ret = sys_sprintf("[%s]", tmp) ; \
    string_free(tmp) ; \
    for (ofs=0;ofs<len;ofs++) { \
      string_free(array_get(a1, ofs)) ; \
    } \
    array_free(a1) ; \
    return ret ; \
}
#endif

#define ARRAY_COPY(_name) \
array_of(_name) _name ## _array_copy(array_of(_name) a0, len_t len) { \
    array_of(_name) a1 ; \
    ofs_t ofs ; \
    assert_array_length(a0, len) ; \
    a1 = _name ## _array_create(len) ; \
    for (ofs=0;ofs<len;ofs++) { \
      array_set(a1, ofs, array_get(a0, ofs)) ; \
    } ; \
    return a1 ; \
}

#define ARRAY_MARSH(_name, _type) \
void marsh_ ## _name ## _array(marsh_t m, array_of(_type) v, len_t len) { \
    ofs_t i ; \
    assert(v) ; \
    assert_array_length(v, len) ; \
    for(i=0;i<len;i++) { \
	marsh_ ## _name(m, array_get(v, i)) ; \
    } \
    marsh_len(m, len) ; \
} \
\
void unmarsh_ ## _name ## _array(unmarsh_t m, array_of(_type) *ret, len_t len) { \
    array_of(_type) a ; \
    ofs_t i ; \
    len_t n ; \
    unmarsh_uint32(m, &n) ; \
    assert(n == len) ; \
    a = _name ## _array_create(len) ; \
    for(i=0;i<len;i++) { \
	_type ## _t v ; \
	unmarsh_ ## _name(m, &v) ; \
	array_set(a, n - i - 1, v) ; \
    } \
    *ret = a ; \
}

#define ARRAY_PTR_MARSH(_name, _type) \
void marsh_ ## _name ## _array(marsh_t m, const array_of(_type) v, len_t len) { \
    ofs_t i ; \
    assert(v) ; \
    assert_array_length(v, len) ; \
    for(i=0;i<len;i++) { \
	marsh_ ## _name(m, &array_get(v, i)) ; \
    } \
    marsh_len(m, len) ; \
} \
\
void unmarsh_ ## _name ## _array(unmarsh_t m, array_of(_type) *ret, len_t len) { \
    array_of(_type) a ; \
    ofs_t i ; \
    len_t n ; \
    unmarsh_len(m, &n) ; \
    assert(n == len) ; \
    a = _name ## _array_create(len) ; \
    for(i=0;i<len;i++) { \
	_type ## _t v ; \
	unmarsh_ ## _name(m, &v) ; \
	array_set(a, len - i - 1, v) ; \
    } \
    *ret = a ; \
}

#define ARRAY_FREE(_name) \
void _name ## _array_free(array_of(const _name) a, len_t len) { \
    ofs_t ofs ; \
    assert_array_length(a, len) ; \
    for (ofs=0;ofs<len;ofs++) { \
      _name ## _free(array_get(a, ofs)) ; \
    } ; \
    array_free(a) ; \
}
