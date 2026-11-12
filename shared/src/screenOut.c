/** [모듈 설명]
 * screenOut.c - 고정 화면과 화면별 표시
 * 역할: 윈도 콘솔의 좌표에 쓰며 줄바꿈으로 화면을 쌓지 않습니다. 목록 10개, 거래 내역 5건 단위로 페이지를 나눕니다.
 * 관리 대상: Game은 읽기 전용; 콘솔 화면만 변경
 * 읽는 순서: 같은 이름의 .h 선언 → 구조체 → 공개 함수 → 내부 보조 함수.
 */
#include "screenOut.h"
#include "company.h"
#include "stockQuery.h"
#include "portfolio.h"
#include "market.h"
#include <stdarg.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define SCREEN_ROWS 24
#define ROW_BYTES 768
#define PAGE_SIZE 10U
static char rows[SCREEN_ROWS][ROW_BYTES];
static size_t row_count;
static char previous_rows[SCREEN_ROWS][ROW_BYTES];
static SMALL_RECT previous_window;
static bool first_paint = true;
static CONSOLE_CURSOR_INFO saved_cursor;
static bool cursor_saved;
static HANDLE original_output = INVALID_HANDLE_VALUE, screen_output = INVALID_HANDLE_VALUE;
static UINT original_cp;
static bool console_output;
/** [학습 설명]
 * 함수: row
 * 한 화면에 출력할 UTF-8 행을 준비합니다.
 * 매개변수:
 * - format: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void row(const char *format, ...)
{
    va_list arguments;
    if (row_count >= SCREEN_ROWS)
        return;
    va_start(arguments, format);
    (void)vsnprintf(rows[row_count++], ROW_BYTES, format, arguments);
    va_end(arguments);
}
/** [학습 설명]
 * 함수: format_money
 * 정수 금액을 천 단위 쉼표가 있는 문자열로 만듭니다. 호출자가 제공한 버퍼를 사용하므로 같은 출력문에서 여러 금액을 안전하게 표시할 수
 * 있습니다.
 * 매개변수:
 * - value: 계산·변환할 값 또는 계산 결과를 받을 출력 주소.
 * - buffer: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 호출자가 소유하거나 모듈이 보관한 문자열 주소. 반환 주소를 free하지 않습니다.
 */
const char *format_money(int64_t value, char buffer[48])
{
    char digits[32];
    uint64_t magnitude = value < 0 ? (uint64_t)(-(value + 1)) + 1 : (uint64_t)value;
    size_t i, length, written = 0;
    (void)snprintf(digits, sizeof(digits), "%" PRIu64, magnitude);
    length = strlen(digits);
    if (value < 0)
        buffer[written++] = '-';
    for (i = 0; i < length; ++i)
    {
        if (i && (length - i) % 3 == 0)
            buffer[written++] = ',';
        buffer[written++] = digits[i];
    }
    buffer[written] = '\0';
    return buffer;
}
/** [학습 설명]
 * 함수: screen_init
 * 가능하면 전용 Windows 콘솔 화면 버퍼를 만들고 활성화합니다. ANSI 지우기 명령에 의존하지 않습니다. 새 버퍼 생성 실패 시
 * 기존 콘솔 버퍼에 고정 위치로 출력합니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
void screen_init(void)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    original_output = GetStdHandle(STD_OUTPUT_HANDLE);
    original_cp = GetConsoleOutputCP();
    (void)SetConsoleOutputCP(CP_UTF8);
    console_output = GetConsoleScreenBufferInfo(original_output, &info) != 0;
    first_paint = true;
    if (console_output)
    {
        HANDLE created = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                                   CONSOLE_TEXTMODE_BUFFER, NULL);
        if (created != INVALID_HANDLE_VALUE)
        {
            COORD dimensions = {(SHORT)(info.srWindow.Right - info.srWindow.Left + 1),
                                (SHORT)(info.srWindow.Bottom - info.srWindow.Top + 1)};
            SMALL_RECT window = {0, 0, (SHORT)(dimensions.X - 1), (SHORT)(dimensions.Y - 1)};
            CONSOLE_SCREEN_BUFFER_INFO created_info;
            if (GetConsoleScreenBufferInfo(created, &created_info))
            {
                SMALL_RECT tiny = {0, 0, 0, 0};
                (void)SetConsoleWindowInfo(created, TRUE, &tiny);
            }
            if (SetConsoleScreenBufferSize(created, dimensions) &&
                SetConsoleWindowInfo(created, TRUE, &window) &&
                SetConsoleActiveScreenBuffer(created))
                screen_output = created;
            else
                CloseHandle(created);
        }
        if (screen_output == INVALID_HANDLE_VALUE)
            screen_output = original_output;
        if (GetConsoleCursorInfo(screen_output, &saved_cursor))
        {
            CONSOLE_CURSOR_INFO hidden = saved_cursor;
            cursor_saved = true;
            hidden.bVisible = FALSE;
            (void)SetConsoleCursorInfo(screen_output, &hidden);
        }
    }
}
/** [학습 설명]
 * 함수: screen_shutdown
 * 원래 콘솔 화면으로 복귀하고 생성한 화면 버퍼를 닫습니다. 프로그램 종료 처리에서 호출합니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
void screen_shutdown(void)
{
    if (cursor_saved && screen_output != INVALID_HANDLE_VALUE)
        (void)SetConsoleCursorInfo(screen_output, &saved_cursor);
    cursor_saved = false;
    if (screen_output != INVALID_HANDLE_VALUE && screen_output != original_output)
    {
        (void)SetConsoleActiveScreenBuffer(original_output);
        CloseHandle(screen_output);
    }
    screen_output = INVALID_HANDLE_VALUE;
    if (original_cp)
        (void)SetConsoleOutputCP(original_cp);
}
/** [학습 설명]
 * 함수: screen_interactive
 * 실제 콘솔 화면을 사용하는지 반환합니다. 파일·파이프로 출력할 때는 매초 전체 화면을 로그로 쌓지 않도록 타이머 화면 갱신을 구분합니다.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
bool screen_interactive(void)
{
    return console_output;
}
/** [학습 설명]
 * 함수: first_row
 * 페이지 번호가 범위를 넘으면 마지막 유효 페이지로 제한합니다.
 * 매개변수:
 * - count: 현재 유효한 항목 개수. 배열 최대 크기와 구분합니다.
 * - page: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
static size_t first_row(size_t count, size_t page)
{
    size_t pages = count ? (count + PAGE_SIZE - 1) / PAGE_SIZE : 1;
    if (page >= pages)
        page = pages - 1;
    return page * PAGE_SIZE;
}
/** [학습 설명]
 * 함수: page_label
 * 현재 페이지와 전체 개수를 표시합니다.
 * 매개변수:
 * - count: 현재 유효한 항목 개수. 배열 최대 크기와 구분합니다.
 * - start: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void page_label(size_t count, size_t start)
{
    row("페이지 %zu/%zu | page 번호로 이동 | 총 %zu개", start / PAGE_SIZE + 1,
        count ? (count + PAGE_SIZE - 1) / PAGE_SIZE : 1, count);
}
/** [학습 설명]
 * 함수: position_value
 * 단일 종목 평가액을 계산 한도 안에서 구합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - slot: 회사 배열의 위치(0~29). 화면 정렬 순번이나 회사 고유 ID와 다릅니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
static int64_t position_value(const Game *game, size_t slot)
{
    if (game->shares[slot] > MAX_MONEY / game->companies[slot].price)
        return MAX_MONEY;
    return game->shares[slot] * game->companies[slot].price;
}
/** [학습 설명]
 * 함수: market_view
 * 종목 목록과 관심·검색·정렬 결과를 표시합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - state: 선택한 화면·검색·정렬·페이지 등 화면 상태.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void market_view(const Game *game, const ScreenState *state)
{
    int slots[COMPANY_COUNT];
    size_t count = stock_query(game, slots, state->filter, state->watched_only, state->sort,
                               state->descending);
    size_t start = first_row(count, state->page), i;
    char a[48];
    row("[종목 목록] %s | 검색: %s", state->watched_only ? "관심 종목" : "전체 종목",
        state->filter[0] ? state->filter : "없음");
    row("ID    관심 회사명            업종          현재가(원)     등락률    보유(주)");
    for (i = start; i < count && i < start + PAGE_SIZE; ++i)
    {
        int slot = slots[i];
        const Company *c = &game->companies[slot];

        int name_width = 16 + (int)(strlen(c->name) / 3);
        row("%-5" PRIu64 " %c    %-*s %-9s %15s  %+7.2f%% %10" PRId64, c->id,
            c->watched ? '*' : ' ', name_width, c->name, sector_name(c->sector),
            format_money(c->price, a), c->last_change_bp / 100.0, game->shares[slot]);
    }
    while (row_count < 16)
        row("");
    page_label(count, start);
}
/** [학습 설명]
 * 함수: detail_view
 * 회사 재무 정보와 최근 가격의 문자 차트를 표시합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - state: 선택한 화면·검색·정렬·페이지 등 화면 상태.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void detail_view(const Game *game, const ScreenState *state)
{
    int slot = company_find(game, state->selected_id);
    size_t i, start, count;
    int64_t low, high;
    char a[48], b[48];
    const Company *c;
    if (slot < 0)
    {
        row("해당 회사가 상장되어 있지 않습니다. l로 목록을 확인하세요.");
        return;
    }
    c = &game->companies[slot];
    row("[회사 상세] ID %" PRIu64 " %s | 업종 %s | 관심 %s", c->id, c->name, sector_name(c->sector),
        c->watched ? "등록" : "미등록");
    row("현재가 %s원 | 영업이익률 %.2f%% | 부채비율 %d%%", format_money(c->price, a),
        c->profit_bp / 100.0, c->debt_percent);
    row("매출 %s백만원 | 업종 영향 %+.2f%% | 재무 정보는 %d회 갱신마다 변경",
        format_money(c->revenue_million, a), game->sector_bp[c->sector] / 100.0, FINANCE_INTERVAL);
    row("가상의 %s 분야 기업입니다. 수익성과 부채, 업종 상황이 가격 변동에 반영됩니다.",
        sector_name(c->sector));
    count = c->price_count < 60 ? c->price_count : 60;
    start = c->price_count - count;
    low = high = c->prices[start];
    for (i = start; i < c->price_count; ++i)
    {
        if (c->prices[i] < low)
            low = c->prices[i];
        if (c->prices[i] > high)
            high = c->prices[i];
    }
    row("최근 %zu개 가격 | 최저 %s / 최고 %s원 (위쪽이 높은 가격)", count, format_money(low, a),
        format_money(high, b));
    {
        int level;
        for (level = 6; level >= 0; --level)
        {
            char chart[64];
            for (i = 0; i < count; ++i)
            {
                int point =
                    high == low ? 3 : (int)((c->prices[start + i] - low) * 6 / (high - low));
                chart[i] = point == level ? '*' : ' ';
            }
            chart[count] = '\0';
            row("|%s", chart);
        }
    }
    row("+------------------------------------------------------------> 시간");
    row("watch ID: 관심 전환 | b/s ID 수량: 시장가 | lb/ls ID 수량 가격: 지정가");
}
/** [학습 설명]
 * 함수: portfolio_view
 * 수량·매도 가능 수량·평균 원가·평가 손익을 표시합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - state: 선택한 화면·검색·정렬·페이지 등 화면 상태.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void portfolio_view(const Game *game, const ScreenState *state)
{
    int slots[COMPANY_COUNT];
    size_t count = 0, start, i;
    char a[48], b[48];
    for (i = 0; i < COMPANY_COUNT; ++i)
        if (game->shares[i])
            slots[count++] = (int)i;
    start = first_row(count, state->page);
    row("[보유 자산] 평균 매입가는 매수 수수료 포함. 평가손익은 미래 매도 비용 제외.");
    row("ID    회사명              보유/매도가능        평균매입가       평가손익(원)");
    for (i = start; i < count && i < start + PAGE_SIZE; ++i)
    {
        int slot = slots[i];
        const Company *c = &game->companies[slot];
        int width = 16 + (int)(strlen(c->name) / 3);
        row("%-5" PRIu64 " %-*s %8" PRId64 "/%-8" PRId64 " %14.2f %17s", c->id, width, c->name,
            game->shares[slot], available_shares(game, (size_t)slot),
            (double)game->cost_basis[slot] / (double)game->shares[slot],
            format_money(position_value(game, (size_t)slot) - game->cost_basis[slot], a));
    }
    while (row_count < 16)
        row("");
    row("누적 수수료 %s원 / 누적 매도세 %s원", format_money(game->fees_paid, a),
        format_money(game->taxes_paid, b));
    page_label(count, start);
}
/** [학습 설명]
 * 함수: orders_view
 * 미체결 지정가와 예약 금액을 표시합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - state: 선택한 화면·검색·정렬·페이지 등 화면 상태.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void orders_view(const Game *game, const ScreenState *state)
{
    size_t start = first_row(game->order_count, state->page), i;
    char a[48], b[48];
    row("[미체결 주문] 가격 조건을 만족하면 현재가로 전량 체결. cancel 주문ID로 취소.");
    row("주문ID  회사ID  구분      수량          지정가(원)       현금예약(원)");
    for (i = start; i < game->order_count && i < start + PAGE_SIZE; ++i)
    {
        const Order *o = &game->orders[i];
        row("%-7" PRIu64 " %-7" PRIu64 " %-6s %8" PRId64 " %17s %18s", o->id, o->company_id,
            o->side == SIDE_BUY ? "매수" : "매도", o->quantity, format_money(o->limit_price, a),
            format_money(o->reserved_cash, b));
    }
    while (row_count < 16)
        row("");
    page_label(game->order_count, start);
}
/** [학습 설명]
 * 함수: trades_view
 * 최근 체결·폐지, 개별 수수료와 세금을 표시합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - state: 선택한 화면·검색·정렬·페이지 등 화면 상태.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void trades_view(const Game *game, const ScreenState *state)
{
    size_t pages = game->trade_count ? (game->trade_count + 4U) / 5U : 1U;
    size_t page = state->page < pages ? state->page : pages - 1;
    size_t start = page * 5U, i;
    char a[48], b[48];
    row("[거래 내역] 최신순, 최근 %d건. 폐지 손실도 실현손익에 포함합니다.", TRADE_HISTORY_MAX);
    row("회차  회사ID  구분     수량          체결가(원)         실현손익(원)");
    for (i = start; i < game->trade_count && i < start + 5U; ++i)
    {
        const TradeRecord *t = &game->trades[game->trade_count - 1 - i];
        row("%-5" PRIu64 " %-7" PRIu64 " %-6s %8" PRId64 " %17s %18s", t->tick, t->company_id,
            t->side == SIDE_BUY ? "매수" : (t->side == SIDE_SELL ? "매도" : "폐지"), t->quantity,
            format_money(t->price, a), format_money(t->realized, b));
        row("      %s | 수수료 %s원 / 세금 %s원 | 주문ID %" PRIu64, t->name,
            format_money(t->fee, a), format_money(t->tax, b), t->order_id);
    }
    while (row_count < 16)
        row("");
    row("페이지 %zu/%zu | page 번호로 이동 | 총 %u건", page + 1, pages, game->trade_count);
}
/** [학습 설명]
 * 함수: assets_view
 * 최근 자산 변동을 표시합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - state: 선택한 화면·검색·정렬·페이지 등 화면 상태.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void assets_view(const Game *game, const ScreenState *state)
{
    size_t start = first_row(game->asset_count, state->page), i;
    char a[48], b[48];
    row("[자산 변동 기록] 최신순, 최근 %d건. 가격 갱신과 거래 후 기록.", ASSET_HISTORY_MAX);
    row("갱신 회차           현금(원)                총자산(원)");
    for (i = start; i < game->asset_count && i < start + PAGE_SIZE; ++i)
    {
        const AssetRecord *a_record = &game->assets[game->asset_count - 1 - i];
        row("%-10" PRIu64 " %22s %26s", a_record->tick, format_money(a_record->cash, a),
            format_money(a_record->total, b));
    }
    while (row_count < 16)
        row("");
    page_label(game->asset_count, start);
}
/** [학습 설명]
 * 함수: help_view
 * 현재 실행 프로그램의 실제 갱신 간격과 수수료 설정, 일반 명령을 표시합니다. 수동 저장 명령은 안내하지 않습니다.
 * 매개변수:
 * - state: 선택한 화면·검색·정렬·페이지 등 화면 상태.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void help_view(const ScreenState *state)
{
    row("[도움말] 명령은 영문 소문자, 입력 후 Enter. 가격은 입력 중에도 %u초마다 갱신.",
        (state->interval_ms ? state->interval_ms : TICK_MS) / 1000);
    row("l: 종목 목록 | d ID: 회사 상세와 차트 | p: 보유 자산 | o: 미체결 주문");
    row("t: 거래 내역 | a: 자산 기록 | h: 도움말 | page 번호: 페이지 이동");
    row("b ID 수량 / s ID 수량: 시장가 매수 / 매도");
    row("lb ID 수량 가격 / ls ID 수량 가격: 지정가 매수 / 매도");
    row("cancel 주문ID: 미체결 취소 | watch ID: 관심 등록/해제 | watches: 관심 목록");
    row("find 이름/업종/ID: 검색 (예: find 의료) | find: 검색 해제");
    row("sort id/price/change/name asc/desc: 정렬 (예: sort price desc)");
    row("q: 본 저장 후 종료 | Ctrl+C: 복구 상태를 남기고 종료");
    row("공포지수: 0에 가까울수록 탐욕, 100에 가까울수록 공포. 새 게임은 50.");
    if (state->extra_help && state->extra_help[0])
        row("%s", state->extra_help);
    row("게임용 수수료: 매수/매도 각각 %.2f%%, 매도세 %.2f%%. 각 비용은 원 단위 올림.",
        FEE_BP / 100.0, SELL_TAX_BP / 100.0);
    row("1,000원 이하 즉시 폐지, 주식은 무보상 소멸. 현금과 주식 가치가 모두 0원이면 종료.");
    row("최근 변경은 자동 복구용으로 기록. q 종료 시 본 저장. 꺼진 동안 시장 정지.");
}
/** [학습 설명]
 * 함수: paint
 * 이전 문자열과 달라진 행만 공백을 덧붙여 같은 좌표에 덮어씁니다. 줄바꿈 출력이 화면을 밀지 않게 폭을 제한하고 작은 창에서도 하단
 * 입력과 안내를 남깁니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
/* 이전 행 문자열과 비교해 바뀐 행만 덮어씁니다.
 * 짧아진 글자의 흔적은 뒤에 공백을 붙여 같은 출력 한 번으로 지웁니다.
 * 전체 화면을 먼저 비우지 않으므로 빈 화면이 눈에 보이지 않습니다. */
static void paint(void)
{
    size_t i;
    if (console_output)
    {
        CONSOLE_SCREEN_BUFFER_INFO info;
        SHORT width, height;
        bool resized;
        if (!GetConsoleScreenBufferInfo(screen_output, &info))
            return;
        width = (SHORT)(info.srWindow.Right - info.srWindow.Left + 1);
        height = (SHORT)(info.srWindow.Bottom - info.srWindow.Top + 1);
        resized = first_paint || memcmp(&previous_window, &info.srWindow, sizeof(SMALL_RECT));
        for (i = 0; i < (size_t)height; ++i)
        {
            const char *text = "";
            size_t source = i;
            wchar_t output[ROW_BYTES];
            int count, j, columns = 0;
            DWORD written;
            COORD location = {info.srWindow.Left, (SHORT)(info.srWindow.Top + (SHORT)i)};
            if (height >= 8 && (size_t)height < row_count && i >= (size_t)height - 4)
                source = row_count - 4 + i - ((size_t)height - 4);
            if (source < row_count)
                text = rows[source];
            if (!resized && i >= SCREEN_ROWS)
                continue;
            if (!resized && !strcmp(previous_rows[i], text))
                continue;
            count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, output, ROW_BYTES);
            if (!count)
                continue;
            for (j = 0; j < count - 1; ++j)
            {
                int cells = output[j] >= 0x1100 ? 2 : 1;
                if (columns + cells >= width)
                    break;
                columns += cells;
            }
            /* 마지막 한 열은 남겨 우측 끝 자동 줄바꿈을 피합니다. */
            while (columns < width - 1 && j < ROW_BYTES - 1)
            {
                output[j++] = L' ';
                ++columns;
            }
            (void)SetConsoleCursorPosition(screen_output, location);
            if (WriteConsoleW(screen_output, output, (DWORD)j, &written, NULL) &&
                written == (DWORD)j)
            {
                if (i < SCREEN_ROWS)
                    (void)snprintf(previous_rows[i], ROW_BYTES, "%s", text);
            }
            else
            {
                first_paint = true;
                return;
            }
        }
        previous_window = info.srWindow;
        first_paint = false;
    }
    else
    {
        for (i = 0; i < row_count; ++i)
            puts(rows[i]);
        (void)fflush(stdout);
    }
}
/** [학습 설명]
 * 함수: screen_render
 * Game을 읽기만 하여 현재 화면의 행들을 구성하고 출력합니다. 내부 StockType은 표시하지 않습니다. 화면 모드와 페이지에 따라
 * 목록·상세·자산·주문·거래·자산 기록·도움말을 만듭니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - state: 선택한 화면·검색·정렬·페이지 등 화면 상태.
 * - notice: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * - input: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * - save_failed: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * - save_path: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
void screen_render(const Game *game, const ScreenState *state, const char *notice,
                   const char *input, bool save_failed, const char *save_path)
{
    int64_t total, unrealized = 0;
    bool fits = portfolio_total(game, &total);
    char a[48], b[48], c[48];
    size_t i;
    row_count = 0;
    for (i = 0; i < COMPANY_COUNT; ++i)
    {
        int64_t value = position_value(game, i) - game->cost_basis[i];
        if (value > 0 && unrealized > MAX_MONEY - value)
            unrealized = MAX_MONEY;
        else if (value < 0 && unrealized < -MAX_MONEY - value)
            unrealized = -MAX_MONEY;
        else
            unrealized += value;
    }
    row("%s | 공포지수 %d/100 | 시장 상태: %s | 갱신 # %" PRIu64 " | 다음 %u초%s",
        state->title ? state->title : "주식 모의투자 V2", game->fear,
        market_phase_name(game->phase), game->tick_count, (game->remaining_ms + 999) / 1000,
        portfolio_bankrupt(game) ? " | 파산: 게임 종료" : "");
    row("총자산 %s%s원 | 현금 %s원 | 사용 가능 %s원", fits ? "" : "최소 ", format_money(total, a),
        format_money(game->cash, b), format_money(available_cash(game), c));
    row("실현손익 %s원 | 평가손익 %s원 | 저장 %s", format_money(game->realized, a),
        format_money(unrealized, b), save_failed ? "실패! q로 재시도" : "정상");
    row("------------------------------------------------------------------------------------------"
        "-");
    switch (state->view)
    {
    case VIEW_DETAIL:
        detail_view(game, state);
        break;
    case VIEW_PORTFOLIO:
        portfolio_view(game, state);
        break;
    case VIEW_ORDERS:
        orders_view(game, state);
        break;
    case VIEW_TRADES:
        trades_view(game, state);
        break;
    case VIEW_ASSETS:
        assets_view(game, state);
        break;
    case VIEW_HELP:
        help_view(state);
        break;
    default:
        market_view(game, state);
        break;
    }
    while (row_count < 18)
        row("");
    row("l 목록 | d ID 상세 | p 자산 | o 주문 | t 거래 | a 자산기록 | h 도움말 | q 종료");
    row("%s", notice);
    row("%s%s", state->paused ? "[일시정지] " : "",
        state->footer ? state->footer
                      : "q로 종료하면 저장됩니다. 다음 실행은 자동 이어 하기입니다.");
    (void)save_path;
    row("> %s", input);
    paint();
}
