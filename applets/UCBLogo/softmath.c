#include <math.h>
#include <limits.h>

double floor(double x)
{
    if (!(x >= LONG_MIN && x < -(double)LONG_MIN))
        return x;
    long n = x;
    return n > x ? n - 1.0 : n;
}

double ceil(double x) { return -floor(-x); }

double fmod(double x, double y)
{
    double factor = y < 0 ? -y : y;
    double remaining = x < 0 ? -x : x;
    if (factor == 0 || !isfinite(factor) || !isfinite(remaining))
        return 0;
    while (factor <= remaining / 2)
        factor *= 2;
    double divisor = y < 0 ? -y : y;
    while (factor >= divisor) {
        if (remaining >= factor)
            remaining -= factor;
        factor *= 0.5;
    }
    return x < 0 ? -remaining : remaining;
}

double sin(double x)
{
    const double pi = 3.14159265358979323846;
    x = fmod(x, 2 * pi);
    if (x > pi) x -= 2 * pi;
    if (x < -pi) x += 2 * pi;
    if (x > pi / 2) x = pi - x;
    if (x < -pi / 2) x = -pi - x;
    double term = x, sum = x;
    for (int n = 2; n <= 14; n += 2) {
        term *= -(x * x) / (n * (n + 1));
        sum += term;
    }
    return sum;
}

double cos(double x) { return sin(x + 1.57079632679489661923); }

double fabs(double x) { return x < 0 ? -x : x; }

double hypot(double x, double y)
{
    x = fabs(x);
    y = fabs(y);
    double scale = x > y ? x : y;
    if (scale == 0)
        return 0;
    x /= scale;
    y /= scale;
    double square = x * x + y * y;
    double root = 1;
    for (int i = 0; i < 6; ++i)
        root = (root + square / root) / 2;
    return scale * root;
}

double atan(double x)
{
    const double half_pi = 1.57079632679489661923;
    if (x < 0)
        return -atan(-x);
    if (x > 1)
        return half_pi - atan(1 / x);
    if (x > 0.414213562373095)
        return half_pi / 2 + atan((x - 1) / (x + 1));
    double term = x, sum = x;
    for (int n = 3; n <= 23; n += 2) {
        term *= -x * x;
        sum += term / n;
    }
    return sum;
}
