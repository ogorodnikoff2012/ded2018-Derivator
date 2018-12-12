#pragma once

#include "expression.h"

namespace calculus {

class Function : public Expression {
public:
    explicit Function(const std::string& name) : name_(name) {
    }

    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual void TexDump(std::ostream& out, int cur_priority_level = -1) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    const std::string& GetName() const {
        return name_;
    }

private:
    std::string name_;
};

}  /* namespace calculus */
