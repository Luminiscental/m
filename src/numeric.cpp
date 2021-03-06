
#include <mth/mth.h>

#include <mth/numeric.h>
#include <mth/polynomial.h>

// Return a polynomial interpolated through points (xTransform(t), yFunc(t)) with t approaching zero from above on the real axis
mth::Polynomial lerpTowards(std::function<mth::comp(mth::comp)> xTransform, std::function<mth::comp(size_t,mth::comp)> yFunc) {

    using std::pow;
    using std::abs;

    std::vector<mth::cvec2> result;

    for (size_t i = 2; i < 100; i++) {

        // TODO: Parametrize this sequence or choose it contextually
        auto approachingZero = pow(mth::comp(2), -mth::comp(i));

        auto x = xTransform(approachingZero);
        auto y = yFunc(i, x);

        // If sequence is close enough to cause division by zero then we can probably break
        if (std::isnan(y.real()) || std::isnan(y.imag()) || std::isnan(x.real()) || std::isnan(x.imag())) {

            break;
        }

        result.push_back(mth::cvec2(x, y));
    }

    // number of vertices to interpolate
    auto n = size_t{6};

    auto lastIndex = result.size() - 1;

    // If there are less than n points use all of them
    auto startIndex = lastIndex > (n - 1) ? lastIndex - n : 0;

    return mth::Polynomial::interpolate(result, startIndex, lastIndex);
}

// Returns the shank transform of a sequence of partial sums to accelerate convergence
std::function<mth::comp(size_t)> shankTransform(const std::function<mth::comp(size_t)> &partialSum, const std::function<mth::comp(size_t)> &sequence) {

    return [partialSum, sequence] (size_t n) {

        if (n == 0) return sequence(0);

        auto nextValue = sequence(n + 1);
        auto currentValue = sequence(n);
        auto nextSum = nextValue + currentValue + partialSum(n - 1);

        auto denom = nextValue - currentValue;

        // Avoid division by zero
        if (mth::util::isZero(denom)) {

            return nextSum;
        }

        return nextSum - nextValue * nextValue / denom;
    };
}

// Returns the aitken delta-squared transform of a sequence to accelerate convergence
std::function<mth::comp(size_t)> aitkenTransform(const std::function<mth::comp(size_t)> &sequence) {

    return [sequence] (size_t n) {

        if (n == 0) return mth::comp(0);

        auto next = sequence(n + 1);
        auto curr = sequence(n);
        auto prev = sequence(n - 1);

        auto step = next - curr;

        auto denom = step - curr + prev;

        // Avoid division by zero
        if (mth::util::isZero(denom)) {

            return next;
        }

        return next - step * step / denom;
    };
}

mth::comp mth::limit(const std::function<mth::comp(size_t)> &sequence) {

    auto accelerated = aitkenTransform(sequence);

    auto id = [] (comp z) { return z; };
    auto y = [&] (size_t index, comp x) { return accelerated(index); };

    auto pol = lerpTowards(id, y);

    return pol.getCoeff(0);
}

mth::comp mth::seriesLimit(const std::function<mth::comp(size_t)> &partialSum, const std::function<mth::comp(size_t)> &sequence) {

    auto accelerated = shankTransform(partialSum, sequence);

    auto id = [] (comp z) { return z; };
    auto y = [&] (size_t index, comp x) { return accelerated(index); };

    auto pol = lerpTowards(id, y);

    return pol.getCoeff(0);
}

mth::comp mth::lowerLimit(const std::function<mth::comp(mth::comp)> &function, const mth::comp &input) {

    auto x = [&] (comp small) { return input - small; };
    auto y = [&] (size_t index, comp x) { return function(x); };

    auto pol = lerpTowards(x, y);

    return pol.getCoeff(0);
}

mth::comp mth::upperLimit(const std::function<mth::comp(mth::comp)> &function, const mth::comp &input) {

    auto x = [&] (comp small) { return input + small; };
    auto y = [&] (size_t index, comp x) { return function(x); };

    auto pol = lerpTowards(x, y);

    return pol.getCoeff(0);
}

mth::comp mth::limit(const std::function<mth::comp(mth::comp)> &function, const mth::comp &input) {

    return lowerLimit(function, input);
}

mth::comp mth::limitInfPos(const std::function<mth::comp(mth::comp)> &function) {

    auto inverted = [function] (mth::comp z) { return function(z.inverse()); };
    return upperLimit(inverted, comp(0));
}

mth::comp mth::limitInfNeg(const std::function<mth::comp(mth::comp)> &function) {

    auto inverted = [function] (mth::comp z) { return function(z.inverse()); };
    return lowerLimit(inverted, comp(0));
}

std::function<mth::comp(mth::comp)> mth::differentiate(const std::function<mth::comp(mth::comp)> &function) {

    auto avgGradient = [&] (comp a, comp b) {

        auto dy = function(b) - function(a);
        auto dx = b - a;

        return dy / dx;
    };

    return [&] (comp x) {

        auto gradientApprox = [&] (mth::comp dx) {

            return avgGradient(x, x + dx);
        };

        return limit(gradientApprox, comp(0));
    };
}

