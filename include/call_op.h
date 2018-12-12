#pragma once

#include "expression.h"

namespace calculus {

class CallOp : public Expression {
public:
    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual void TexDump(std::ostream& out, int cur_priority_level = -1) const override;
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

}  /* namespace calculus */
