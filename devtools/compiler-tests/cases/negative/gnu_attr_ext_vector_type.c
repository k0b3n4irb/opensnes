// The clang spelling of a vector type, refused for the reason vector_size is.
typedef short v4 __attribute__((ext_vector_type(4)));
unsigned int n = sizeof(v4);
