#pragma once

namespace fire_engine
{

class VariantState
{
public:
    VariantState() = default;
    ~VariantState() = default;

    VariantState(const VariantState&) = default;
    VariantState& operator=(const VariantState&) = default;
    VariantState(VariantState&&) noexcept = default;
    VariantState& operator=(VariantState&&) noexcept = default;

    [[nodiscard]] int cycleDelta() const noexcept
    {
        return cycleDelta_;
    }
    void cycleDelta(int delta) noexcept
    {
        cycleDelta_ = delta;
    }

    [[nodiscard]] bool hasCycleCommand() const noexcept
    {
        return cycleDelta_ != 0;
    }

private:
    int cycleDelta_{0};
};

} // namespace fire_engine
