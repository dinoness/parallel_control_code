#include "myeigen.h"
#include "zmcbuildin.h"
static float mysqrt(float v)
{
    return sqrt(v);
}

vec3* vec3_copy(vec3* des, const vec3* src)
{
    des->x = src->x;
    des->y = src->y;
    des->z = src->z;
    return des;
}


vec3* vec3_set(vec3* out, float x, float y, float z)
{
    out->x = x;
    out->y = y;
    out->z = z;
    return out;
}

float vec3_dot(const vec3* v1, const vec3* v2)
{
    vec3 out;
    vec3_mult_v3_copy(&out, v1, v2);
    return out.x + out.y + out.z;
}

vec3* vec3_mult_v3_copy(vec3* out, const vec3* left, const vec3* right)
{
    out->x = left->x * right->x;
    out->y = left->y * right->y;
    out->z = left->z * right->z;
    return out;
}

float vec3_length(const vec3* v)
{
    return mysqrt(vec3_dot(v,v));
}

vec3* vec3_normalize_copy(vec3* out, const vec3* v)
{
    return vec3_mult_v_copy(out, v, 1.f / mysqrt(vec3_dot(v,v)));
}
vec3* vec3_normalize(vec3* v)
{
    vec3 out;
    vec3_normalize_copy(&out, v);
    return vec3_copy(v, &out);
}

vec3* vec3_mult_v_copy(vec3* out, const vec3* left, float v)
{
    out->x = left->x * v;
    out->y = left->y * v;
    out->z = left->z * v;
    return out;
}

vec3* vec3_mult_v(vec3* left, float v)
{
    vec3 out;
    vec3_mult_v_copy(&out, left, v);
    return vec3_copy(left, &out);
}

vec3* vec3_cross_copy(vec3* out, const vec3* left, const vec3* right)
{
    out->x = left->y * right->z - right->y * left->z;
    out->y = left->z * right->x - right->z * left->x;
    out->z = left->x * right->y - right->x * left->y;
    return out;
}

vec3* vec3_cross(vec3* left, const vec3* right)
{
    vec3 out;
    vec3_cross_copy(&out, left, right);
    return vec3_copy(left, &out);
}

vec3* vec3_add_copy(vec3* out, const vec3* left, const vec3* right)
{
    out->x = left->x + right->x;
    out->y = left->y + right->y;
    out->z = left->z + right->z;
    return out;
}

vec3* vec3_add(vec3* left, const vec3* right)
{
   vec3 out;
   vec3_add_copy(&out, left, right);
   return vec3_copy(left,&out);
}




mat3x3* mat3x3_diag(mat3x3* out, float v)
{
    out->m[0][0] = v;
    out->m[0][1] = 0;
    out->m[0][2] = 0;
    out->m[1][0] = 0;
    out->m[1][1] = v;
    out->m[1][2] = 0;
    out->m[2][0] = 0;
    out->m[2][1] = 0;
    out->m[2][2] = v;
    return out;
}


mat3x3* mat3x3_one(mat3x3* out)
{
    return mat3x3_diag(out, 1.f);
}


mat3x3* mat3x3_zero(mat3x3* out)
{
    return mat3x3_diag(out, 0.f);
}


vec3* col_vec_form_mat3x3_copy(vec3* out, mat3x3* m33, int col)
{
    out->x = m33->m[col][0];
    out->y = m33->m[col][1];
    out->z = m33->m[col][2];
    return out;
}

mat3x3* col_vec_to_mat3x3_copy(mat3x3* out, vec3* vec1, vec3* vec2, vec3* vec3)
{
    out->m[0][0] = vec1->x;
    out->m[0][1] = vec1->y;
    out->m[0][2] = vec1->z;
    out->m[1][0] = vec2->x;
    out->m[1][1] = vec2->y;
    out->m[1][2] = vec2->z;
    out->m[2][0] = vec3->x;
    out->m[2][1] = vec3->y;
    out->m[2][2] = vec3->z;
    return out;
}

vec3* mat3x3_mult_v3_copy(vec3* out, const mat3x3* m33, const vec3* v)
{
    out->x = m33->m[0][0] * v->x + m33->m[1][0] * v->y + m33->m[2][0] * v->z;
	out->y = m33->m[0][1] * v->x + m33->m[1][1] * v->y + m33->m[2][1] * v->z;
	out->z = m33->m[0][2] * v->x + m33->m[1][2] * v->y + m33->m[2][2] * v->z;
    return out;
}

vec3* mat3x3_mult_v3(const mat3x3* m33, vec3* v)
{
    vec3 out;
    return vec3_copy(v, mat3x3_mult_v3_copy(&out, m33, v));
}