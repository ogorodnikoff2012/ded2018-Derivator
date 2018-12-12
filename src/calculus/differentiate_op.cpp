#include <differentiate_op.h>
#include <call_op.h>
#include "calculus_internal.h"

namespace calculus {

ExpressionPtr DifferentiateOp::Simplify() {
    return expr_->Simplify()->TakeDerivative(var_name_);
}

ExpressionPtr DifferentiateOp::TakeDerivative(char var_name) {
    return std::make_shared<DifferentiateOp>(expr_->TakeDerivative(var_name_), var_name);
}

ExpressionPtr DifferentiateOp::Call(const std::vector<ExpressionPtr>& args) {
    auto func = expr_->TakeDerivative(var_name_);
    if (Is<DifferentiateOp>(func)) {
        return std::make_shared<CallOp>(func, args);
    } else {
        return func->Call(args);
    }
}

ExpressionPtr DifferentiateOp::Substitute(char var_name, const ExpressionPtr& expr) {
    return std::make_shared<DifferentiateOp>(expr_->Substitute(var_name, expr), var_name_);
}

void DifferentiateOp::Print(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << '(';
    }
    expr_->Print(out, kPostfixOpPriorityLevel);
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << ')';
    }
    out << "'_" << var_name_;
}

void DifferentiateOp::TexDump(std::ostream& out, int cur_priority_level) const {
    ExpressionPtr inner_expr = expr_;
    std::vector<char> variables = {var_name_};
    while (Is<DifferentiateOp>(inner_expr)) {
        variables.push_back(As<DifferentiateOp>(inner_expr)->var_name_);
        inner_expr = As<DifferentiateOp>(inner_expr)->expr_;
    }

    if (cur_priority_level > kPostfixOpPriorityLevel + 1) {
        out << "\\left(";
    }
    inner_expr->TexDump(out, kPostfixOpPriorityLevel + 1);
    if (cur_priority_level > kPostfixOpPriorityLevel + 1) {
        out << "\\right)";
    }

    out << "_{";
    for (auto iter = variables.rbegin(); iter != variables.rend(); ++iter) {
        out << *iter;
    }
    out << "}^{";
    for (auto iter = variables.rbegin(); iter != variables.rend(); ++iter) {
        out << "\\lefteqn{\\prime}\\phantom{" << *iter << "}";
    }
    out << '}';
}

bool DifferentiateOp::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return var_name_ == ptr->var_name_ && expr_->DeepCompare(ptr->expr_);
}


}  /* namespace calculus */
