/*
  Copyright 2007 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <cstring>
#include "json.h"

namespace ggadget {

static void AppendJSON(JSContext *cx, jsval js_val, std::string *json);

static void AppendArrayToJSON(JSContext *cx, JSObject *array,
                              std::string *json) {
  (*json) += '[';
  jsuint length = 0;
  JS_GetArrayLength(cx, array, &length);
  for (jsuint i = 0; i < length; i++) {
    jsval value = JSVAL_NULL;
    JS_GetElement(cx, array, static_cast<jsint>(i), &value);
    AppendJSON(cx, value, json);
    if (i != length - 1)
      (*json) += ',';
  }
  (*json) += ']';
}

static void AppendStringToJSON(JSContext *cx, JSString *str,
                               std::string *json) {
  *json += '"';
  jschar *chars = JS_GetStringChars(str);
  if (chars) {
    size_t length = JS_GetStringLength(str);
    for (size_t i = 0; i < length; i++) {
      char buf[10];
      switch (chars[i]) {
        // The following special chars are not so complete, but also works.
        case '"': *json += "\\\""; break;
        case '\\': *json += "\\\\"; break;
        case '\n': *json += "\\n"; break;
        case '\r': *json += "\\r"; break;
        default:
          if (chars[i] >= 0x7f || chars[i] < 0x20) {
            snprintf(buf, sizeof(buf), "\\u%04X", chars[i]);
            *json += buf;
          } else {
            *json += static_cast<char>(chars[i]);
          }
      }
    }
  }
  *json += '"';
}

static void AppendObjectToJSON(JSContext *cx, JSObject *object,
                               std::string *json) {
  (*json) += '{';
  JSIdArray *id_array = JS_Enumerate(cx, object);
  JSObject *prototype = JS_GetPrototype(cx, object);
  if (id_array) {
    for (int i = 0; i < id_array->length; i++) {
      jsid id = id_array->vector[i];
      jsval key = JSVAL_VOID;
      JS_IdToValue(cx, id, &key);
      if (JSVAL_IS_STRING(key)) {
        JSString *key_str = JSVAL_TO_STRING(key);
        jschar *key_chars = JS_GetStringChars(key_str);
        if (key_chars) {
          jsval value = JSVAL_VOID;
          jsval prototype_value = JSVAL_VOID;
          JS_GetUCProperty(cx, object,
                           key_chars, JS_GetStringLength(key_str),
                           &value);
          // Don't output methods.
          if (JS_TypeOfValue(cx, value) != JSTYPE_FUNCTION &&
              // Only output properties defined not in the prototype.
              (!JS_GetUCProperty(cx, prototype,
                                 key_chars, JS_GetStringLength(key_str),
                                 &prototype_value) ||
               prototype_value != value)) {
            AppendStringToJSON(cx, key_str, json);
            (*json) += ':';
            AppendJSON(cx, value, json);
            (*json) += ',';
          }
        }
      }
      // Otherwise, ignore the property.
    }
    // Remove the last extra ','.
    if (json->length() > 0 && *(json->end() - 1) == ',')
      json->erase(json->end() - 1);
  }
  (*json) += '}';
}

static void AppendNumberToJSON(JSContext *cx, jsval js_val, std::string *json) {
  JSString *str = JS_ValueToString(cx, js_val);
  if (str) {
    char *bytes = JS_GetStringBytes(str);
    // Treat Infinity, -Infinity and NaN as zero.
    if (bytes && bytes[0] != 'I' && bytes[1] != 'I' && bytes[0] != 'N')
      *json += bytes;
    else
      *json += '0';
  } else {
    *json += '0';
  }
}

static void AppendJSON(JSContext *cx, jsval js_val, std::string *json) {
  switch (JS_TypeOfValue(cx, js_val)) {
    case JSTYPE_OBJECT: {
      JSObject *obj = JSVAL_TO_OBJECT(js_val);
      if (!obj)
        (*json) += "null";
      else if (JS_IsArrayObject(cx, obj))
        AppendArrayToJSON(cx, obj, json);
      else
        AppendObjectToJSON(cx, obj, json);
      // We don't support Date class now. Neither JSON grammar diagram in
      // http://www.json.org nor RFC 4627 metioned Date.
      break;
    }
    case JSTYPE_STRING:
      AppendStringToJSON(cx, JSVAL_TO_STRING(js_val), json);
      break;
    case JSTYPE_NUMBER:
      AppendNumberToJSON(cx, js_val, json);
      break;
    case JSTYPE_BOOLEAN:
      (*json) += JSVAL_TO_BOOLEAN(js_val) ? "true" : "false";
      break;
    default:
      (*json) += "null";
      break;
  }
}

JSBool JSONEncode(JSContext *cx, jsval js_val, std::string *json) {
  json->clear();
  AppendJSON(cx, js_val, json);
  return JS_TRUE;
}

JSBool JSONDecode(JSContext *cx, const char *json, jsval *js_val) {
  if (!json || !json[0]) {
    *js_val = JSVAL_VOID;
    return JS_TRUE;
  }

  // Valid chars in state 0.
  // Our JSON decoding is stricter than the standard, according to the
  // format we are outputing.
  static const char *kValidChars = ",:{}[]0123456789.-+eE";
  // State: 0: normal; 1: in word; 2: in string.
  int state = 0;
  const char *word_start = json;
  for (const char *p = json; *p; p++) {
    switch (state) {
      case 0:
        if (*p >= 'a' && *p <= 'z') {
          word_start = p;
          state = 1;
        } else if (*p == '"') {
          state = 2;
        } else if (strchr(kValidChars, *p) == NULL) {
          // Invalid JSON format.
          return JS_FALSE;
        }
        break;
      case 1:
        if (*p < 'a' || *p > 'z') {
          state = 0;
          if (strncmp("true", word_start, 4) != 0 &&
              strncmp("false", word_start, 5) != 0 &&
              strncmp("null", word_start, 4) != 0)
            return JS_FALSE;
        }
        break;
      case 2:
        // According to our encoding format, '"' inside of strings are encoded
        // into \x22, so we need not worry about '"'s in the string.
        if (*p == '\\')
          p++;  // Omit the next char.  Also works for \uXXXX cases.
        else if (*p == '"')
          state = 0;
        break;
      default:
        break;
    }
  }

  // Add '()' around the expression to avoid ambiguity of '{}'.
  // See http://www.json.org/json.js.
  std::string json_script(1, '(');
  json_script += json;
  json_script += ')';
  std::string json_filename("JSON:");
  json_filename += json;
  return JS_EvaluateScript(cx, JS_GetGlobalObject(cx),
                           json_script.c_str(), json_script.length(),
                           json_filename.c_str(), 1, js_val);
}

} // namespace ggadget