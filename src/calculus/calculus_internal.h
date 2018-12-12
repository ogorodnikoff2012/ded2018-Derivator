#pragma once

#include <ostream>
#include <constant.h>

#define COMPARE_CHECK_TRIVIAL                                                   \
    auto ptr = dynamic_cast<const std::decay_t<decltype(*this)>*>(other.get()); \
    if (ptr == nullptr) {                                                       \
        return false;                                                           \
    }                                                                           \
    if (ptr == this) {                                                          \
        return true;                                                            \
    }

namespace calculus {

static inline bool IsZero(double x) {
    return std::fabs(x) < kDoubleTolerance;
}

template <class T>
static inline const T* As(const ExpressionPtr& ptr) {
    return dynamic_cast<const T*>(ptr.get());
}

template <class T>
static inline bool Is(const ExpressionPtr& ptr) {
    return As<T>(ptr) != nullptr;
}

template <class T>
static inline void AssociativeOpAlign(const std::vector<AssociativeOperand>& operands, bool global_inverse, std::vector<AssociativeOperand>* result) {
    for (size_t i = 0; i < operands.size(); ++i) {
        if (Is<T>(operands[i].expr)) {
            auto op = As<T>(operands[i].expr);
            AssociativeOpAlign<T>(op->GetOperands(), operands[i].inverse ^ global_inverse, result);
        } else {
            result->push_back(operands[i]);
            result->back().inverse ^= global_inverse;
        }
    }
}

static inline void MoveConstantsToEnd(std::vector<AssociativeOperand>* operands_ptr, int* last_nonconstant_ptr) {
    auto& operands = *operands_ptr;
    int& last_nonconstant = *last_nonconstant_ptr;

    last_nonconstant = operands.size() - 1;
    for (int i = operands.size() - 1; i >= 0; --i) {
        if (Is<Constant>(operands[i].expr)) {
            if (i != last_nonconstant) {
                std::swap(operands[i], operands[last_nonconstant]);
            }
            --last_nonconstant;
        }
    }
}

static inline ExpressionPtr BuildConstant(double x) {
    if (IsZero(x)) {
        return kConstantZero;
    }
    if (IsZero(x - 1)) {
        return kConstantOne;
    }
    if (IsZero(x - M_PI)) {
        return kConstantPi;
    }
    return std::make_shared<Constant>(x);
}

}  /* namespace calculus */
