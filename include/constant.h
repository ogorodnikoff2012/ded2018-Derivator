#pragma once

#include "expression.h"

namespace calculus {

class Constant : public Expression {
public:
    explicit Constant(double value) : value_(value) {
    }

    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual void TexDump(std::ostream& out, int cur_priority_level = -1) const override;
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

}  /* namespace calculus */
