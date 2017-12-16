
HailContext::HailContext()
  : int32_required(std::make_shared<TInt32>(true)),
    int32_optiona(std::make_shared<TInt32>(false)),
    float64_required(std::make_shared<TFloat64>(true)),
    float64_optional(std::make_shared<TFloat64>(false))
{
}
