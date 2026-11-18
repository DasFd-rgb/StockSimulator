/* session.h: main이 사용하는 실행 상태와 보조 함수입니다.
 * 큰 Game은 동적 메모리에 한 번 할당합니다. 구조체 주소를 전달해 같은 상태를 사용합니다. */
#ifndef SESSION_H
#define SESSION_H
#include "gameData.h"
#include "screenOut.h"
#include "input.h"

typedef struct
{
    uint32_t interval_ms; /* 가격 갱신 간격: 일반 10000, 시연 3000 밀리초 */
    bool paused;
} RunControl;

typedef struct
{
    Game game;
    ScreenState screen;
    InputLine input;
    const char *save_path;
    RunControl control;
    char notice[512];
    uint64_t last_clock, last_checkpoint;
    bool save_failed, redraw, quit, fatal;
    bool restart_enabled; /* 일반용 main.c가 다시하기 명령을 켭니다. */
    bool restart_pending; /* restart를 받은 뒤에만 restart yes를 허용합니다. */
} Session;

bool session_open(Session *app, uint32_t interval_ms, uint64_t seed);
void session_close(void);
bool session_checkpoint(Session *app);
void session_advance_time(Session *app);
void session_command(Session *app);
bool session_stop_requested(void);
#endif
