//
// Created by Larry Tin.
//

#import "GPBMessage.h"

@interface GPBMessage (JsonFormat)

/// Creates a new instance by parsing Json format. If
/// there is an error the method returns nil and the error is returned in
/// errorPtr (when provided).
///
/// @param json     The json to parse.
/// @param errorPtr An optional error pointer to fill in with a failure reason if
///                 the data can not be parsed.
///
/// @return A new instance of the class messaged.
+ (instancetype)parseFromJson:(nullable NSDictionary *)json error:(NSError **)errorPtr;

/// Parses json patches of this type and merges it with this message.
///
/// @note This will throw if there is an error parsing the data.
- (void)mergeFromJson:(nullable NSDictionary *)json;

/// Serializes the message to Json.
///
/// If there is an error while generating the data, nil is returned.
///
/// @note This value is not cached, so if you are using it repeatedly, cache
///       it yourself.
- (nullable NSDictionary *)toJson;

/// Merges the fields from another message (of the same type) into this
/// message.
- (void)mergeFrom:(id)other;

+ (NSDictionary *)printMessage:(GPBMessage *)msg useTextFormatKey:(BOOL)useTextFormatKey;

@end