#include <power_op.h>
#include <product.h>
#include <sum.h>
#include <call_op.h>
#include <function.h>
#include "calculus_internal.h"

namespace calculus {


ExpressionPtr PowerOp::Simplify() {
    if (Is<Constant>(base_)) {
        double base = As<Constant>(base_)->GetValue();
        if (IsZero(base)) {
            return kConstantZero;
        }
        if (IsZero(base - 1)) {
            return kConstantOne;
        }
    }

    if (Is<Constant>(exp_)) {
        double exp = As<Constant>(exp_)->GetValue();
        if (IsZero(exp)) {
            return kConstantOne;
        }
        if (IsZero(exp - 1)) {
            return base_;
        }
        if (Is<Constant>(base_)) {
            return BuildConstant(std::pow(As<Constant>(base_)->GetValue(), exp));
        }
    }
    if (Is<PowerOp>(base_)) {
        auto new_exp = std::make_shared<Product>();
        *new_exp *= exp_;
        *new_exp *= As<PowerOp>(base_)->exp_;
        return std::make_shared<PowerOp>(As<PowerOp>(base_)->base_->Simplify(), new_exp->Simplify())->Simplify();
    }
    if (Is<Product>(base_)) {
        auto multipliers_copy = As<Product>(base_)->GetOperands();
        for (auto& multiplier : multipliers_copy) {
            multiplier.expr = std::make_shared<PowerOp>(multiplier.expr, exp_);
        }
        return std::make_shared<Product>(std::move(multipliers_copy))->Simplify();
    }
    return std::make_shared<PowerOp>(base_->Simplify(), exp_->Simplify());
}

ExpressionPtr PowerOp::TakeDerivative(char var_name) {
    auto prod1 = std::make_shared<Product>();
    auto sum = std::make_shared<Sum>();
    auto prod2 = std::make_shared<Product>();
    auto prod3 = std::make_shared<Product>();

    *prod2 *= exp_->TakeDerivative(var_name);
    *prod2 *= std::make_shared<CallOp>(std::make_shared<Function>("log"), std::vector<ExpressionPtr>{base_});

    *prod3 *= exp_;
    *prod3 *= base_->TakeDerivative(var_name);
    *prod3 /= base_;

    *sum += prod2;
    *sum += prod3;

    *prod1 *= sum;
    *prod1 *= shared_from_this();
    return prod1;
}

ExpressionPtr PowerOp::Call(const std::vector<ExpressionPtr>& args) {
    return std::make_shared<PowerOp>(base_->Call(args), exp_->Call(args));
}

ExpressionPtr PowerOp::Substitute(char var_name, const ExpressionPtr& value) {
    return std::make_shared<PowerOp>(base_->Substitute(var_name, value), exp_->Substitute(var_name, value));
}

void PowerOp::Print(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << '(';
    }
    base_->Print(out, kPostfixOpPriorityLevel);
    out << " ^ ";
    exp_->Print(out, kPostfixOpPriorityLevel);
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << ')';
    }
}

void PowerOp::TexDump(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << "\\left(";
    }
    if (Is<Constant>(exp_) && IsZero(As<Constant>(exp_)->GetValue() - 0.5)) {
        out << "\\sqrt{";
        base_->TexDump(out, kSumPriorityLevel);
        out << '}';
    } else {
        base_->TexDump(out, kPostfixOpPriorityLevel);
        out << "^{";
        exp_->TexDump(out, kPostfixOpPriorityLevel);
        out << '}';
    }
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << "\\right)";
    }
}

bool PowerOp::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return base_->DeepCompare(ptr->base_) && exp_->DeepCompare(ptr->exp_);
}


}  /* namespace calculus */
