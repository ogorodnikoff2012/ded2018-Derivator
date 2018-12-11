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
    virtual bool DeepCompare(const ExpressionPtr& other) const = 0;
};

using ExpressionPtr = std::shared_ptr<Expression>;

class Constant : public Expression {
public:
    explicit Constant(double value) : value_(value) {
    }

    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    double GetValue() const {
        return value_;
    }

private:
    double value_;
};

extern const ExpressionPtr kConstantZero;
extern const ExpressionPtr kConstantOne;
extern const ExpressionPtr kConstantPi;

class Variable : public Expression {
public:
    explicit Variable(char name) : name_(name) {
    }

    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    char GetName() const {
        return name_;
    }

private:
    char name_;
};

class Function : public Expression {
public:
    explicit Function(const std::string& name) : name_(name) {
    }

    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    const std::string& GetName() const {
        return name_;
    }

private:
    std::string name_;
};

struct AssociativeOperand {
    ExpressionPtr expr;
    bool inverse;

    template <class EPtr>
    AssociativeOperand(EPtr&& expr, bool inverse) : expr(std::forward<EPtr>(expr)), inverse(inverse) {
    }

    AssociativeOperand() = default;
};

class Sum : public Expression {
public:
    Sum() = default;

    template <class Vector>
    explicit Sum(Vector&& summands) : summands_(std::forward<Vector>(summands)) {
    }

    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    Sum& operator+=(const ExpressionPtr& expr);
    Sum& operator-=(const ExpressionPtr& expr);
    void ReserveSize(int size);

    const std::vector<AssociativeOperand>& GetOperands() const {
        return summands_;
    }

private:
    std::vector<AssociativeOperand> summands_;
};

class Product : public Expression {
public:
    Product() = default;

    template <class Vector>
    explicit Product(Vector&& multipliers) : multipliers_(std::forward<Vector>(multipliers)) {
    }

    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    Product& operator*=(const ExpressionPtr& expr);
    Product& operator/=(const ExpressionPtr& expr);
    void ReserveSize(int size);

    const std::vector<AssociativeOperand>& GetOperands() const {
        return multipliers_;
    }

private:
    std::vector<AssociativeOperand> multipliers_;
};

class NegateOp : public Expression {
public:
    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    explicit NegateOp(const ExpressionPtr& expr) : expr_(expr) {
    }

    const ExpressionPtr& GetInnerExpr() const {
        return expr_;
    }

private:
    ExpressionPtr expr_;
};

class DifferentiateOp : public Expression {
public:
    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    DifferentiateOp(const ExpressionPtr& expr, char var_name) : expr_(expr), var_name_(var_name) {
    }

private:
    ExpressionPtr expr_;
    char var_name_;
};

class CallOp : public Expression {
public:
    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    explicit CallOp(const ExpressionPtr& func) : func_(func) {
    }

    template <class Vector>
    CallOp(const ExpressionPtr& func, Vector&& args) : func_(func), args_(std::forward<Vector>(args)) {
    }

    void ReserveSize(int size);
    void AddArgument(ExpressionPtr& arg);

private:
    ExpressionPtr func_;
    std::vector<ExpressionPtr> args_;
};

class PowerOp : public Expression {
public:
    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    explicit PowerOp(const ExpressionPtr& base, const ExpressionPtr& exp) : base_(base), exp_(exp) {
    }

    const ExpressionPtr& GetBase() const {
        return base_;
    }

    const ExpressionPtr& GetExp() const {
        return exp_;
    }

private:
    ExpressionPtr base_;
    ExpressionPtr exp_;
};

class SubstOp : public Expression {
public:
    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    explicit SubstOp(const ExpressionPtr& target, char var_name, const ExpressionPtr& value)
        : target_(target), var_name_(var_name), value_(value) {
    }

private:
    ExpressionPtr target_;
    char var_name_;
    ExpressionPtr value_;
};

double Ratio(const ExpressionPtr& lhs, const ExpressionPtr& rhs);

} /* namespace */
