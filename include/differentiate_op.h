#pragma once

#include "expression.h"

namespace calculus {

class DifferentiateOp : public Expression {
public:
    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual void TexDump(std::ostream& out, int cur_priority_level = -1) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    DifferentiateOp(const ExpressionPtr& expr, char var_name) : expr_(expr), var_name_(var_name) {
    }

private:
    ExpressionPtr expr_;
    char var_name_;
};

}  /* namespace calculus */
