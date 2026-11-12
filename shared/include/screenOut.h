/** screenOut.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. */
#ifndef SCREEN_OUT_H
#define SCREEN_OUT_H
#include "gameData.h"
typedef struct
{
    ViewMode view;
    SortMode sort;
    bool descending, watched_only;
    size_t page;
    uint64_t selected_id;
    char filter[64];
    const char *title;      /* 실행 프로그램이 제공하는 화면 제목 */
    const char *footer;     /* 실행 프로그램별 하단 사용 안내 */
    const char *extra_help; /* 별도 진입 프로그램이 제공하는 추가 도움말 */
    bool paused;            /* 현재 시간 진행을 멈췄는지 표시 */
    uint32_t interval_ms;   /* 도움말에 표시할 실제 갱신 간격(밀리초) */
} ScreenState;
/* 가능하면 전용 Windows 콘솔 화면 버퍼를 만들고 활성화합니다. ANSI 지우기 명령에 의존하지 않습니다. 새 버퍼 생성 실패 시 기존 콘솔 버퍼에 고정 위치로 출력합니다. */
void screen_init(void);
/* 원래 콘솔 화면으로 복귀하고 생성한 화면 버퍼를 닫습니다. 프로그램 종료 처리에서 호출합니다. */
void screen_shutdown(void);
/* 실제 콘솔 화면을 사용하는지 반환합니다. 파일·파이프로 출력할 때는 매초 전체 화면을 로그로 쌓지 않도록 타이머 화면 갱신을 구분합니다. */
bool screen_interactive(void);
/* Game을 읽기만 하여 현재 화면의 행들을 구성하고 출력합니다. 내부 StockType은 표시하지 않습니다. 화면 모드와 페이지에 따라 목록·상세·자산·주문·거래·자산 기록·도움말을 만듭니다. */
void screen_render(const Game *game, const ScreenState *state, const char *notice,
                   const char *input, bool save_failed, const char *save_path);
const char *format_money(int64_t value, char buffer[48]);
#endif
