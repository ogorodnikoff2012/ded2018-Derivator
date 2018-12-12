#include <negate_op.h>
#include <sum.h>
#include "calculus_internal.h"

namespace calculus {

ExpressionPtr NegateOp::Simplify() {
    if (Is<Constant>(expr_)) {
        return BuildConstant(-As<Constant>(expr_)->GetValue());
    }
    if (Is<NegateOp>(expr_)) {
        return As<NegateOp>(expr_)->GetInnerExpr()->Simplify();
    }
    if (Is<Sum>(expr_)) {
        auto summands_copy = As<Sum>(expr_)->GetOperands();
        for (size_t i = 0; i < summands_copy.size(); ++i) {
            summands_copy[i].inverse ^= true;
        }
        return std::make_shared<Sum>(std::move(summands_copy));
    }
    return std::make_shared<NegateOp>(expr_->Simplify());
}

ExpressionPtr NegateOp::TakeDerivative(char var_name) {
    return std::make_shared<NegateOp>(expr_->TakeDerivative(var_name));
}

ExpressionPtr NegateOp::Call(const std::vector<ExpressionPtr>& args) {
    return std::make_shared<NegateOp>(expr_->Call(args));
}

ExpressionPtr NegateOp::Substitute(char var_name, const ExpressionPtr& value) {
    return std::make_shared<NegateOp>(expr_->Substitute(var_name, value));
}

void NegateOp::Print(std::ostream& out, int cur_priority_level) const {
    out << '-';

    if (cur_priority_level > kPrefixOpPriorityLevel) {
        out << '(';
    }
    expr_->Print(out, kPrefixOpPriorityLevel);
    if (cur_priority_level > kPrefixOpPriorityLevel) {
        out << ')';
    }
}

void NegateOp::TexDump(std::ostream& out, int cur_priority_level) const {
    out << '-';

    if (cur_priority_level > kPrefixOpPriorityLevel) {
        out << "\\left(";
    }
    expr_->TexDump(out, kPrefixOpPriorityLevel);
    if (cur_priority_level > kPrefixOpPriorityLevel) {
        out << "\\right)";
    }
}

bool NegateOp::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return expr_->DeepCompare(ptr->expr_);
}



}  /* namespace calculus */
