/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

// http://en.cppreference.com/w/cpp/language/operator_assignment

namespace rdd {

enum AssignOp {
  ASSIGN,  // a  = b
  PLUS,    // a += b
  MINUS,   // a -= b
  TIMES,   // a *= b
  DIVIDE,  // a -= b
  AND,     // a &= b
  OR,      // a |= b
  XOR      // a ^= b
};

/**
 * return an assignment function: right op= left
 */
template<typename T>
inline void assignOp(const T& lhs, AssignOp op, T* rhs) {
  switch (op) {
    case ASSIGN: *rhs  = lhs; break;
    case PLUS:   *rhs += lhs; break;
    case MINUS:  *rhs -= lhs; break;
    case TIMES:  *rhs *= lhs; break;
    case DIVIDE: *rhs /= lhs; break;
    case AND:    *rhs &= lhs; break;
    case OR:     *rhs |= lhs; break;
    case XOR:    *rhs ^= lhs; break;
  }
}

} // namespace rdd
