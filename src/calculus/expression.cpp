#include <expression.h>
#include <negate_op.h>
#include <product.h>
#include <sum.h>

#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <ostream>

#include "calculus_internal.h"

namespace calculus {

static double RatioOfSums(const std::vector<AssociativeOperand>& l_summands, const std::vector<AssociativeOperand>& r_summands) {
    std::vector<bool> l_used(l_summands.size(), false);
    std::vector<bool> r_used(r_summands.size(), false);
    std::vector<std::vector<double>> ratios(l_summands.size(), std::vector<double>(r_summands.size(), std::nan("")));

    double final_ratio = std::nan("");

    for (size_t l_index = 0; l_index < l_summands.size(); ++l_index) {
        if (l_used[l_index]) {
            continue;
        }

        l_used[l_index] = true;

        std::vector<int> r_active;
        for (size_t r_index = 0; r_index < r_summands.size(); ++r_index) {
            if (r_used[r_index]) {
                continue;
            }

            ratios[l_index][r_index] = Ratio(l_summands[l_index].expr, r_summands[r_index].expr);
            if (l_summands[l_index].inverse ^ r_summands[r_index].inverse) {
                ratios[l_index][r_index] *= -1;
            }

            if (!std::isnan(ratios[l_index][r_index])) {
                r_used[r_index] = true;
                r_active.push_back(r_index);
            }
        }

        if (r_active.empty()) {
            return std::nan("");
        }

        std::vector<size_t> l_active{l_index};

        for (size_t l_candidate = l_index + 1; l_candidate < l_summands.size(); ++l_candidate) {
            bool success = true;
            for (int r_index : r_active) {
                ratios[l_candidate][r_index] = Ratio(l_summands[l_candidate].expr, r_summands[r_index].expr);
                if (l_summands[l_candidate].inverse ^ r_summands[r_index].inverse) {
                    ratios[l_candidate][r_index] *= -1;
                }
                if (std::isnan(ratios[l_candidate][r_index])) {
                    success = false;
                    break;
                }
            }
            if (success) {
                l_active.push_back(l_candidate);
                l_used[l_candidate] = true;
            }
        }

        double current_ratio = 0;
        for (auto r_index : r_active) {
            double sum = 0;
            for (auto l_index : l_active) {
                sum += ratios[l_index][r_index];
            }
            current_ratio += 1 / sum;
        }
        current_ratio = 1 / current_ratio;

        if (!std::isnan(final_ratio) && !IsZero(final_ratio - current_ratio)) {
            return std::nan("");
        }
        final_ratio = current_ratio;
    }

    return final_ratio;
}

static double RatioOfProducts(const std::vector<AssociativeOperand>& l_multipliers, const std::vector<AssociativeOperand>& r_multipliers) {
    std::vector<bool> used(r_multipliers.size(), false);

    double const_ratio = 1;

    for (size_t i = 0; i < r_multipliers.size(); ++i) {
        if (Is<Constant>(r_multipliers[i].expr)) {
            used[i] = true;
            double value = As<Constant>(r_multipliers[i].expr)->GetValue();
            if (r_multipliers[i].inverse) {
                const_ratio *= value;
            } else {
                const_ratio /= value;
            }
        }
    }

    for (size_t l_index = 0; l_index < l_multipliers.size(); ++l_index) {
        if (Is<Constant>(l_multipliers[l_index].expr)) {
            double value = As<Constant>(l_multipliers[l_index].expr)->GetValue();
            if (l_multipliers[l_index].inverse) {
                const_ratio /= value;
            } else {
                const_ratio *= value;
            }
            continue;
        }

        bool success = false;
        for (size_t r_index = 0; r_index < r_multipliers.size(); ++r_index) {
            if (!used[r_index] && l_multipliers[l_index].inverse == r_multipliers[r_index].inverse &&
                    l_multipliers[l_index].expr->DeepCompare(r_multipliers[r_index].expr)) {
                success = true;
                used[r_index] = true;
                break;
            }
        }

        if (!success) {
            return std::nan("");
        }
    }

    for (bool flag : used) {
        if (!flag) {
            return std::nan("");
        }
    }
    return const_ratio;
}

double Ratio(const ExpressionPtr& lhs, const ExpressionPtr& rhs) {
    if (Is<NegateOp>(lhs)) {
        return -Ratio(As<NegateOp>(lhs)->GetInnerExpr(), rhs);
    }
    if (Is<NegateOp>(rhs)) {
        return -Ratio(lhs, As<NegateOp>(rhs)->GetInnerExpr());
    }

    if (Is<Constant>(lhs) && Is<Constant>(rhs)) {
        return As<Constant>(lhs)->GetValue() / As<Constant>(rhs)->GetValue();
    }

    if (lhs->DeepCompare(rhs)) {
        return 1;
    }

    if (Is<Product>(lhs)) {
        const auto& l_multipliers = As<Product>(lhs)->GetOperands();
        if (l_multipliers.size() == 2) {
            if (Is<Constant>(l_multipliers[0].expr) && !l_multipliers[1].inverse) {
                double r = Ratio(l_multipliers[1].expr, rhs);
                double alpha = As<Constant>(l_multipliers[0].expr)->GetValue();
                return l_multipliers[0].inverse ? r / alpha : r * alpha;
            }
            if (Is<Constant>(l_multipliers[1].expr) && !l_multipliers[0].inverse) {
                double r = Ratio(l_multipliers[0].expr, rhs);
                double alpha = As<Constant>(l_multipliers[1].expr)->GetValue();
                return l_multipliers[1].inverse ? r / alpha : r * alpha;
            }
        }

        if (Is<Product>(rhs)) {
            const auto& r_multipliers = As<Product>(rhs)->GetOperands();
            return RatioOfProducts(l_multipliers, r_multipliers);
        }
    } else if (Is<Product>(rhs)) {
        return 1 / Ratio(rhs, lhs);
    }

    if (Is<Sum>(lhs) && Is<Sum>(rhs)) {
        const auto& l_summands = As<Sum>(lhs)->GetOperands();
        const auto& r_summands = As<Sum>(rhs)->GetOperands();
        return RatioOfSums(l_summands, r_summands);
    }

    return std::nan("");
}

}
