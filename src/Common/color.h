#ifdef USE_16BIT_COLOR
typedef uint16_t ColorType;
#else
typedef uint8_t ColorType;
#endif

struct Color
{
    union
    {
        struct
        {
            ColorType a;
            ColorType r;
            ColorType g;
            ColorType b;
        };
        ColorType raw[4];
        uint32_t value; // This will only be meaningful in 8-bit mode.
    };

    inline Color operator+(Color &c) const
    {
        float ta = c.a / (float)maxValue();
        return Color(
            r + min<ColorType>(c.r * ta, maxValue() - r),
            g + min<ColorType>(c.g * ta, maxValue() - g),
            b + min<ColorType>(c.b * ta, maxValue() - b),
            a + min<ColorType>(c.a, maxValue() - a));
    }

    inline Color operator-(Color &c) const
    {
        return Color(
            max<ColorType>(r - c.r, (ColorType)0),
            max<ColorType>(g - c.g, (ColorType)0),
            max<ColorType>(b - c.b, (ColorType)0),
            max<ColorType>(a - c.a, (ColorType)0));
    }

    inline Color operator*(Color &c) const
    {
        return Color(
            constrain(r * (c.r / (float)maxValue()), 0, maxValue()),
            constrain(g * (c.g / (float)maxValue()), 0, maxValue()),
            constrain(b * (c.b / (float)maxValue()), 0, maxValue()),
            constrain(a * (c.a / (float)maxValue()), 0, maxValue()));
    }

    inline Color operator*(const float &val) const
    {
        return Color(
            constrain(r * val, 0, maxValue()),
            constrain(g * val, 0, maxValue()),
            constrain(b * val, 0, maxValue()),
            constrain(a * val, 0, maxValue()));
    }

    inline Color operator/(const float &val) const
    {
        return Color(
            constrain(r / val, 0, maxValue()),
            constrain(g / val, 0, maxValue()),
            constrain(b / val, 0, maxValue()),
            constrain(a / val, 0, maxValue()));
    }

    inline Color &operator+=(Color &c)
    {
        float ta = c.a / (float)maxValue();
        r += min<ColorType>(c.r * ta, maxValue() - r);
        g += min<ColorType>(c.g * ta, maxValue() - g);
        b += min<ColorType>(c.b * ta, maxValue() - b);
        a += min<ColorType>(c.a, maxValue() - a);
        return *this;
    }

    inline Color &operator-=(Color &c)
    {
        r = max<ColorType>(r - c.r, (ColorType)0);
        g = max<ColorType>(g - c.g, (ColorType)0);
        b = max<ColorType>(b - c.b, (ColorType)0);
        a = max<ColorType>(a - c.a, (ColorType)0);
        return *this;
    }

    inline Color &operator*=(Color &c)
    {
        r = constrain(r * (c.r / (float)maxValue()), 0, maxValue());
        g = constrain(g * (c.g / (float)maxValue()), 0, maxValue());
        b = constrain(b * (c.b / (float)maxValue()), 0, maxValue());
        a = constrain(a * (c.a / (float)maxValue()), 0, maxValue());
        return *this;
    }

    Color(uint32_t v = 0xff000000)
    {
#ifdef USE_16BIT_COLOR
        a = maxValue();
        r = g = b = 0;
#else
        value = v;
#endif
    }

    Color(ColorType r, ColorType g, ColorType b, ColorType a = maxValue())
        : a(a), r(r), g(g), b(b)
    {
    }

    Color clone() { return Color(r, g, b, a); }

    static Color HSV(float h, float s, float v, float a = 1.0f)
    {
        return Color(
            v * mix(1.0, constrain(abs(fract(h + 1.0) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s) * maxValue(),
            v * mix(1.0, constrain(abs(fract(h + 0.6666666) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s) * maxValue(),
            v * mix(1.0, constrain(abs(fract(h + 0.3333333) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s) * maxValue(),
            a * maxValue());
    }

    static float mix(float a, float b, float t) { return a + (b - a) * t; }
    static float fract(float x) { return x - int(x); }

    Color withMultipliedAlpha(float val)
    {
        Color c = clone();
        c.a *= val;
        return c;
    }

    Color lerp(Color toColor, float weight)
    {
        Color c;
        for (uint8_t i = 0; i < 4; i++)
        {
            if (raw[i] == toColor.raw[i])
                c.raw[i] = raw[i];
            else
                c.raw[i] = raw[i] + weight * (toColor.raw[i] - raw[i]);
        }

        return c;
    }

    String toString() const
    {
        return "[" + String(r) + "," + String(g) + "," + String(b) + "," + String(a) + "]";
    }

private:
    static constexpr ColorType maxValue()
    {
#ifdef USE_16BIT_COLOR
        return 65535;
#else
        return 255;
#endif
    }
};