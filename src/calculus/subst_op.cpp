#include <subst_op.h>
#include "calculus_internal.h"

namespace calculus {

ExpressionPtr SubstOp::Simplify() {
    return target_->Simplify()->Substitute(var_name_, value_->Simplify());
}

ExpressionPtr SubstOp::TakeDerivative(char var_name) {
    return Simplify()->TakeDerivative(var_name);
}

ExpressionPtr SubstOp::Call(const std::vector<ExpressionPtr>& args) {
    return Simplify()->Call(args);
}

ExpressionPtr SubstOp::Substitute(char var_name, const ExpressionPtr& value) {
    return Simplify()->Substitute(var_name, value);
}

void SubstOp::Print(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << '(';
    }
    target_->Print(out, kPostfixOpPriorityLevel);
    out << '[' << var_name_ << " = ";
    value_->Print(out, kSumPriorityLevel);
    out << ']';
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << ')';
    }
}

void SubstOp::TexDump(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << "\\left(";
    }
    out << "\\left.";
    target_->TexDump(out, kPostfixOpPriorityLevel);
    out << "\\right|_{" << var_name_ << " = ";
    value_->TexDump(out, kSumPriorityLevel);
    out << "}";
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << "\\right)";
    }
}

bool SubstOp::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return var_name_ == ptr->var_name_ && target_->DeepCompare(ptr->target_) && value_->DeepCompare(ptr->value_);
}


}  /* namespace calculus */
