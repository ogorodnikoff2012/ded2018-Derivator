#pragma once

#include <memory>
#include <vector>
#include <cmath>

namespace calculus {

constexpr char kDefaultDerivativeVariable = 'x';
constexpr double kDoubleTolerance = 1e-12;

constexpr int kSumPriorityLevel = 0;
constexpr int kProdPriorityLevel = 10;
constexpr int kPrefixOpPriorityLevel = 20;
constexpr int kPostfixOpPriorityLevel = 30;

class RuntimeError : public std::runtime_error {
public:
    explicit RuntimeError(const std::string& what) : std::runtime_error(what) {
    }
};

class Expression : public std::enable_shared_from_this<Expression> {
public:
    using ExpressionPtr = std::shared_ptr<Expression>;
    virtual ~Expression() = default;
    virtual ExpressionPtr Simplify() = 0;
    virtual ExpressionPtr TakeDerivative(char var_name) = 0;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) = 0;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) = 0;
    virtual void Print(std::ostream& out, int cur_priority_level = -1) const = 0;
    virtual void TexDump(std::ostream& out, int cur_priority_level = -1) const = 0;
    virtual bool DeepCompare(const ExpressionPtr& other) const = 0;
};

using ExpressionPtr = std::shared_ptr<Expression>;

struct AssociativeOperand {
    ExpressionPtr expr;
    bool inverse;

    template <class EPtr>
    AssociativeOperand(EPtr&& expr, bool inverse) : expr(std::forward<EPtr>(expr)), inverse(inverse) {
    }

    AssociativeOperand() = default;
};

double Ratio(const ExpressionPtr& lhs, const ExpressionPtr& rhs);

}  /* namespace calculus */
