#pragma once

#include "expression.h"

namespace calculus {

class PowerOp : public Expression {
public:
    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual void TexDump(std::ostream& out, int cur_priority_level = -1) const override;
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

}  /* namespace calculus */
