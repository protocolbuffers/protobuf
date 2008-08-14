using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers.FieldAccess {

  delegate bool HasDelegate<T>(T message);
  delegate T ClearDelegate<T>(T builder);
  delegate int RepeatedCountDelegate<T>(T message);
  delegate object GetValueDelegate<T>(T message);
}
