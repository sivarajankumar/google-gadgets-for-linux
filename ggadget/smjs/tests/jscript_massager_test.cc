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

#include "ggadget/smjs/jscript_massager.h"
#include "unittest/gunit.h"

const char *input =
  "options.item(a(b(c))) = d(e(f) + g)\n"
  "options.item(a(b(c\n"
  "))) =\n"
  "d(\n"
  "e(f) +\n"
  "g)\n"
  "options(a) = b;\n"
  "xxoptions(a) = b;\n"
  "optionsHaha(xy(z)) = options.item(x(y) - z);\n"
  "a + (options.item(a) = a);\n"
  "a + (options.item(a(b) * a) = a(b) / a);\n"
  "options.item(a)\n"
  " = a\n"
  "+ options.item((options.item(options.item(d)))) +\n"
  "c\n"
  ".d.\n"
  "e(f)\n"
  "options.ITEM(a /* comment */ ) /* comment */ // comments\n"
  " = a /* comment */\n"
  "/* comment\n"
  "*/\n"
  "+ options.item((options.item(options.item(d)))) +\n"
  "c\n"
  ".d.\n"
  "e(f)\n"
  "  options.item(a + options.item(x(options.item(k)))) =\n"
  " options.item(d(e(f)));\n"
  "options.defaultValue(a - (options.item(z(y) ^ k) = options.item(m))) = xyz;\n"
  "options.DEFAULTVALUE(a) = options.item(b)\n"
  " = options.item(c);\n"
  "options.defaultValue(a) = options.item(b) = options.item(c) = d\n"
  " = e;\n"
  "options.defaultValue(a) = options.item(b) = options.item(c) = d(ef)\n"
  " = e;\n"
  "options.defaultValue(a) = options.item(b) = options.item(c) = d(ef) =\n"
  " e;\n"
  "options.item(x) = function(a,b,c,d) {\n"
  "  if (a) {\n"
  "    options.item(z) = options.item(y)\n"
  "  } else {\n"
  "    options.item(k[x]) = options.item(k[z]);\n"
  "  }\n"
  "}\n"
  "if (a) {options.item(z) = options.item(y)}; else {options.item[k[x]) = options.item(k[z]);}\n"
  "if (a) {options.item(z) = options.item(y)\n"
  "}; else {options.item[k[x]) = options.item(k[z]);}\n"
  "\n"
  "options.item(k) = { a:b; b:[1\n"
  ",2,3], c:{d:e}}, \n"
  "options.item(k) = { a:b; b:[1\n"
  ",2,3], c:{d:e}} \n"
  "\n"
  "/*  comment:\n"
  " \" options.item(a) = b to options.item(a, b)\n"
  "*/\n"
  "// \"\n"
  "options.item(\"options.item(\\\"xyz'\\\"))))\\\"\\\"=\\\n"
  " ds options.kd\") = \"sjdfoptions.item(\\\"slkdjf\\\"\";\n"
  "options.item('options.item(\\xyz\"))))\\\"\\'= ds\\\n"
  " options.kd') = 'sjdfoptions.item(\\'slkdjf\"'; \n"
  "function Options(options, item) {\n"
  "  this.options = options;\n"
  "  this.options(item) = item;\n"
  "}\n"
  ;

const char *output =
  "options.putValue(a(b(c)),  d(e(f) + g))\n"
  "options.putValue(a(b(c\n"
  ")), \n"
  "d(\n"
  "e(f) +\n"
  "g))\n"
  "options.putValue(a,  b);\n"
  "xxoptions(a) = b;\n"
  "optionsHaha(xy(z)) = options.item(x(y) - z);\n"
  "a + (options.putValue(a,  a));\n"
  "a + (options.putValue(a(b) * a,  a(b) / a));\n"
  "options.putValue(a,\n"
  "  a\n"
  "+ options.item((options.item(options.item(d)))) +\n"
  "c\n"
  ".d.\n"
  "e(f))\n"
  "options.putValue(a /* comment */ , /* comment */ // comments\n"
  "  a /* comment */\n"
  "/* comment\n"
  "*/\n"
  "+ options.item((options.item(options.item(d)))) +\n"
  "c\n"
  ".d.\n"
  "e(f))\n"
  "  options.putValue(a + options.item(x(options.item(k))), \n"
  " options.item(d(e(f))));\n"
  "options.putDefaultValue(a - (options.putValue(z(y) ^ k,  options.item(m))),  xyz);\n"
  "options.putDefaultValue(a,  options.putValue(b,\n"
  "  options.item(c)));\n"
  "options.putDefaultValue(a,  options.putValue(b,  options.putValue(c,  d\n"
  " = e)));\n"
  "options.putDefaultValue(a,  options.putValue(b,  options.putValue(c,  d(ef)\n"
  " = e)));\n"
  "options.putDefaultValue(a,  options.putValue(b,  options.putValue(c,  d(ef) =\n"
  " e)));\n"
  "options.putValue(x,  function(a,b,c,d) {\n"
  "  if (a) {\n"
  "    options.putValue(z,  options.item(y))\n"
  "  } else {\n"
  "    options.putValue(k[x],  options.item(k[z]));\n"
  "  }\n"
  "})\n"
  "if (a) {options.putValue(z,  options.item(y)}  else {options.item[k[x]) = options.item(k[z]);})\n"
  "if (a) {options.putValue(z,  options.item(y))\n"
  "}  else {options.item[k[x]) = options.item(k[z]);}\n"
  "\n"
  "options.putValue(k,  { a:b; b:[1\n"
  ",2,3], c:{d:e}}), \n"
  "options.putValue(k,  { a:b; b:[1\n"
  ",2,3], c:{d:e}} )\n"
  "\n"
  "/*  comment:\n"
  " \" options.item(a) = b to options.item(a, b)\n"
  "*/\n"
  "// \"\n"
  "options.putValue(\"options.item(\\\"xyz'\\\"))))\\\"\\\"=\\\n"
  " ds options.kd\",  \"sjdfoptions.item(\\\"slkdjf\\\"\");\n"
  "options.putValue('options.item(\\xyz\"))))\\\"\\'= ds\\\n"
  " options.kd',  'sjdfoptions.item(\\'slkdjf\"'); \n"
  "function Options(options, item) {\n"
  "  this.options = options;\n"
  "  this.options.putValue(item,  item);\n"
  "}\n"
  ;

TEST(JScriptMassager, Normal) {
  std::string result = ggadget::smjs::MassageJScript(input, "filename", 1);
  ASSERT_STREQ(output, result.c_str());
}

const char *invalid_input1 =
  "options.item[a(]) = b;\n"
  "options.item(a,b(((\n"
  ;
const char *invalid_output1 =
  "options.item[a(]) = b;\n"
  "options.item(a,b(((\n"
  ;

TEST(JScriptMassager, Invalid1) {
  std::string result = ggadget::smjs::MassageJScript(invalid_input1,
                                                     "filename", 1);
  ASSERT_STREQ(invalid_output1, result.c_str());
}

const char *invalid_input2 =
  "options.item(a[]]]) = b;\n"
  "options.item(a) = options.item(b)));\n"
  ;
const char *invalid_output2 =
  "options.item(a[]]]) = b;\n"
  "options.putValue(a,  options.item(b))));\n"
  ;

TEST(JScriptMassager, Invalid2) {
  std::string result = ggadget::smjs::MassageJScript(invalid_input2,
                                                     "filename", 1);
  ASSERT_STREQ(invalid_output2, result.c_str());
}

const char *invalid_input3 =
  "options.item(a) = options.item((((b);\n"
  "options.item(a) = options.item(b);\n"
  ;
const char *invalid_output3 =
  "options.putValue(a,  options.item((((b);\n"
  "options.putValue(a,  options.item(b));\n"
  ")"
  ;

TEST(JScriptMassager, Invalid3) {
  std::string result = ggadget::smjs::MassageJScript(invalid_input3,
                                                     "filename", 1);
  ASSERT_STREQ(invalid_output3, result.c_str());
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
