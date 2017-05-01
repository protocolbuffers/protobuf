module google.protobuf.any;

import google.protobuf;

class Any
{
    @Proto(1) string typeUrl = defaultValue!(string);
    @Proto(2) bytes value = defaultValue!(bytes);
}
