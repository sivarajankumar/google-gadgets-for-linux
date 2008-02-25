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

#ifndef GGADGET_SMJS_JSCRIPT_MASSAGER_H_
#define GGADGET_SMJS_JSCRIPT_MASSAGER_H_

#include <string>
namespace ggadget {
namespace smjs {

std::string MassageJScript(const char *input,
                           const char *filename, int lineno);

} // namespace smjs
} // namespace ggadget

#endif // GGADGET_SMJS_JSCRIPT_MASSAGER_H_