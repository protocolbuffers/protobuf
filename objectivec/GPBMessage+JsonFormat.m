//
// Created by Larry Tin.
//

#import "GPBMessage+JsonFormat.h"
#import "GPBDescriptor.h"
#import "GPBArray.h"
#import "GPBUtilities.h"
#import "GPBDictionary_PackagePrivate.h"
#import "GPBUtilities_PackagePrivate.h"

static NSString *const arraySuffix = @"Array";

@implementation GPBMessage (JsonFormat)

+ (instancetype)parseFromJson:(nullable NSDictionary *)json error:(NSError **)errorPtr {
  @try {
    GPBMessage *msg = [[self alloc] init];
    [self merge:json message:msg ignoreDefaultValue:YES];
    if (errorPtr) {
      *errorPtr = nil;
    }
    return msg;
  } @catch (NSException *exception) {
    if (errorPtr) {
      NSDictionary *userInfo = exception.reason.length ? @{@"Reason": exception.reason} : nil;
      *errorPtr = [NSError errorWithDomain:GPBMessageErrorDomain code:-105 userInfo:userInfo];
    }
  }
  return nil;
}

- (void)mergeFromJson:(nullable NSDictionary *)json {
  [self.class merge:json message:self ignoreDefaultValue:NO];
}

- (NSDictionary *)toJson {
  return [self.class printMessage:self useTextFormatKey:NO];
}

#pragma mark  Parses from JSON into a protobuf message.

+ (void)merge:(nullable NSDictionary *)json message:(GPBMessage *)msg ignoreDefaultValue:(BOOL)ignoreDefVal {
  if (!json || json == NSNull.null || ![json isKindOfClass:NSDictionary.class]) {
    if (!ignoreDefVal) {
      [msg clear];
    }
    return;
  }
  GPBDescriptor *descriptor = [msg.class descriptor];
  for (NSString *key in json) {
    id val = json[key];
    if (ignoreDefVal && val == NSNull.null) {
      continue;
    }
    GPBFieldDescriptor *field = [descriptor fieldWithName:key];
    field = field ?: [self fieldWithTextFormatName:key inDescriptor:descriptor];
    if (!field) {
      if (![key hasSuffix:arraySuffix]) {
        field = [descriptor fieldWithName:[key stringByAppendingString:arraySuffix]];
        if (field.fieldType != GPBFieldTypeRepeated) {
          field = nil;
        }
      }
      if (!field) {
        // message doesn't have the field set, on to the next.
        continue;
      }
    }
    [self mergeField:field json:val message:msg ignoreDefaultValue:ignoreDefVal];
  }
}

+ (void)mergeField:(GPBFieldDescriptor *)field json:(nonnull id)json message:(GPBMessage *)msg ignoreDefaultValue:(BOOL)ignoreDefVal {
  switch (field.fieldType) {
    case GPBFieldTypeSingle:
      [self parseFieldValue:field json:json message:msg ignoreDefaultValue:ignoreDefVal];
      break;
    case GPBFieldTypeRepeated:
      [self mergeRepeatedField:field json:json message:msg ignoreDefaultValue:ignoreDefVal];
      break;
    case GPBFieldTypeMap:
      [self mergeMapField:field json:json message:msg ignoreDefaultValue:ignoreDefVal];
      break;
    default:
      NSCAssert(NO, @"Can't happen");
      break;
  }
}

+ (void)parseFieldValue:(GPBFieldDescriptor *)field json:(nonnull id)json message:(GPBMessage *)msg ignoreDefaultValue:(BOOL)ignoreDefVal {
  switch (field.dataType) {
    case GPBDataTypeBool:
    case GPBDataTypeSFixed32:
    case GPBDataTypeInt32:
    case GPBDataTypeSInt32:
    case GPBDataTypeFixed32:
    case GPBDataTypeUInt32:
    case GPBDataTypeSFixed64:
    case GPBDataTypeInt64:
    case GPBDataTypeSInt64:
    case GPBDataTypeFixed64:
    case GPBDataTypeUInt64:
    case GPBDataTypeFloat:
    case GPBDataTypeDouble:
      json = [self canonicalValue:json field:field toJson:NO];
      if (ignoreDefVal && [json isEqualToNumber:@(0)]) {
        return;
      }
      [msg setValue:json forKey:field.name];
      break;
    case GPBDataTypeEnum:
      json = [self canonicalValue:json field:field toJson:NO];
      if (ignoreDefVal && [json isEqualToNumber:@(0)]) {
        return;
      }
      GPBSetInt32IvarWithFieldInternal(msg, field, [json intValue], msg.class.descriptor.file.syntax);
      break;
    case GPBDataTypeBytes:
    case GPBDataTypeString: {
      json = [self canonicalValue:json field:field toJson:NO];
      if (ignoreDefVal && [json length] == 0) {
        return;
      }
      [msg setValue:json forKey:field.name];
      break;
    }
    case GPBDataTypeGroup:
    case GPBDataTypeMessage: {
      GPBMessage *message = [msg valueForKey:field.name];
      [self merge:json message:message ignoreDefaultValue:ignoreDefVal];
      break;
    }
    default:
      NSCAssert(NO, @"Can't happen");
      break;
  }
}

+ (void)mergeRepeatedField:(GPBFieldDescriptor *)field json:(nonnull id)json message:(GPBMessage *)msg ignoreDefaultValue:(BOOL)ignoreDefVal {
  if (json == NSNull.null || ![json isKindOfClass:NSArray.class]) {
    if (!ignoreDefVal) {
      [msg setValue:nil forKey:field.name];
    }
    return;
  }
  if ([json count] <= 0) {
    return;
  }
  id genericArray = [msg valueForKey:field.name];
  for (id ele in json) {
    if (ignoreDefVal && ele == NSNull.null) {
      continue;
    }
    switch (field.dataType) {
      case GPBDataTypeBool:
      case GPBDataTypeSFixed32:
      case GPBDataTypeInt32:
      case GPBDataTypeSInt32:
      case GPBDataTypeFixed32:
      case GPBDataTypeUInt32:
      case GPBDataTypeSFixed64:
      case GPBDataTypeInt64:
      case GPBDataTypeSInt64:
      case GPBDataTypeFixed64:
      case GPBDataTypeUInt64:
      case GPBDataTypeFloat:
      case GPBDataTypeDouble: {
        NSNumber *val = [self canonicalValue:ele field:field toJson:NO];
        [(GPBInt32Array *) genericArray addValue:[val doubleValue]];
        break;
      }
      case GPBDataTypeEnum: {
        NSNumber *val = [self canonicalValue:ele field:field toJson:NO];
        [(GPBEnumArray *) genericArray addRawValue:[val intValue]];
        break;
      }
      case GPBDataTypeBytes:
      case GPBDataTypeString: {
        id val = [self canonicalValue:ele field:field toJson:NO];
        [(NSMutableArray *) genericArray addObject:val];
        break;
      }
      case GPBDataTypeGroup:
      case GPBDataTypeMessage: {
        GPBMessage *val = [[field.msgClass alloc] init];
        if (ele != NSNull.null) {
          [self merge:ele message:val ignoreDefaultValue:ignoreDefVal];
        }
        [(NSMutableArray *) genericArray addObject:val];
        break;
      }
      default:
        NSCAssert(NO, @"Can't happen");
        break;
    }
  }
}

+ (void)mergeMapField:(GPBFieldDescriptor *)field json:(nonnull id)json message:(GPBMessage *)msg ignoreDefaultValue:(BOOL)ignoreDefVal {
  if (json == NSNull.null || ![json isKindOfClass:NSDictionary.class]) {
    if (!ignoreDefVal) {
      [msg setValue:nil forKey:field.name];
    }
    return;
  }
  id map = [msg valueForKey:field.name];
  GPBDataType keyDataType = field.mapKeyDataType;
  GPBDataType valueDataType = field.dataType;
  if (keyDataType == GPBDataTypeString && GPBDataTypeIsObject(valueDataType)) {
    // Cases where keys are strings and values are strings, bytes, or messages are handled by NSMutableDictionary.
    for (NSString *key in json) {
      id value = json[key];
      if (ignoreDefVal && value == NSNull.null) {
        continue;
      }
      id val;
      switch (valueDataType) {
        case GPBDataTypeGroup:
        case GPBDataTypeMessage:
          val = ((NSMutableDictionary *) map)[key] ?: [[field.msgClass alloc] init];
          if (value != NSNull.null) {
            [self merge:value message:val ignoreDefaultValue:ignoreDefVal];
          } else {
            [(GPBMessage *) val clear];
          }
          break;
        case GPBDataTypeBytes:
        case GPBDataTypeString:
        default:
          val = [self canonicalValue:value field:field toJson:NO];
          break;
      }
      ((NSMutableDictionary *) map)[key] = val;
    }
    return;
  }
  // Other cases are: GPB<KEY><VALUE>Dictionary
  for (NSString *key in json) {
    id value = json[key];
    if (ignoreDefVal && value == NSNull.null) {
      continue;
    }
    GPBGenericValue genericKey = [self readValue:key dataType:keyDataType field:field];
    GPBGenericValue genericValue = field.defaultValue;
    if (value != NSNull.null) {
      if (valueDataType == GPBDataTypeMessage) {
        int numKey = key.intValue;
        GPBMessage *msgVal = [(GPBUInt32ObjectDictionary *) map objectForKey:numKey];
        msgVal = msgVal ?: [[field.msgClass alloc] init];
        [self merge:value message:msgVal ignoreDefaultValue:ignoreDefVal];
        genericValue.valueMessage = msgVal;
        // must hold msgVal which is declared in a if block until the use of outside scope(setGPBGenericValue:forGPBGenericValueKey)
        value = msgVal;
      } else {
        genericValue = [self readValue:value dataType:valueDataType field:field];
      }
    }
    [map setGPBGenericValue:&genericValue forGPBGenericValueKey:&genericKey];
  }
}

+ (GPBFieldDescriptor *)fieldWithTextFormatName:(NSString *)name inDescriptor:(GPBDescriptor *)descriptor {
  for (GPBFieldDescriptor *field in descriptor.fields) {
    if ([field.textFormatName isEqual:name]) {
      return field;
    }
  }
  return nil;
}

+ (GPBGenericValue)readValue:(id)val dataType:(GPBDataType)type field:(GPBFieldDescriptor *)field {
  GPBGenericValue valueToFill;
  switch (type) {
    case GPBDataTypeBool:
      valueToFill.valueBool = [val boolValue];
      break;
    case GPBDataTypeSFixed32:
      valueToFill.valueInt32 = [val intValue];
      break;
    case GPBDataTypeEnum: {
      int32_t outValue = 0;
      if ([val isKindOfClass:NSNumber.class]) {
        outValue = [val intValue];
      } else {
        [field.enumDescriptor getValue:&outValue forEnumTextFormatName:val];
      }
      valueToFill.valueEnum = outValue;
      break;
    }
    case GPBDataTypeInt32:
      valueToFill.valueInt32 = [val intValue];
      break;
    case GPBDataTypeSInt32:
      valueToFill.valueInt32 = [val intValue];
      break;
    case GPBDataTypeFixed32:
      valueToFill.valueUInt32 = [val unsignedIntValue];
      break;
    case GPBDataTypeUInt32:
      valueToFill.valueUInt32 = [val unsignedIntValue];
      break;
    case GPBDataTypeSFixed64:
      valueToFill.valueInt64 = [val longLongValue];
      break;
    case GPBDataTypeInt64:
      valueToFill.valueInt64 = [val longLongValue];
      break;
    case GPBDataTypeSInt64:
      valueToFill.valueInt64 = [val longLongValue];
      break;
    case GPBDataTypeFixed64:
      valueToFill.valueUInt64 = [val unsignedLongLongValue];
      break;
    case GPBDataTypeUInt64:
      valueToFill.valueUInt64 = [val unsignedLongLongValue];
      break;
    case GPBDataTypeFloat:
      valueToFill.valueFloat = [val floatValue];
      break;
    case GPBDataTypeDouble:
      valueToFill.valueDouble = [val doubleValue];
      break;
    case GPBDataTypeBytes:
      valueToFill.valueData = [[NSData alloc] initWithBase64EncodedString:val options:0];
      break;
    case GPBDataTypeString:
      valueToFill.valueString = [val copy];
      break;
    case GPBDataTypeMessage:
    case GPBDataTypeGroup:
    default:
      NSCAssert(NO, @"Can't happen");
      break;
  }
  return valueToFill;
}

#pragma mark  Converts protobuf message to JSON format.

+ (NSDictionary *)printMessage:(GPBMessage *)msg useTextFormatKey:(BOOL)useTextFormatKey {
  if (![msg isKindOfClass:GPBMessage.class]) {
    return NSNull.null;
  }
  NSMutableDictionary *json = [NSMutableDictionary dictionary];
  GPBDescriptor *descriptor = [msg.class descriptor];
  for (GPBFieldDescriptor *field in descriptor.fields) {
    if (!GPBMessageHasFieldSet(msg, field)) {
      // Nothing to print, out of here.
      continue;
    }
    id jsonVal = [self printField:field msg:msg useTextFormatKey:useTextFormatKey];
    NSString *name = useTextFormatKey ? field.textFormatName : field.name;
    if (!useTextFormatKey && field.fieldType == GPBFieldTypeRepeated) {
      name = [name substringToIndex:name.length - arraySuffix.length];
    }
    json[name] = jsonVal;
  }
  return json;
}

+ (id)printField:(GPBFieldDescriptor *)field msg:(GPBMessage *)msg useTextFormatKey:(BOOL)useTextFormatKey {
  switch (field.fieldType) {
    case GPBFieldTypeSingle:
      return [self printSingleFieldValue:field msg:msg useTextFormatKey:useTextFormatKey];
    case GPBFieldTypeRepeated:
      return [self printRepeatedFieldValue:field value:[msg valueForKey:field.name] useTextFormatKey:useTextFormatKey];
    case GPBFieldTypeMap:
      return [self printMapFieldValue:field value:[msg valueForKey:field.name] useTextFormatKey:useTextFormatKey];
    default:
      NSCAssert(NO, @"Can't happen");
      break;
  }
  return nil;
}

+ (id)printSingleFieldValue:(GPBFieldDescriptor *)field msg:(GPBMessage *)msg useTextFormatKey:(BOOL)useTextFormatKey {
  switch (field.dataType) {
    case GPBDataTypeMessage:
    case GPBDataTypeGroup:
      return [self printMessage:[msg valueForKey:field.name] useTextFormatKey:useTextFormatKey];
    case GPBDataTypeEnum:
      return [self canonicalValue:@(GPBGetMessageInt32Field(msg, field)) field:field toJson:YES];
    default:
      return [self canonicalValue:[msg valueForKey:field.name] field:field toJson:YES];
  }
}

+ (id)printRepeatedFieldValue:(GPBFieldDescriptor *)field value:(nonnull id)arrayVal useTextFormatKey:(BOOL)useTextFormatKey {
  NSMutableArray *json = [NSMutableArray array];
  switch (field.dataType) {
    case GPBDataTypeBool:
    case GPBDataTypeDouble:
    case GPBDataTypeFixed32:
    case GPBDataTypeFixed64:
    case GPBDataTypeFloat:
    case GPBDataTypeInt32:
    case GPBDataTypeInt64:
    case GPBDataTypeSFixed32:
    case GPBDataTypeSFixed64:
    case GPBDataTypeSInt32:
    case GPBDataTypeSInt64:
    case GPBDataTypeUInt32:
    case GPBDataTypeUInt64: {
      if (![arrayVal respondsToSelector:@selector(valueAtIndex:)]) {
        return NSNull.null;
      }
      // The exact type doesn't matter, they all implement -valueAtIndex:.
      GPBInt32Array *array = (GPBInt32Array *) arrayVal;
      for (NSUInteger i = 0; i < array.count; i++) {
        [json addObject:@([array valueAtIndex:i])];
      }
      break;
    }
    case GPBDataTypeEnum: {
      if (![arrayVal isKindOfClass:GPBEnumArray.class]) {
        return NSNull.null;
      }
      [(GPBEnumArray *) arrayVal enumerateRawValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
          [json addObject:[self canonicalValue:@(value) field:field toJson:YES]];
      }];
      break;
    }
    case GPBDataTypeBytes:
    case GPBDataTypeString:
      if (![arrayVal isKindOfClass:NSArray.class]) {
        return NSNull.null;
      }
      for (id ele in arrayVal) {
        [json addObject:[self canonicalValue:ele field:field toJson:YES]];
      }
      break;
    case GPBDataTypeGroup:
    case GPBDataTypeMessage:
      if (![arrayVal isKindOfClass:NSArray.class]) {
        return NSNull.null;
      }
      for (id ele in arrayVal) {
        [json addObject:[self printMessage:ele useTextFormatKey:useTextFormatKey]];
      }
      break;
    default:
      NSCAssert(NO, @"Can't happen");
      break;
  }
  return json;
}

+ (id)printMapFieldValue:(GPBFieldDescriptor *)field value:(nonnull id)mapVal useTextFormatKey:(BOOL)useTextFormatKey {
  NSMutableDictionary *json = [NSMutableDictionary dictionary];
  GPBDataType keyDataType = field.mapKeyDataType;
  GPBDataType valueDataType = field.dataType;
  if (keyDataType == GPBDataTypeString && GPBDataTypeIsObject(valueDataType)) {
    // Cases where keys are strings and values are strings, bytes, or messages are handled by NSMutableDictionary.
    if (![mapVal isKindOfClass:NSDictionary.class]) {
      return NSNull.null;
    }
    for (NSString *key in mapVal) {
      id jsonVal;
      switch (valueDataType) {
        case GPBDataTypeGroup:
        case GPBDataTypeMessage:
          jsonVal = [self printMessage:mapVal[key] useTextFormatKey:useTextFormatKey];
          break;
        case GPBDataTypeBytes:
        case GPBDataTypeString:
        default:
          jsonVal = [self canonicalValue:mapVal[key] field:field toJson:YES];
          break;
      }
      json[key] = jsonVal;
    }
    return json;
  }

  // Other cases are: GPB<KEY><VALUE>Dictionary
  // The exact type doesn't matter, they all implement -enumerateForTextFormat:.
  if (![mapVal respondsToSelector:@selector(enumerateForTextFormat:)]) {
    return NSNull.null;
  }
  [(GPBStringInt32Dictionary *) mapVal enumerateForTextFormat:^(id keyObj, id valueObj) {
      switch (field.dataType) {
        case GPBDataTypeGroup:
        case GPBDataTypeMessage:
          valueObj = [valueObj toJson];
          break;
        case GPBDataTypeEnum:
        default:
          valueObj = [self canonicalValue:valueObj field:field toJson:YES];
          break;
      }
      json[keyObj] = valueObj;
  }];
  return json;
}

#pragma mark Exception assert

+ (id)canonicalValue:(nonnull id)value field:(GPBFieldDescriptor *)field toJson:(BOOL)toJson {
  switch (field.dataType) {
    case GPBDataTypeBool:
    case GPBDataTypeDouble:
    case GPBDataTypeFloat:
    case GPBDataTypeFixed32:
    case GPBDataTypeFixed64:
    case GPBDataTypeInt32:
    case GPBDataTypeInt64:
    case GPBDataTypeSFixed32:
    case GPBDataTypeSFixed64:
    case GPBDataTypeSInt32:
    case GPBDataTypeSInt64:
    case GPBDataTypeUInt32:
    case GPBDataTypeUInt64:
      if (toJson) {
        return value;
      }
      if ([value isKindOfClass:NSNumber.class]) {
        return value;
      }
      if ([value isKindOfClass:NSString.class]) {
        switch (field.dataType) {
          case GPBDataTypeBool:
            return @([value isEqualToString:@"true"]);
          case GPBDataTypeDouble:
            return @(((NSString *) value).doubleValue);
          case GPBDataTypeFloat:
            return @(((NSString *) value).floatValue);
        }
        return @(((NSString *) value).longLongValue);
      }
      return @0;
    case GPBDataTypeEnum:
      if (toJson) {
        return [field.enumDescriptor textFormatNameForValue:[value intValue]] ?: value;
      }
      // return a int
      if ([value isKindOfClass:NSString.class]) {
        int32_t outValue = 0;
        [field.enumDescriptor getValue:&outValue forEnumTextFormatName:value];
        if (outValue != 0) {
          return @(outValue);
        }
        NSCharacterSet *notDigits = [[NSCharacterSet decimalDigitCharacterSet] invertedSet];
        if ([value rangeOfCharacterFromSet:notDigits].location == NSNotFound) {
          // value consists only of the digits 0 through 9
          return @([value intValue]);
        }
        return @0;
      }
      if ([value isKindOfClass:NSNumber.class]) {
        return value;
      }
      return @0;
    case GPBDataTypeBytes:
      if (toJson) {
        // return a string
        if ([value isKindOfClass:NSData.class]) {
          return [(NSData *) value base64EncodedStringWithOptions:0];
        }
        if ([value isKindOfClass:NSString.class]) {
          return value;
        }
        return NSNull.null;
      }
      // return NSData
      if ([value isKindOfClass:NSString.class]) {
        return [[NSData alloc] initWithBase64EncodedString:value options:0];
      }
      if ([value isKindOfClass:NSData.class]) {
        return value;
      }
      return NSData.data;
    case GPBDataTypeString:
      if ([value isKindOfClass:NSString.class]) {
        return value;
      }
      if ([value isKindOfClass:NSNumber.class]) {
        return [value stringValue];
      }
      return toJson ? NSNull.null : @"";
    case GPBDataTypeMessage:
    case GPBDataTypeGroup:
      if (toJson) {
        return [value isKindOfClass:GPBMessage.class] ? value : NSNull.null;
      }
      return [value isKindOfClass:NSDictionary.class] ? value : nil;
    default:
      NSCAssert(NO, @"Can't happen");
      break;
  }
  return nil;
}

//+ (void)assert:(id)value isKindOfClass:(Class)clz {
//  if (![value isKindOfClass:clz]) {
//    [NSException raise:NSInvalidArgumentException format:@"[%@]Invalid %@ value: %@", NSStringFromClass(self.class), NSStringFromClass(clz), value];
//  }
//}
@end