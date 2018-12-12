#include <constant.h>
#include "calculus_internal.h"

namespace calculus {

ExpressionPtr Constant::Simplify() {
    return shared_from_this();
}

ExpressionPtr Constant::TakeDerivative(char) {
    return kConstantZero;
}

ExpressionPtr Constant::Call(const std::vector<ExpressionPtr>&) {
    return shared_from_this();
}

ExpressionPtr Constant::Substitute(char, const ExpressionPtr&) {
    return shared_from_this();
}

void Constant::Print(std::ostream& out, int current_priority_level) const {
    if (value_ < 0 && current_priority_level > kSumPriorityLevel) {
        out << '(';
    }
    out << value_;
    if (value_ < 0 && current_priority_level > kSumPriorityLevel) {
        out << ')';
    }
}

void Constant::TexDump(std::ostream& out, int current_priority_level) const {
    if (value_ < 0 && current_priority_level > kSumPriorityLevel) {
        out << "\\left(";
    }
    out << value_;
    if (value_ < 0 && current_priority_level > kSumPriorityLevel) {
        out << "\\right)";
    }
}

bool Constant::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return std::fabs(value_ - ptr->value_) < kDoubleTolerance;
}

const ExpressionPtr kConstantZero   = std::make_shared<Constant>(0);
const ExpressionPtr kConstantOne    = std::make_shared<Constant>(1);
const ExpressionPtr kConstantNegOne = std::make_shared<Constant>(-1);
const ExpressionPtr kConstantPi     = std::make_shared<Constant>(M_PI);

}  /* namespace calculus */
