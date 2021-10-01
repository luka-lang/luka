#ifndef LUKA_CORE_H
#define LUKA_CORE_H

#include "defs.h"
#include "logger.h"

bool CORE_initialize_builtins(t_logger *logger);

t_ast_node *CORE_lookup_builtin(const t_ast_node *builtin);

#endif /* LUKA_CORE_H */
