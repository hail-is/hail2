
class HailContext {
  const std::shared_ptr<const TInt32> int32_required, int32_optional;
  const std::shared_ptr<const TFloat64> float64_required, float64_optional;
  
  std::shared_ptr<const TInt32> get_int32(bool required);
  
public:
  HailContext();
};
