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
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    const std::string& GetName() const {
        return name_;
    }

private:
    std::string name_;
};

class Sum : public Expression {
public:
    Sum() = default;

    template <class Vec1, class Vec2>
    Sum(Vec1&& summands, Vec2&& negate)
        : summands_(std::forward<Vec1>(summands)), negate_(std::forward<Vec2>(negate)) {
    }

    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    Sum& operator+=(const ExpressionPtr& expr);
    Sum& operator-=(const ExpressionPtr& expr);
    void ReserveSize(int size);

    const std::vector<ExpressionPtr>& GetOperands() const {
        return summands_;
    }

    const std::vector<bool>& GetInverses() const {
        return negate_;
    }

private:
    std::vector<ExpressionPtr> summands_;
    std::vector<bool> negate_;
};

class Product : public Expression {
public:
    Product() = default;

    template <class Vec1, class Vec2>
    Product(Vec1&& multipliers, Vec2&& inverse)
        : multipliers_(std::forward<Vec1>(multipliers)), inverse_(std::forward<Vec2>(inverse)) {
    }

    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    Product& operator*=(const ExpressionPtr& expr);
    Product& operator/=(const ExpressionPtr& expr);
    void ReserveSize(int size);

    const std::vector<ExpressionPtr>& GetOperands() const {
        return multipliers_;
    }

    const std::vector<bool>& GetInverses() const {
        return inverse_;
    }

private:
    std::vector<ExpressionPtr> multipliers_;
    std::vector<bool> inverse_;
};

class NegateOp : public Expression {
public:
    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
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

} /* namespace */
