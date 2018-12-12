#include <function.h>
#include <product.h>
#include <negate_op.h>
#include <differentiate_op.h>
#include <call_op.h>
#include "calculus_internal.h"
#include <unordered_set>
#include <unordered_map>

namespace calculus {

static ExpressionPtr Inverse(const ExpressionPtr& expr) {
    auto result = std::make_shared<Product>();
    *result /= expr;
    return result;
}

static const std::unordered_map<std::string, ExpressionPtr> kTableOfDerivatives = {
    {"sin", std::make_shared<Function>("cos")},
    {"cos", std::make_shared<NegateOp>(std::make_shared<Function>("sin"))},
    {"log", Inverse(std::make_shared<Function>("id"))},
    {"exp", std::make_shared<Function>("exp")},
    {"id",  kConstantOne},
};

static const std::unordered_map<std::string, double(*)(double)> kUnaryFunctionTable = {
    {"sin", std::sin},
    {"cos", std::cos},
    {"log", std::log},
    {"exp", std::exp},
    {"id",  [](double x) { return x; }},
};

static const std::unordered_set<std::string> kTableOfLaTeXDeclaredFunctions = {
    "sin", "cos", "log", "exp"
};


ExpressionPtr Function::Simplify() {
    return shared_from_this();
}

ExpressionPtr Function::TakeDerivative(char) {
    auto iter = kTableOfDerivatives.find(name_);
    if (iter == kTableOfDerivatives.end()) {
        return std::make_shared<DifferentiateOp>(shared_from_this(), kDefaultDerivativeVariable);
    }
    return iter->second;
}

ExpressionPtr Function::Call(const std::vector<ExpressionPtr>& args) {
    // Now we have only unary functions
    if (args.size() != 1) {
        std::string what = "Argument count mismatch: expected 1, got ";
        what += std::to_string(args.size());
        throw RuntimeError(what);
    }

    const auto& arg = args[0]->Simplify();

    if (name_ == "id") {
        return arg;
    }

    auto iter = kUnaryFunctionTable.find(name_);
    if (!Is<Constant>(arg) || iter == kUnaryFunctionTable.end()) {
        return std::make_shared<CallOp>(shared_from_this(), args);
    }

    double result = iter->second(As<Constant>(arg)->GetValue());
    return BuildConstant(result);
}

ExpressionPtr Function::Substitute(char, const ExpressionPtr&) {
    return shared_from_this();
}

void Function::Print(std::ostream& out, int) const {
    out << name_;
}

void Function::TexDump(std::ostream& out, int) const {
    if (kTableOfLaTeXDeclaredFunctions.find(name_) != kTableOfLaTeXDeclaredFunctions.end()) {
        out << '\\';
    }
    out << name_;
}

bool Function::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return name_ == ptr->name_;
}

}  /* namespace calculus */
