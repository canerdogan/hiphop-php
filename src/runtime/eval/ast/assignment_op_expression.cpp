/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010- Facebook, Inc. (http://www.facebook.com)         |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#include <runtime/eval/ast/assignment_op_expression.h>
#include <runtime/eval/ast/lval_expression.h>
#include <runtime/eval/strict_mode.h>
#include <runtime/eval/ast/variable_expression.h>
#include <util/parser/hphp.tab.hpp>

namespace HPHP {
namespace Eval {
///////////////////////////////////////////////////////////////////////////////

AssignmentOpExpression::AssignmentOpExpression(EXPRESSION_ARGS, int op,
                                               LvalExpressionPtr lhs,
                                               ExpressionPtr rhs)
  : Expression(KindOfAssignmentOpExpression, EXPRESSION_PASS),
  m_op(op), m_lhs(lhs), m_rhs(rhs) {}

Expression *AssignmentOpExpression::optimize(VariableEnvironment &env) {
  Eval::optimize(env, m_rhs);
  return NULL;
}

Variant AssignmentOpExpression::eval(VariableEnvironment &env) const {
  Variant rhs(m_rhs->eval(env));
  if (m_lhs->isKindOf(Expression::KindOfVariableExpression)) {
    Variant &lhs = static_cast<VariableExpression*>(m_lhs.get())->getRef(env);
    if (m_op == '=') return lhs.assignVal(rhs);
    Variant::TypedValueAccessor lhsAcc = lhs.getTypedAccessor();
    DataType lhsType = Variant::GetAccessorType(lhsAcc);
    Variant::TypedValueAccessor rhsAcc = rhs.getTypedAccessor();
    DataType rhsType = Variant::GetAccessorType(rhsAcc);
    if (lhsType == HPHP::KindOfInt64 && rhsType == HPHP::KindOfInt64) {
      int64 &lv = *Variant::GetInt64Data(lhsAcc);
      int64 rv = Variant::GetInt64(rhsAcc);
      switch (m_op) {
      case T_PLUS_EQUAL:     return lv += rv;
      case T_MINUS_EQUAL:    return lv -= rv;
      case T_MUL_EQUAL:      return lv *= rv;
      case T_AND_EQUAL:      return lv &= rv;
      case T_OR_EQUAL:       return lv |= rv;
      case T_XOR_EQUAL:      return lv ^= rv;
      case T_SL_EQUAL:       return lv <<= rv;
      case T_SR_EQUAL:       return lv >>= rv;
      default:               break;
      }
    }
    if (RuntimeOption::EnableStrict) {
      if (!VariableExpression::CheckCompatibleAssignment(lhs, rhs)) {
        throw_strict(TypeVariableChangeException(
                     location_to_string(m_lhs->loc())),
                     StrictMode::StrictHardCore);
      }
    }
    return m_lhs->LvalExpression::setOpVariant(lhs, m_op, rhs);
  }
  return (m_op == '=') ? m_lhs->set(env, rhs) : m_lhs->setOp(env, m_op, rhs);
}

void AssignmentOpExpression::dump(std::ostream &out) const {
  m_lhs->dump(out);
  out << " ";
  switch (m_op) {
  case '=':            out << "=";   break;
  case T_PLUS_EQUAL:   out << "+=";  break;
  case T_MINUS_EQUAL:  out << "-=";  break;
  case T_MUL_EQUAL:    out << "*=";  break;
  case T_DIV_EQUAL:    out << "/=";  break;
  case T_CONCAT_EQUAL: out << ".=";  break;
  case T_MOD_EQUAL:    out << "%=";  break;
  case T_AND_EQUAL:    out << "&=";  break;
  case T_OR_EQUAL:     out << "|=";  break;
  case T_XOR_EQUAL:    out << "^=";  break;
  case T_SL_EQUAL:     out << "<<="; break;
  case T_SR_EQUAL:     out << ">>="; break;
  default:
    ASSERT(false);
  }
  out << " ";
  m_rhs->dump(out);
}

Variant AssignmentOpExpression::refval(VariableEnvironment &env,
    int strict /* = 2 */) const {
  return strongBind(eval(env));
}

///////////////////////////////////////////////////////////////////////////////
}
}

