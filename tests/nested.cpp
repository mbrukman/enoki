/*
    tests/nested.cpp -- tests nested arrays and other fancy scalar types

    Enoki is a C++ template library that enables transparent vectorization
    of numerical kernels using SIMD instruction sets available on current
    processor architectures.

    Copyright (c) 2017 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a BSD-style
    license that can be found in the LICENSE.txt file.
*/

#include "test.h"

ENOKI_TEST(test01_string) { /* Arrays can be instantiated for all sorts of types */
    Array<std::string, 2> v1("Hello ", " How are ");
    Array<std::string, 2> v2("world!", "you?");

    assert(v1.x() == "Hello ");
    assert(to_string(v1) == "[Hello ,  How are ]");
    assert(to_string(v1 + v2) == "[Hello world!,  How are you?]");
    assert(hsum(v1 + v2) == "Hello world! How are you?");
    assert(hsum(v1 + "you!") == "Hello you! How are you!");
}

ENOKI_TEST(test02_float_array) {
    /* Scalar initialization */
    Array<float, 4> a(1.f);
    assert(to_string(a) == "[1, 1, 1, 1]");

    /* Value initialization */
    Array<float, 4> b(1.f, 2.f, 3.f, 4.f);
    assert(to_string(b) == "[1, 2, 3, 4]");
    assert(b.x() == 1.f && b.y() == 2.f && b.z() == 3.f && b.w() == 4.f);

    /* Copy initialization */
    Array<float, 4> c(b);
    assert(to_string(c) == "[1, 2, 3, 4]");

    /* Operations involving scalars (left) */
    assert(to_string(c + 1.f) == "[2, 3, 4, 5]");

    /* Operations involving scalars (right) */
    assert(to_string(1.f + c) == "[2, 3, 4, 5]");

    /* Binary operations */
    assert(to_string(c + c) == "[2, 4, 6, 8]");
}

ENOKI_TEST(test03_floatref_array) {
    float tmp1 = 1.f;
    Array<float, 4> tmp2(1.f, 2.f, 3.f, 4.f);

    /* Value initialization */
    Array<float&, 4> a(tmp1, tmp1, tmp1, tmp1);
    assert(to_string(a) == "[1, 1, 1, 1]");
    a.x() = 2.f;
    assert(to_string(a) == "[2, 2, 2, 2]");

    /* Reference an existing array */
    Array<float&, 4> b(tmp2);
    assert(to_string(b) == "[1, 2, 3, 4]");
    assert(to_string(a + b) == "[3, 4, 5, 6]");

    /* .. and reference it once more */
    Array<float&, 4> c(b);

    /* Convert back into a regular array */
    Array<float, 4> d(c);
    assert(to_string(d) == "[1, 2, 3, 4]");

    /* Operations involving scalars (left) */
    assert(to_string(c + 1.f) == "[2, 3, 4, 5]");

    /* Operations involving scalars (right) */
    assert(to_string(1.f + c) == "[2, 3, 4, 5]");

    /* Binary operations */
    assert(to_string(c + c) == "[2, 4, 6, 8]");
    assert(to_string(d + c) == "[2, 4, 6, 8]");
    assert(to_string(c + d) == "[2, 4, 6, 8]");

    c += c; c += d; c += 1.f;

    assert(to_string(c) == "[4, 7, 10, 13]");
}

ENOKI_TEST(test04_array_of_arrays) {
    using Vector4f = Array<float, 4>;
    using Vector4fP = Array<Vector4f, 2>;

    Vector4f a(1, 2, 3, 4);
    Vector4f b(1, 1, 1, 1);
    Vector4fP c(a, b);

    assert(to_string(c) == "[[1, 1], [2, 1], [3, 1], [4, 1]]");
    assert(to_string(c + c) == "[[2, 2], [4, 2], [6, 2], [8, 2]]");
    assert(to_string(c + c.x()) == "[[2, 2], [4, 3], [6, 4], [8, 5]]");
    assert(to_string(c + 1.f) == "[[2, 2], [3, 2], [4, 2], [5, 2]]");
    assert(to_string(1.f + c) == "[[2, 2], [3, 2], [4, 2], [5, 2]]");

    assert((std::is_same<scalar_t<Vector4fP>, Vector4f>::value));
    assert((std::is_same<base_scalar_t<Vector4fP>, float>::value));
}

ENOKI_TEST(test05_mask_types) {
    assert((std::is_same<mask_t<bool>, bool>::value));
    assert((std::is_same<scalar_t<float>, float>::value));
    assert((std::is_same<mask_t<Array<float, 1>>, Array<bool, 1>>::value));
    assert((std::is_same<scalar_t<Array<float, 1>>, float>::value));
}