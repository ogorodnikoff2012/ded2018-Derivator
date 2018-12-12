#include <variable.h>
#include "calculus_internal.h"

namespace calculus {

ExpressionPtr Variable::Simplify() {
    return shared_from_this();
}

ExpressionPtr Variable::TakeDerivative(char var_name) {
    return (var_name == name_) ? kConstantOne : kConstantZero;
}

ExpressionPtr Variable::Call(const std::vector<ExpressionPtr>&) {
    return shared_from_this();
}

ExpressionPtr Variable::Substitute(char var_name, const ExpressionPtr& other) {
    return var_name == name_ ? other : shared_from_this();
}

void Variable::Print(std::ostream& out, int) const {
    out << name_;
}

void Variable::TexDump(std::ostream& out, int) const {
    out << name_;
}

bool Variable::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return name_ == ptr->name_;
}

}  /* namespace calculus */
