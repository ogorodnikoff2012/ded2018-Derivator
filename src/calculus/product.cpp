#include <product.h>
#include <negate_op.h>
#include <power_op.h>
#include <sum.h>
#include "calculus_internal.h"

namespace calculus {

ExpressionPtr Product::Simplify() {
    if (multipliers_.size() == 1 && !multipliers_[0].inverse) {
        return multipliers_[0].expr->Simplify();
    }
    std::vector<AssociativeOperand> multipliers_copy;

    AssociativeOpAlign<Product>(multipliers_, false, &multipliers_copy);

    bool need_to_be_negated = false;

    for (size_t i = 0; i < multipliers_copy.size(); ++i) {
        multipliers_copy[i].expr = multipliers_copy[i].expr->Simplify();
        if (Is<NegateOp>(multipliers_copy[i].expr)) {
            need_to_be_negated ^= true;
            multipliers_copy[i].expr = As<NegateOp>(multipliers_copy[i].expr)->GetInnerExpr();
        }
        if (Is<PowerOp>(multipliers_copy[i].expr)) {
            auto expr = As<PowerOp>(multipliers_copy[i].expr);
            if (Is<Constant>(expr->GetExp())) {
                if (As<Constant>(expr->GetExp())->GetValue() + kDoubleTolerance >= 0) {
                    continue;
                }
            } else if (!multipliers_copy[i].inverse) {
                continue;
            }
            multipliers_copy[i].inverse ^= true;
            multipliers_copy[i].expr = std::make_shared<PowerOp>(expr->GetBase(), std::make_shared<NegateOp>(expr->GetExp()));
        }
        if (Is<Constant>(multipliers_copy[i].expr) && As<Constant>(multipliers_copy[i].expr)->GetValue() + kDoubleTolerance < 0) {
            need_to_be_negated ^= true;
            multipliers_copy[i].expr = BuildConstant(-As<Constant>(multipliers_copy[i].expr)->GetValue());
        }
    }

    // Constant folding
    int last_nonconstant = 0;
    MoveConstantsToEnd(&multipliers_copy, &last_nonconstant);

    double value = 1;
    for (size_t i = last_nonconstant + 1; i < multipliers_copy.size(); ++i) {
        double multiplier = As<Constant>(multipliers_copy[i].expr)->GetValue();
        if (multipliers_copy[i].inverse) {
            if (IsZero(multiplier)) {
                throw RuntimeError("Division by zero");
            }
            value /= multiplier;
        } else {
            value *= multiplier;
        }
    }

    if (IsZero(value)) {
        return kConstantZero;
    }

    if (last_nonconstant < 0) {
        return BuildConstant(need_to_be_negated ? -value : value);
    }

    if (IsZero(value - 1) || IsZero(value + 1)) {
        multipliers_copy.resize(last_nonconstant + 1);
        need_to_be_negated ^= IsZero(value + 1);
    } else {
        multipliers_copy.resize(last_nonconstant + 2);
        multipliers_copy.back().expr = BuildConstant(value);
        multipliers_copy.back().inverse = false;
    }

    // Power folding

    for (size_t i = 0; i < multipliers_copy.size(); ++i) {
        if (Is<Constant>(multipliers_copy[i].expr)) {
            continue;
        }

        auto total_power = std::make_shared<Sum>();

        auto simple = multipliers_copy[i].expr;
        if (Is<PowerOp>(simple)) {
            auto exp = As<PowerOp>(simple)->GetExp();
            simple = As<PowerOp>(simple)->GetBase();
            *total_power += exp;
        } else {
            *total_power += kConstantOne;
        }

        bool found_similars = false;
        for (size_t j = i + 1; j < multipliers_copy.size(); ++j) {
            if (Is<Constant>(multipliers_copy[j].expr)) {
                continue;
            }
            bool not_equal_inverse = multipliers_copy[i].inverse != multipliers_copy[j].inverse;

            auto other_simple = multipliers_copy[j].expr;
            ExpressionPtr other_exp = kConstantOne;
            if (Is<PowerOp>(other_simple)) {
                other_exp = As<PowerOp>(other_simple)->GetExp();
                other_simple = As<PowerOp>(other_simple)->GetBase();
            }

            double ratio = Ratio(other_simple, simple);
            if (std::isnan(ratio)) {
                continue;
            }
            found_similars = true;

            if (not_equal_inverse) {
                *total_power -= other_exp;
            } else {
                *total_power += other_exp;
            }

            multipliers_copy[j].expr = std::make_shared<PowerOp>(BuildConstant(ratio), other_exp);

/*            if (multipliers_copy[i].inverse != multipliers_copy[j].inverse) {
                double ratio = Ratio(multipliers_copy[i].expr, multipliers_copy[j].expr);
                if (!std::isnan(ratio)) {
                    multipliers_copy[i].expr = BuildConstant(ratio);
                    multipliers_copy[j].expr = kConstantOne;
                    break;
                }
            }*/
        }

        if (found_similars) {
            multipliers_copy[i].expr = std::make_shared<PowerOp>(simple, total_power);
        }
    }

    auto result = std::make_shared<Product>(std::move(multipliers_copy));
    if (need_to_be_negated) {
        return std::make_shared<NegateOp>(result);
    }
    return result;
}

ExpressionPtr Product::TakeDerivative(char var_name) {
    auto result = std::make_shared<Sum>();
    result->ReserveSize(multipliers_.size());

    for (size_t i = 0; i < multipliers_.size(); ++i) {
        auto multipliers_copy = multipliers_;
        multipliers_copy[i].expr = multipliers_copy[i].expr->TakeDerivative(var_name);
        multipliers_copy[i].inverse = false;

        auto summand = std::make_shared<Product>(std::move(multipliers_copy));
        if (multipliers_[i].inverse) {
            *summand /= multipliers_[i].expr;
            *summand /= multipliers_[i].expr;
            *result -= summand;
        } else {
            *result += summand;
        }
    }

    return result->Simplify();
}

ExpressionPtr Product::Call(const std::vector<ExpressionPtr>& args) {
    decltype(multipliers_) multipliers;
    multipliers.reserve(multipliers_.size());
    for (const auto& multiplier : multipliers_) {
        multipliers.emplace_back(multiplier.expr->Call(args), multiplier.inverse);
    }

    return std::make_shared<Product>(std::move(multipliers))->Simplify();
}

ExpressionPtr Product::Substitute(char var_name, const ExpressionPtr& expr) {
    decltype(multipliers_) multipliers;
    multipliers.reserve(multipliers_.size());
    for (const auto& multiplier : multipliers_) {
        multipliers.emplace_back(multiplier.expr->Substitute(var_name, expr), multiplier.inverse);
    }

    return std::make_shared<Product>(std::move(multipliers))->Simplify();
}

void Product::Print(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kProdPriorityLevel) {
        out << '(';
    }

    bool skip_last = false;

    if (multipliers_[0].inverse) {
        if (Is<Constant>(multipliers_.back().expr) && !multipliers_.back().inverse) {
            out << As<Constant>(multipliers_.back().expr)->GetValue() << " / ";
            skip_last = true;
        } else {
            out << "1 / ";
        }
    }
    multipliers_[0].expr->Print(out, kProdPriorityLevel + (multipliers_[0].inverse ? 1 : 0));

    for (size_t i = 1, n = multipliers_.size() - (skip_last ? 1 : 0); i < n; ++i) {
        out << (multipliers_[i].inverse ? " / " : " * ");
        multipliers_[i].expr->Print(out, kProdPriorityLevel + (multipliers_[i].inverse ? 1 : 0));
    }
    if (cur_priority_level > kProdPriorityLevel) {
        out << ')';
    }
}

static inline void PrintProduct(std::ostream& out, const std::vector<const ExpressionPtr*>& multipliers) {
    bool was_prev_a_number = false;
    for (size_t i = 0; i < multipliers.size(); ++i) {
        bool is_number = Is<Constant>(*multipliers[i]);
        if ((i > 0 && is_number) || was_prev_a_number) {
            out << "\\cdot ";
        }
        multipliers[i]->get()->TexDump(out, kProdPriorityLevel);
        was_prev_a_number = is_number;
    }
}

void Product::TexDump(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kProdPriorityLevel) {
        out << "\\left(";
    }

    std::vector<const ExpressionPtr*> numerator;
    std::vector<const ExpressionPtr*> denominator;

    for (const auto& multiplier : multipliers_) {
        if (multiplier.inverse) {
            denominator.push_back(&multiplier.expr);
        } else {
            numerator.push_back(&multiplier.expr);
        }
    }

    if (!denominator.empty()) {
        out << "\\frac{";
    }
    if (numerator.empty()) {
        out << "1";
    } else {
        PrintProduct(out, numerator);
    }
    if (!denominator.empty()) {
        out << "}{";
        PrintProduct(out, denominator);
        out << "}";
    }

    if (cur_priority_level > kProdPriorityLevel) {
        out << "\\right)";
    }
}

bool Product::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    if (multipliers_.size() != ptr->multipliers_.size()) {
        return false;
    }
    for (size_t i = 0; i < multipliers_.size(); ++i) {
        if (multipliers_[i].inverse != ptr->multipliers_[i].inverse ||
                !multipliers_[i].expr->DeepCompare(ptr->multipliers_[i].expr)) {
            return false;
        }
    }
    return true;
}

Product& Product::operator*=(const ExpressionPtr& expr) {
    multipliers_.emplace_back(expr, false);
    return *this;
}

Product& Product::operator/=(const ExpressionPtr& expr) {
    multipliers_.emplace_back(expr, true);
    return *this;
}

void Product::ReserveSize(int size) {
    multipliers_.reserve(size);
}



}  /* namespace calculus */
