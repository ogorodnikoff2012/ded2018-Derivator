#pragma once 

#include "expression.h"

namespace calculus {
	
class SubstOp : public Expression {
public:
    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual void TexDump(std::ostream& out, int cur_priority_level = -1) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    explicit SubstOp(const ExpressionPtr& target, char var_name, const ExpressionPtr& value)
        : target_(target), var_name_(var_name), value_(value) {
    }

private:
    ExpressionPtr target_;
    char var_name_;
    ExpressionPtr value_;
};

}  /* namespace calculus */