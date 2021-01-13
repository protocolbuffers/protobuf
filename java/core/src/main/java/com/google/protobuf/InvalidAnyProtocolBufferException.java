package com.google.protobuf;

import java.io.IOException;

public class InvalidAnyProtocolBufferException extends InvalidProtocolBufferException {
    public final String typeUrl;

    public InvalidAnyProtocolBufferException(String description, String typeUrl) {
        super(description);
        this.typeUrl = typeUrl;
    }

    public InvalidAnyProtocolBufferException(IOException e, String typeUrl) {
        super(e);
        this.typeUrl = typeUrl;
    }

    public InvalidAnyProtocolBufferException(String description, IOException e, String typeUrl) {
        super(description, e);
        this.typeUrl = typeUrl;
    }
}
