namespace Google.ProtocolBuffers.Bcl {
  public partial class Decimal {
    public decimal ToDecimal() {
      if (Lo == 0 && Hi == 0) return decimal.Zero;

      int lo = (int)(Lo & 0xFFFFFFFFL),
          mid = (int)((Lo >> 32) & 0xFFFFFFFFL),
          hi = (int)Hi;
      bool isNeg = (SignScale & 0x0001) == 0x0001;
      byte scale = (byte)((SignScale & 0x01FE) >> 1);
      return new decimal(lo, mid, hi, isNeg, scale);
    }
  }
}
