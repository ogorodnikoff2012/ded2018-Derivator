#pragma once

#include "expression.h"

namespace calculus {

class NegateOp : public Expression {
public:
    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual void TexDump(std::ostream& out, int cur_priority_level = -1) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    explicit NegateOp(const ExpressionPtr& expr) : expr_(expr) {
    }

    const ExpressionPtr& GetInnerExpr() const {
        return expr_;
    }

private:
    ExpressionPtr expr_;
};

}  /* namespace calculus */
