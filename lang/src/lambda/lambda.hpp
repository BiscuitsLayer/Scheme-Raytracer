#pragma once

#include <optional>
#include <cstdint>

#include <helpers/helpers.hpp>
#include <scope/scope.hpp>
#include <object/object.hpp>

namespace Interp {

ObjectPtr BuildLambda(std::optional<std::string> name, ObjectPtr init, ScopePtr scope, bool eval_immediately = false);
std::pair<std::string, ObjectPtr> BuildLambdaSugar(std::vector<ObjectPtr> parts, ScopePtr scope);

} // namespace Interp

namespace Codegen {

ObjectPtr BuildLambdaCodegen(std::optional<std::string> name, ObjectPtr init, ScopePtr scope, bool eval_immediately = false);

} // namespace Codegen