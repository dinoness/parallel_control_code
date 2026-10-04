#ifndef _MYEIGEN_H
#define _MYEIGEN_H

#define radians(degress) ((degress) * 0.01745329251994329576923690768489)
#define degrees(radians) ((radians) * 57.295779513082320876798154814105)


#define M33_ONE(N)  mat3x3 N; do{mat3x3_one(&N);}while(0)
#define M33_ZERO(N) mat3x3 N; do{mat3x3_zero(&N);}while(0)


// 3向量
typedef struct {
    union {float x, r, s;};
    union {float y, g, t;};
    union {float z, b, p;};
}vec3;

vec3* vec3_set(vec3* out, float x, float y, float z);
vec3* vec3_copy(vec3* des, const vec3* src);
vec3* vec3_normalize(vec3* v);
vec3* vec3_normalize_copy(vec3* out, const vec3* v);
float vec3_length(const vec3* v);
float vec3_dot(const vec3* v1, const vec3* v2);
vec3* vec3_mult_v(vec3* left, float v);
vec3* vec3_mult_v_copy(vec3* out, const vec3* left, float v);
vec3* vec3_mult_v3_copy(vec3* out, const vec3* left, const vec3* right);
vec3* vec3_cross(vec3* left, const vec3* right);
vec3* vec3_cross_copy(vec3* out, const vec3* left, const vec3* right);
vec3* vec3_add(vec3* left, const vec3* right);
vec3* vec3_add_copy(vec3* out, const vec3* left, const vec3* right);

// 3x3矩阵
// 第一个指标是列，第二个指标是行
typedef struct{
    union
    {
     float m[3][3];
    };
} mat3x3;

mat3x3* mat3x3_diag(mat3x3* out, float v);
mat3x3* mat3x3_zero(mat3x3* out);
mat3x3* mat3x3_one(mat3x3* out);
vec3* col_vec_form_mat3x3_copy(vec3* out, mat3x3* m33, int col);
mat3x3* col_vec_to_mat3x3_copy(mat3x3* out, vec3* vec1, vec3* vec2, vec3* vec3);
vec3* mat3x3_mult_v3(const mat3x3* m33, vec3* v);
vec3* mat3x3_mult_v3_copy(vec3* out, const mat3x3* m33, const vec3* v);

#endif