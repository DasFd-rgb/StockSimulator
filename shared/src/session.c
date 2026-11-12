/** [모듈 설명]
 * session.c - 실행 준비와 명령 처리 보조
 * 역할: 저장 복원·명령 처리·시간 계산을 돕습니다. 프로그램의 반복문과 입력 흐름은 main.c에 있습니다.
 * 관리 대상: Session 실행 상태와 Game
 * 읽는 순서: 같은 이름의 .h 선언 → 구조체 → 공개 함수 → 내부 보조 함수.
 */
#include "common.h"
#include "company.h"
#include "gameControl.h"
#include "portfolio.h"
#include "order.h"
#include "stockQuery.h"
#include "saveLoad.h"
#include "screenOut.h"
#include "input.h"
#include "session.h"
#include <time.h>
#include <wchar.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static HANDLE game_lock = INVALID_HANDLE_VALUE;
static volatile LONG stop_requested;
/** [학습 설명]
 * 함수: console_handler
 * Ctrl+C/Break를 받으면 종료 요청 표시만 설정합니다. 이 처리기에서 파일을 저장하지 않습니다.
 * 매개변수:
 * - event: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
static BOOL WINAPI console_handler(DWORD event)
{
    if (event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT)
    {
        InterlockedExchange(&stop_requested, 1);
        return TRUE;
    }
    return FALSE;
}
/** [학습 설명]
 * 함수: now_ms
 * 시스템의 단조 증가 시간을 밀리초로 읽습니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
static uint64_t now_ms(void)
{
    return GetTickCount64();
}
/** [학습 설명]
 * 함수: session_close
 * 원래 화면을 복구하고 동시 실행 방지용 뮤텍스를 닫습니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
void session_close(void)
{
    screen_shutdown();
    if (game_lock != INVALID_HANDLE_VALUE)
        CloseHandle(game_lock);
    game_lock = INVALID_HANDLE_VALUE;
}
/** [학습 설명]
 * 함수: lock_save
 * 정규화한 고정 저장 경로로 Windows 이름 있는 뮤텍스를 만듭니다. 같은 데이터의 두 실행을 차단하며 디스크 잠금 파일은 만들지
 * 않습니다.
 * 매개변수:
 * - path: 내부 저장 기준 경로. 일반 사용자가 선택하는 슬롯 인수가 아닙니다.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
static bool lock_save(const char *path)
{
    wchar_t full[2048], name[96];
    uint64_t hash = UINT64_C(14695981039346656037);
    DWORD count = GetFullPathNameW(L"data/game_v4.sav", 2048, full, NULL);
    size_t i;
    (void)path;
    if (!count || count >= 2048)
        return false;
    for (i = 0; full[i]; ++i)
    {
        wchar_t ch = full[i];
        if (ch >= L'A' && ch <= L'Z')
            ch += L'a' - L'A';
        hash ^= (uint64_t)ch;
        hash *= UINT64_C(1099511628211);
    }
    (void)swprintf(name, 96, L"Local\\StockGame_%llu", (unsigned long long)hash);
    game_lock = CreateMutexW(NULL, FALSE, name);
    if (!game_lock)
    {
        game_lock = INVALID_HANDLE_VALUE;
        return false;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(game_lock);
        game_lock = INVALID_HANDLE_VALUE;
        return false;
    }
    return true;
}
/** [학습 설명]
 * 함수: say
 * 가장 최근 안내 문장을 보관하고 화면 갱신을 요청합니다.
 * 매개변수:
 * - app: 시간·입력·화면과 게임을 묶어 보관하는 실행 상태의 주소.
 * - text: 검사하거나 표시할 문자열. 호출자가 내용과 종료 문자를 준비합니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void say(Session *app, const char *text)
{
    (void)snprintf(app->notice, sizeof(app->notice), "%s", text);
    app->redraw = true;
}
/** [학습 설명]
 * 함수: session_checkpoint
 * 실행 중 최근 상태를 복구 파일에 기록합니다. 실패 안내와 다음 재시도 시각을 관리합니다. q의 본 저장 처리와 구분합니다.
 * 매개변수:
 * - app: 시간·입력·화면과 게임을 묶어 보관하는 실행 상태의 주소.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
bool session_checkpoint(Session *app)
{
    char error[256];
    if (!storage_checkpoint(app->save_path, &app->game, error, sizeof(error)))
    {
        app->save_failed = true;
        say(app, error);
        return false;
    }
    if (app->save_failed)
        say(app, "저장이 다시 정상 작동합니다.");
    app->save_failed = false;
    app->last_checkpoint = now_ms();
    return true;
}
/** [학습 설명]
 * 함수: session_advance_time
 * 이전 반복 이후 흐른 밀리초를 빼고 만료되면 정상 시장 갱신을 실행합니다. 한 단계 진행이나 일시정지 뒤의 경과 시간을 잘못 합산하지
 * 않으며, 입력 확정 직전에도 호출합니다.
 * 매개변수:
 * - app: 시간·입력·화면과 게임을 묶어 보관하는 실행 상태의 주소.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
void session_advance_time(Session *app)
{
    uint64_t now = now_ms(), elapsed = now - app->last_clock;
    app->last_clock = now;
    if (portfolio_bankrupt(&app->game) || app->fatal || app->control.paused)
        return;
    while (elapsed >= app->game.remaining_ms)
    {
        TickReport report;
        elapsed -= app->game.remaining_ms;
        if (!game_tick(&app->game, &report))
        {
            app->fatal = true;
            say(app, "시장 갱신에 실패했습니다. 메모리와 계산 범위를 확인하고 저장 후 종료하세요.");
            return;
        }
        app->game.remaining_ms = app->control.interval_ms;
        say(app, report.message);
        (void)session_checkpoint(app);
        if (portfolio_bankrupt(&app->game))
            return;
    }
    app->game.remaining_ms -= (uint32_t)elapsed;
}
/** [학습 설명]
 * 함수: positive
 * 숫자 문자열을 읽은 뒤 1 이상, 명령별 최대값 이하인지 검사합니다.
 * 매개변수:
 * - text: 검사하거나 표시할 문자열. 호출자가 내용과 종료 문자를 준비합니다.
 * - value: 계산·변환할 값 또는 계산 결과를 받을 출력 주소.
 * - maximum: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
static bool positive(const char *text, uint64_t *value, uint64_t maximum)
{
    return input_uint(text, value) && *value > 0 && *value <= maximum;
}
/* 일반용 다시하기 확인 뒤에만 호출합니다.
 * company.c의 game_init으로 새 상태를 준비합니다.
 * saveLoad.c의 storage_checkpoint가 성공해야 현재 게임을 바꿉니다.
 * 저장 순번을 이어 받아야 다음 실행에서 이전 본 저장보다 새 복구본을 우선합니다. */
static void restart_game(Session *app)
{
    char error[256];
    Game *new_game = malloc(sizeof(*new_game));
    if (!new_game)
    {
        say(app, "새 게임 메모리를 확보할 수 없습니다. 다시 시도하세요.");
        return;
    }

    game_init(new_game, (uint64_t)time(NULL) ^ now_ms()); /* company.c: 새 회사와 초기 자산 */
    new_game->remaining_ms = app->control.interval_ms;
    new_game->save_generation = app->game.save_generation;
    if (!storage_checkpoint(app->save_path, new_game, error, sizeof(error)))
    {
        free(new_game);
        say(app, error);
        return;
    }

    app->game = *new_game;
    free(new_game);
    app->last_clock = now_ms();
    app->last_checkpoint = app->last_clock;
    app->save_failed = false;
    app->fatal = false;
    app->screen.view = VIEW_MARKET;
    app->screen.page = 0;
    app->screen.sort = SORT_ID;
    app->screen.descending = false;
    app->screen.watched_only = false;
    app->screen.selected_id = 0;
    app->screen.filter[0] = '\0';
    say(app, "새 게임을 시작했습니다. 초기 현금 10,000,000원입니다.");
}

/* order.c가 거래를 접수합니다. 이 함수는 명령 인수 검사와 안내를 담당합니다.
 * 반환값이 true이면 session_command가 복구 파일을 갱신합니다. */
static bool execute_order(Session *app, char *tokens[], size_t count)
{
    bool limit = tokens[0][0] == 'l';
    TradeSide side = SIDE_SELL;
    size_t expected_count = 3;
    uint64_t id, quantity, price = 0, order_id = 0;
    Result result;

    if (limit)
        expected_count = 4;
    if (!strcmp(tokens[0], "b") || !strcmp(tokens[0], "lb"))
        side = SIDE_BUY;
    if (app->fatal)
    {
        say(app, "시장 갱신이 중단되어 주문할 수 없습니다.");
        return false;
    }
    if (count != expected_count || !positive(tokens[1], &id, UINT64_MAX) ||
        !positive(tokens[2], &quantity, MAX_SHARES) ||
        (limit && !positive(tokens[3], &price, MAX_PRICE)))
    {
        say(app, "시장가: b/s ID 수량 | 지정가: lb/ls ID 수량 가격 (모두 양의 정수)");
        return false;
    }

    if (limit)
        result = order_limit(&app->game, id, side, (int64_t)quantity, (int64_t)price, &order_id);
    else
        result = order_market(&app->game, id, side, (int64_t)quantity);

    if (result == RESULT_OK && limit)
    {
        char text[256];
        (void)snprintf(text, sizeof(text),
                       "주문 ID %" PRIu64 " 접수. 조건 만족 시 현재가로 체결됩니다. o/t로 확인하세요.",
                       order_id);
        say(app, text);
    }
    else
        say(app, result_message(result)); /* portfolio.c: 결과 안내 문구 */

    /* 지정가 처리 과정에서 주문 ID를 사용했다면 그 상태도 복구 기록에 남깁니다. */
    return result == RESULT_OK || (limit && order_id != 0);
}
/** [학습 설명]
 * 함수: session_command
 * 입력 토큰의 개수·숫자 범위를 검사해 기존 주문·조회 함수를 호출합니다. 상태 변경 뒤 복구 기록을 남깁니다. q일 때만 본 저장합니다.
 * 사용자가 고르는 저장·불러오기 메뉴는 없습니다.
 * 매개변수:
 * - app: 시간·입력·화면과 게임을 묶어 보관하는 실행 상태의 주소.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
void session_command(Session *app)
{
    char copy[sizeof(app->input.text)], *tokens[6], *token;
    size_t count = 0;
    uint64_t id = 0;
    Result result;
    bool changed = false;
    memcpy(copy, app->input.text, sizeof(copy));
    token = strtok(copy, " \t");
    while (token && count < 6)
    {
        tokens[count++] = token;
        token = strtok(NULL, " \t");
    }
    if (!count)
        return;
    /* 확인은 한 번만 소비합니다. 다른 명령을 내면 이전 확인 요청이 취소됩니다. */
    if (app->restart_enabled && !strcmp(tokens[0], "restart"))
    {
        bool confirmed = app->restart_pending;
        app->restart_pending = false;
        if (count == 1)
        {
            app->restart_pending = true;
            say(app, "현재 진행을 초기화할까요? restart yes / restart no");
        }
        else if (count == 2 && !strcmp(tokens[1], "no"))
            say(app, "다시하기를 취소했습니다.");
        else if (count == 2 && confirmed && !strcmp(tokens[1], "yes"))
            restart_game(app);
        else
            say(app, "먼저 restart를 입력한 뒤 restart yes로 확인하세요.");
        return;
    }
    app->restart_pending = false;

    if (!strcmp(tokens[0], "b") || !strcmp(tokens[0], "s") || !strcmp(tokens[0], "lb") ||
        !strcmp(tokens[0], "ls"))
        changed = execute_order(app, tokens, count); /* order.c와 연결 */
    else if (!strcmp(tokens[0], "cancel"))
    {
        if (count != 2 || !positive(tokens[1], &id, UINT64_MAX))
        {
            say(app, "cancel 주문ID 형식입니다.");
            return;
        }
        /* order.c: 미체결 주문을 취소하고 예약을 해제합니다. */
        result = order_cancel(&app->game, id);
        say(app, result_message(result));
        changed = result == RESULT_OK;
    }
    else if (!strcmp(tokens[0], "watch"))
    {
        if (count != 2 || !positive(tokens[1], &id, UINT64_MAX))
        {
            say(app, "watch 회사ID 형식입니다.");
            return;
        }
        /* stockQuery.c: 조회 목록에 표시할 관심 등록을 바꿉니다. */
        changed = stock_watch(&app->game, id);
        say(app, changed ? "관심 종목을 변경했습니다." : "회사를 찾을 수 없습니다.");
    }
    else if (!strcmp(tokens[0], "d"))
    {
        if (count != 2 || !positive(tokens[1], &id, UINT64_MAX) || company_find(&app->game, id) < 0)
        {
            say(app, "d 회사ID 형식으로 현재 상장된 회사를 선택하세요.");
            return;
        }
        app->screen.selected_id = id;
        app->screen.view = VIEW_DETAIL;
    }
    else if (!strcmp(tokens[0], "find"))
    {
        if (count > 2 || (count == 2 && strlen(tokens[1]) >= sizeof(app->screen.filter)))
        {
            say(app, "find 검색어 또는 find 형식입니다.");
            return;
        }
        (void)snprintf(app->screen.filter, sizeof(app->screen.filter), "%s",
                       count == 2 ? tokens[1] : "");
        app->screen.view = VIEW_MARKET;
        app->screen.page = 0;
    }
    else if (!strcmp(tokens[0], "sort"))
    {
        SortMode mode;
        if (count != 3 || (strcmp(tokens[2], "asc") && strcmp(tokens[2], "desc")))
        {
            say(app, "sort id/price/change/name asc/desc 형식입니다.");
            return;
        }
        if (!strcmp(tokens[1], "id"))
            mode = SORT_ID;
        else if (!strcmp(tokens[1], "price"))
            mode = SORT_PRICE;
        else if (!strcmp(tokens[1], "change"))
            mode = SORT_CHANGE;
        else if (!strcmp(tokens[1], "name"))
            mode = SORT_NAME;
        else
        {
            say(app, "정렬 항목은 id, price, change, name입니다.");
            return;
        }
        app->screen.sort = mode;
        app->screen.descending = !strcmp(tokens[2], "desc");
        app->screen.page = 0;
        app->screen.view = VIEW_MARKET;
    }
    else if (!strcmp(tokens[0], "page"))
    {
        if (count != 2 || !positive(tokens[1], &id, 1000))
        {
            say(app, "page 1과 같이 입력하세요.");
            return;
        }
        app->screen.page = (size_t)id - 1;
    }
    else if (count != 1)
    {
        say(app, "명령 형식을 확인하세요. h: 도움말");
    }
    else if (!strcmp(tokens[0], "q"))
    {
        char error[256];
        /* saveLoad.c: 본 저장이 완료된 후에만 종료합니다. */
        if (storage_finish(app->save_path, &app->game, error, sizeof(error)))
            app->quit = true;
        else
        {
            app->save_failed = true;
            say(app, error);
        }
    }
    else if (!strcmp(tokens[0], "l") || !strcmp(tokens[0], "watches"))
    {
        app->screen.view = VIEW_MARKET;
        app->screen.page = 0;
        app->screen.watched_only = !strcmp(tokens[0], "watches");
    }
    else if (!strcmp(tokens[0], "p"))
    {
        app->screen.view = VIEW_PORTFOLIO;
        app->screen.page = 0;
    }
    else if (!strcmp(tokens[0], "o"))
    {
        app->screen.view = VIEW_ORDERS;
        app->screen.page = 0;
    }
    else if (!strcmp(tokens[0], "t"))
    {
        app->screen.view = VIEW_TRADES;
        app->screen.page = 0;
    }
    else if (!strcmp(tokens[0], "a"))
    {
        app->screen.view = VIEW_ASSETS;
        app->screen.page = 0;
    }
    else if (!strcmp(tokens[0], "h"))
        app->screen.view = VIEW_HELP;
    else
        say(app, "알 수 없는 명령입니다. h로 도움말을 확인하세요.");
    if (changed)
        (void)session_checkpoint(app);
    app->redraw = true;
}
static bool set_program_directory(void)
{
    wchar_t path[2048], candidate[2048], *end, *part;
    DWORD count = GetModuleFileNameW(NULL, path, 2048);
    if (!count || count >= 2048)
        return false;
    end = wcsrchr(path, L'\\');
    if (!end)
        return false;
    *end = 0;
    (void)wcscpy(candidate, path);
    part = wcsrchr(candidate, L'\\');
    if (part && (!_wcsicmp(part + 1, L"Debug") || !_wcsicmp(part + 1, L"Release")))
    {
        *part = 0;
        part = wcsrchr(candidate, L'\\');
        if (part && !_wcsicmp(part + 1, L"bin"))
        {
            *part = 0;
            (void)wcscpy(path, candidate);
        }
    }
    if (!SetCurrentDirectoryW(path))
        return false;
    if (!CreateDirectoryW(L"data", NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        return false;
    return true;
}
/* Ctrl+C 처리기는 Windows가 호출해야 하므로 OS가 정한 함수 형식을 씁니다.
 * 게임 명령을 함수 포인터로 연결하는 구조는 사용하지 않습니다. */
bool session_stop_requested(void)
{
    return InterlockedExchange(&stop_requested, 0) != 0;
}

/* 화면과 저장을 준비합니다. 반복문은 main에서 직접 실행합니다.
 * 오류가 나면 false이며, 호출자는 session_close와 free로 정리합니다. */
bool session_open(Session *app, uint32_t interval_ms, uint64_t seed)
{
    char error[256];
    LoadResult loaded;
    memset(app, 0, sizeof(*app));
    if (!set_program_directory())
    {
        fputs("Cannot open the fixed data directory.\n", stderr);
        return false;
    }
    app->control.interval_ms = interval_ms;
    app->screen.interval_ms = interval_ms;
    app->save_path = "data/game_v4.sav";
    (void)SetConsoleOutputCP(CP_UTF8);
    if (!lock_save(app->save_path))
    {
        fputs("같은 게임이 이미 실행 중이거나 저장 경로를 사용할 수 없습니다.\n", stderr);
        return false;
    }
    loaded = storage_load(app->save_path, &app->game, error, sizeof(error));
    if (loaded == LOAD_ERROR)
    {
        fprintf(stderr, "%s\n", error);
        return false;
    }
    if (loaded == LOAD_NOT_FOUND)
    {
        if (!seed)
            seed = (uint64_t)time(NULL) ^ (now_ms() << 17);
        game_init(&app->game, seed);
        app->game.remaining_ms = interval_ms;
        say(app, "새 게임입니다. 공포지수 50에서 시작합니다. h: 도움말");
        if (!session_checkpoint(app))
        {
            fprintf(stderr, "%s\n", app->notice);
            return false;
        }
    }
    else
    {
        if (app->game.remaining_ms > interval_ms)
            app->game.remaining_ms = interval_ms;
        say(app, loaded == LOAD_BACKUP ? error : "저장된 시점부터 자동으로 이어서 시작합니다.");
    }
    screen_init();
    (void)SetConsoleCtrlHandler(console_handler, TRUE);
    app->last_clock = app->last_checkpoint = now_ms();
    app->redraw = true;
    return true;
}
