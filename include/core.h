#ifndef __CORE_H__
#define __CORE_H__

#include "defs.h"
#include "logger.h"

bool CORE_initialize_builtins(t_logger *logger);

t_ast_node *CORE_lookup_builtin(const t_ast_node *builtin);

#endif /* __CORE_H__ */
