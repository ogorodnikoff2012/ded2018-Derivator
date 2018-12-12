#include <call_op.h>
#include <product.h>
#include <differentiate_op.h>
#include "calculus_internal.h"

namespace calculus {

ExpressionPtr CallOp::Simplify() {
    std::vector<ExpressionPtr> simplified_args;
    simplified_args.reserve(args_.size());
    for (const auto& arg : args_) {
        simplified_args.push_back(arg->Simplify());
    }
    return func_->Simplify()->Call(simplified_args);
}

ExpressionPtr CallOp::TakeDerivative(char var_name) {
    if (args_.size() == 1) {
        auto result = std::make_shared<Product>();
        result->ReserveSize(2);
        *result *= args_[0]->TakeDerivative(var_name);
        *result *= std::make_shared<CallOp>(func_->TakeDerivative(var_name), args_);
        return result;
    }
    return std::make_shared<DifferentiateOp>(shared_from_this(), var_name);
}

ExpressionPtr CallOp::Call(const std::vector<ExpressionPtr>&/* args*/) {
    return shared_from_this();
    /*
    std::vector<ExpressionPtr> called_args;
    called_args.reserve(args_.size());

    for (const auto& arg : args_) {
        called_args.push_back(arg->Call(args));
    }

    return std::make_shared<CallOp>(func_->Call(called_args), args);
    */
}

ExpressionPtr CallOp::Substitute(char var_name, const ExpressionPtr& value) {
    std::vector<ExpressionPtr> new_args;
    new_args.reserve(args_.size());

    for (const auto& arg : args_) {
        new_args.push_back(arg->Substitute(var_name, value));
    }

    return std::make_shared<CallOp>(func_->Substitute(var_name, value), new_args);

}

void CallOp::Print(std::ostream& out, int cur_priority_level) const {
    func_->Print(out, cur_priority_level);
    out << '(';
    if (!args_.empty()) {
        args_[0]->Print(out, kSumPriorityLevel);
        for (size_t i = 1; i < args_.size(); ++i) {
            out << ", ";
            args_[i]->Print(out, kSumPriorityLevel);
        }
    }
    out << ')';
}

void CallOp::TexDump(std::ostream& out, int cur_priority_level) const {
    func_->TexDump(out, cur_priority_level);
    out << "\\left(";
    if (!args_.empty()) {
        args_[0]->TexDump(out, kSumPriorityLevel);
        for (size_t i = 1; i < args_.size(); ++i) {
            out << ", ";
            args_[i]->TexDump(out, kSumPriorityLevel);
        }
    }
    out << "\\right)";
}

bool CallOp::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    if (args_.size() != ptr->args_.size()) {
        return false;
    }
    if (!func_->DeepCompare(ptr->func_)) {
        return false;
    }
    for (size_t i = 0; i < args_.size(); ++i) {
        if (!args_[i]->DeepCompare(ptr->args_[i])) {
            return false;
        }
    }
    return true;
}


}  /* namespace calculus */
