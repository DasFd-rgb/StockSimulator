#ifndef PRESENTATION_H
#define PRESENTATION_H
#include "../shared/include/session.h"

bool presentation_command(Game *game, char *const *tokens, size_t count, RunControl *control,
                          bool *changed, char *message, size_t size);
bool presentation_dispatch(Session *app);
#endif
