/** [모듈 설명]
 * test_game.c - 자동 검증
 * 역할: 기존 V2 테스트를 재사용하고 V3 저장·파일 개수 검증을 추가했습니다.
 * 관리 대상: 테스트용 상태와 임시 파일
 * 읽는 순서: 같은 이름의 .h 선언 → 구조체 → 공개 함수 → 내부 보조 함수.
 */
#include "company.h"
#include "gameControl.h"
#include "history.h"
#include "market.h"
#include "order.h"
#include "portfolio.h"
#include "trade.h"
#include "listing.h"
#include "stockPrice.h"
#include "stockQuery.h"
#include "saveLoad.h"
#include "screenOut.h"
#include "input.h"
#include <wchar.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static unsigned long checks;
static Game a, b, before;
#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        ++checks;                                                                                  \
        if (!(x))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #x);                            \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
/** [학습 설명]
 * 함수: price
 * 이 모듈 안에서만 쓰는 보조 함수입니다. 아래 인수와 반환값을 이용해 상위 공개 함수의 처리를 나눕니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - slot: 회사 배열의 위치(0~29). 화면 정렬 순번이나 회사 고유 ID와 다릅니다.
 * - amount: 비율을 적용할 금액(원).
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void price(Game *game, size_t slot, int64_t amount)
{
    game->companies[slot].price = amount;
    history_price(&game->companies[slot]);
}
/** [학습 설명]
 * 함수: test_init
 * 이름에 해당하는 기능을 정상·실패 사례로 검사합니다. CHECK 조건을 따라 기대값과 실제값을 비교해 보세요.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void test_init(void)
{
    uint64_t seed;
    for (seed = 0; seed < 100; ++seed)
    {
        int counts[3] = {0};
        size_t i;
        game_init(&a, seed);
        game_init(&b, seed);
        CHECK(!memcmp(&a, &b, sizeof(a)));
        CHECK(game_validate(&a, NULL, 0));
        for (i = 0; i < COMPANY_COUNT; ++i)
        {
            Company *c = &a.companies[i];
            ++counts[c->type];
            CHECK(c->price > 1000 && c->price_count == 1);
            if (c->type == STOCK_BLUE_CHIP)
                CHECK(c->price >= 3000000 && c->price <= 5000000);
            if (c->type == STOCK_ORDINARY)
                CHECK(c->price >= 200000 && c->price <= 700000);
            if (c->type == STOCK_SPECULATIVE)
                CHECK(c->price >= 3000 && c->price <= 12000);
        }
        CHECK(counts[0] == 10 && counts[1] == 15 && counts[2] == 5);
        CHECK(a.cash == INITIAL_CASH && a.asset_count == 1);
        CHECK(a.fear == 50 && a.fear_target == 50 && a.phase == PHASE_NORMAL);
    }
}
/** [학습 설명]
 * 함수: test_accounting
 * 이름에 해당하는 기능을 정상·실패 사례로 검사합니다. CHECK 조건을 따라 기대값과 실제값을 비교해 보세요.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void test_accounting(void)
{
    uint64_t id;
    int64_t total;
    game_init(&a, 123);
    id = a.companies[0].id;
    price(&a, 0, 10000);
    CHECK(rate_amount(10000, FEE_BP) == 5 && rate_amount(1001, FEE_BP) == 1);
    CHECK(order_market(&a, id, SIDE_BUY, 10) == RESULT_OK);
    CHECK(a.cost_basis[0] == 100050 && a.shares[0] == 10 && a.cash == INITIAL_CASH - 100050);
    CHECK(portfolio_total(&a, &total) && total == INITIAL_CASH - 50);
    price(&a, 0, 12000);
    CHECK(order_market(&a, id, SIDE_SELL, 4) == RESULT_OK);
    CHECK(a.realized == 7884 && a.cost_basis[0] == 60030 && a.shares[0] == 6);
    CHECK(order_market(&a, id, SIDE_SELL, 6) == RESULT_OK);
    CHECK(a.realized == 19710 && a.cash == INITIAL_CASH + 19710 && !a.cost_basis[0]);
    CHECK(a.fees_paid == 110 && a.taxes_paid == 180);
    before = a;
    CHECK(order_market(&a, id, SIDE_BUY, 0) == RESULT_BAD_INPUT);
    CHECK(order_market(&a, UINT64_MAX, SIDE_BUY, 1) == RESULT_NO_COMPANY);
    CHECK(order_market(&a, id, SIDE_SELL, 1) == RESULT_NO_SHARES);
    CHECK(!memcmp(&a, &before, sizeof(a)));
    a.cash = 1;
    CHECK(!portfolio_bankrupt(&a));
    a.cash = 0;
    CHECK(portfolio_bankrupt(&a));
    a.shares[0] = 1;
    CHECK(!portfolio_bankrupt(&a));
    CHECK(proportional_cost(MAX_MONEY, 50000000, 100000000) == MAX_MONEY / 2);
}
/** [학습 설명]
 * 함수: test_orders
 * 이름에 해당하는 기능을 정상·실패 사례로 검사합니다. CHECK 조건을 따라 기대값과 실제값을 비교해 보세요.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void test_orders(void)
{
    uint64_t id, order_id, another;
    TickReport report = {0};
    game_init(&a, 123);
    id = a.companies[0].id;
    price(&a, 0, 20000);
    a.cash = 100050;
    CHECK(order_limit(&a, id, SIDE_BUY, 10, 10000, &order_id) == RESULT_OK);
    CHECK(a.order_count == 1 && available_cash(&a) == 0 && reserved_cash(&a) == a.cash);
    CHECK(!portfolio_bankrupt(&a));
    CHECK(order_limit(&a, id, SIDE_BUY, 1, 10000, &another) == RESULT_NO_CASH);
    CHECK(order_market(&a, id, SIDE_BUY, 1) == RESULT_NO_CASH);
    CHECK(game_validate(&a, NULL, 0));
    price(&a, 0, 9000);
    order_process(&a, &report);
    CHECK(report.filled == 1 && !a.order_count && a.shares[0] == 10);
    CHECK(a.cash == 10005 && available_cash(&a) == 10005 && a.trades[0].order_id == order_id);
    CHECK(order_limit(&a, id, SIDE_SELL, 10, 15000, &order_id) == RESULT_OK);
    CHECK(available_shares(&a, 0) == 0);
    CHECK(order_market(&a, id, SIDE_SELL, 1) == RESULT_NO_SHARES);
    CHECK(order_cancel(&a, order_id) == RESULT_OK && available_shares(&a, 0) == 10);
    CHECK(order_cancel(&a, order_id) == RESULT_NO_ORDER);
    CHECK(order_limit(&a, id, SIDE_SELL, 10, 8000, &order_id) == RESULT_OK);
    CHECK(!a.order_count && !a.shares[0] && a.trades[1].price == 9000);
    CHECK(game_validate(&a, NULL, 0));
    game_init(&a, 55);
    id = a.companies[0].id;
    price(&a, 0, 10000);
    CHECK(order_limit(&a, id, SIDE_BUY, 1, 8000, &order_id) == RESULT_OK);
    CHECK(order_limit(&a, id, SIDE_BUY, 1, 8000, &another) == RESULT_OK);
    price(&a, 0, 7000);
    memset(&report, 0, sizeof(report));
    order_process(&a, &report);
    CHECK(report.filled == 2 && a.trades[0].order_id == order_id &&
          a.trades[1].order_id == another);
    CHECK(!reserved_cash(&a) && a.shares[0] == 2 && game_validate(&a, NULL, 0));
}
/** [학습 설명]
 * 함수: test_delist
 * 이름에 해당하는 기능을 정상·실패 사례로 검사합니다. CHECK 조건을 따라 기대값과 실제값을 비교해 보세요.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void test_delist(void)
{
    TickReport report = {0};
    uint64_t id, order_id;
    game_init(&a, 678);
    id = a.companies[0].id;
    price(&a, 0, 5000);
    CHECK(order_market(&a, id, SIDE_BUY, 2) == RESULT_OK);
    CHECK(order_limit(&a, id, SIDE_SELL, 1, 6000, &order_id) == RESULT_OK);
    CHECK(order_limit(&a, id, SIDE_BUY, 1, 2000, &order_id) == RESULT_OK);
    a.cash = reserved_cash(&a);
    price(&a, 0, 1000);
    CHECK(listing_process(&a, &report));
    CHECK(report.delisted == 1 && company_find(&a, id) == -1 && !a.shares[0] && !a.cost_basis[0]);
    CHECK(!a.order_count && available_cash(&a) == a.cash && a.cash > 0);
    CHECK(a.realized == -10005 && a.trades[1].side == SIDE_DELIST);
    CHECK(!portfolio_bankrupt(&a) && game_validate(&a, NULL, 0));
    a.cash = 0;
    CHECK(portfolio_bankrupt(&a));
}
/** [학습 설명]
 * 함수: test_market
 * 이름에 해당하는 기능을 정상·실패 사례로 검사합니다. CHECK 조건을 따라 기대값과 실제값을 비교해 보세요.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void test_market(void)
{
    unsigned int tick;
    unsigned long rising = 0, falling = 0;
    long long sum = 0;
    TickReport report, other;
    game_init(&a, 9876);
    a.phase = PHASE_CRASH;
    a.phase_ticks_left = 4;
    a.fear = a.fear_target = 95;
    for (tick = 0; tick < 1000; ++tick)
    {
        size_t i;
        memset(&report, 0, sizeof(report));
        for (i = 0; i < COMPANY_COUNT; ++i)
            price(&a, i, 100000);
        stock_prices_update(&a, &report);
        rising += report.rising;
        falling += report.falling;
        for (i = 0; i < COMPANY_COUNT; ++i)
        {
            sum += a.companies[i].last_change_bp;
            CHECK(a.companies[i].last_change_bp >= -3000 && a.companies[i].last_change_bp <= 3000);
        }
    }
    CHECK(falling > 22500 && falling < 29700 && rising > 300 && sum < 0);
    printf("Crash sample: rising=%lu falling=%lu out of 30000; average=%.2f bp\n", rising, falling,
           (double)sum / 30000.0);
    game_init(&a, 789);
    b = a;
    for (tick = 0; tick < 10000; ++tick)
    {
        CHECK(game_tick(&a, &report));
        CHECK(game_tick(&b, &other));
        CHECK(!memcmp(&a, &b, sizeof(a)));
        CHECK(game_validate(&a, NULL, 0));
    }
    CHECK(a.next_company_id >= 31 && a.companies[0].price_count >= 1);
}
/** [학습 설명]
 * 함수: test_fundamentals_and_capacity
 * 이름에 해당하는 기능을 정상·실패 사례로 검사합니다. CHECK 조건을 따라 기대값과 실제값을 비교해 보세요.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void test_fundamentals_and_capacity(void)
{
    TickReport report;
    uint64_t id, order_id;
    unsigned int i;
    game_init(&a, 99);
    a.companies[0].type = STOCK_BLUE_CHIP;
    b = a;
    a.companies[0].profit_bp = 2000;
    a.companies[0].debt_percent = 10;
    b.companies[0].profit_bp = -2000;
    b.companies[0].debt_percent = 250;
    memset(&report, 0, sizeof(report));
    stock_prices_update(&a, &report);
    stock_prices_update(&b, &report);
    CHECK(a.companies[0].last_change_bp > b.companies[0].last_change_bp &&
          a.rng_state == b.rng_state);
    game_init(&a, 99);
    a.companies[0].type = STOCK_BLUE_CHIP;
    b = a;
    a.sector_bp[a.companies[0].sector] = 100;
    b.sector_bp[b.companies[0].sector] = -100;
    stock_prices_update(&a, &report);
    stock_prices_update(&b, &report);
    CHECK(a.companies[0].last_change_bp > b.companies[0].last_change_bp &&
          a.rng_state == b.rng_state);
    game_init(&a, 88);
    id = a.companies[0].id;
    for (i = 0; i < ORDER_MAX; ++i)
        CHECK(order_limit(&a, id, SIDE_BUY, 1, 1001, &order_id) == RESULT_OK);
    CHECK(a.order_count == ORDER_MAX && game_validate(&a, NULL, 0));
    CHECK(order_limit(&a, id, SIDE_BUY, 1, 1001, &order_id) == RESULT_LIMIT);
    while (a.order_count)
        CHECK(order_cancel(&a, a.orders[0].id) == RESULT_OK);
    CHECK(!reserved_cash(&a) && available_cash(&a) == a.cash);
    price(&a, 0, 10000);
    for (i = 0; i < 300; ++i)
    {
        CHECK(order_market(&a, id, SIDE_BUY, 1) == RESULT_OK);
        CHECK(order_market(&a, id, SIDE_SELL, 1) == RESULT_OK);
    }
    CHECK(a.trade_count == TRADE_HISTORY_MAX && a.asset_count == ASSET_HISTORY_MAX);
    CHECK(game_validate(&a, NULL, 0));
}
/** [학습 설명]
 * 함수: test_query_and_input
 * 이름에 해당하는 기능을 정상·실패 사례로 검사합니다. CHECK 조건을 따라 기대값과 실제값을 비교해 보세요.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void test_query_and_input(void)
{
    int slots[COMPANY_COUNT];
    size_t i, count;
    uint64_t value;
    InputLine line = {0};
    game_init(&a, 39);
    CHECK(stock_watch(&a, a.companies[3].id));
    CHECK(stock_query(&a, slots, "", true, SORT_ID, false) == 1 && slots[0] == 3);
    count = stock_query(&a, slots, "", false, SORT_PRICE, true);
    CHECK(count == COMPANY_COUNT);
    for (i = 1; i < count; ++i)
        CHECK(a.companies[slots[i - 1]].price >= a.companies[slots[i]].price);
    count = stock_query(&a, slots, a.companies[3].name, false, SORT_ID, false);
    CHECK(count == 1 && slots[0] == 3);
    CHECK(input_uint("18446744073709551615", &value) && value == UINT64_MAX);
    CHECK(!input_uint("18446744073709551616", &value));
    CHECK(!input_uint("-1", &value));
    CHECK(!input_feed(&line, 'b') && !input_feed(&line, ' '));
    CHECK(!input_feed(&line, 0x10000 + 0xAC00) && line.length == 5);
    CHECK(!input_feed(&line, 8) && line.length == 2);
    for (i = 0; i < 250; ++i)
        (void)input_feed(&line, '1');
    CHECK(line.invalid && input_feed(&line, '\n'));
}
/** [학습 설명]
 * 함수: file_digest
 * 테스트에서 본 저장의 바이트가 실행 중 변경되지 않았는지 비교하기 위한 값입니다. 게임의 보안 기능은 아닙니다.
 * 매개변수:
 * - path: 내부 저장 기준 경로. 일반 사용자가 선택하는 슬롯 인수가 아닙니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
static uint64_t file_digest(const char *path)
{
    uint64_t value = UINT64_C(14695981039346656037);
    FILE *file = fopen(path, "rb");
    int ch;
    if (!file)
        return 0;
    while ((ch = fgetc(file)) != EOF)
    {
        value ^= (unsigned int)ch;
        value *= UINT64_C(1099511628211);
    }
    CHECK(fclose(file) == 0);
    return value;
}
/** [학습 설명]
 * 함수: damage_file
 * 복구 시험을 위해 테스트 파일 헤더 한 바이트를 의도적으로 손상시킵니다. 사용자 저장에 호출하지 않습니다.
 * 매개변수:
 * - path: 내부 저장 기준 경로. 일반 사용자가 선택하는 슬롯 인수가 아닙니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void damage_file(const char *path)
{
    FILE *file = fopen(path, "r+b");
    CHECK(file != NULL);
    CHECK(fputc('X', file) != EOF);
    CHECK(fclose(file) == 0);
}
/** [학습 설명]
 * 함수: test_storage
 * 본 저장은 q 전용인지, 복구 후보를 순번대로 고르는지, 200회 기록해도 파일이 세 개인지, 손상 후보를 건너뛰는지 검증합니다.
 * 매개변수:
 * - directory: 테스트가 만든 임시 폴더 경로. 사용자 저장과 분리합니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void test_storage(const char *directory)
{
    char path[2048], names[3][2064], error[256], pattern[2064];
    TickReport report;
    uint64_t order_id, primary_hash;
    unsigned int i, count = 0;
    WIN32_FIND_DATAA found;
    HANDLE scan;
    (void)snprintf(path, sizeof(path), "%s/unit_v3.sav", directory);
    (void)snprintf(names[0], sizeof(names[0]), "%s", path);
    (void)snprintf(names[1], sizeof(names[1]), "%s.tmp0", path);
    (void)snprintf(names[2], sizeof(names[2]), "%s.tmp1", path);
    for (i = 0; i < 3; ++i)
        (void)remove(names[i]);
    game_init(&a, 6789);
    CHECK(storage_load(path, &b, error, sizeof(error)) == LOAD_NOT_FOUND);
    CHECK(order_market(&a, a.companies[0].id, SIDE_BUY, 1) == RESULT_OK);
    CHECK(order_limit(&a, a.companies[1].id, SIDE_BUY, 1, 1001, &order_id) == RESULT_OK);
    a.remaining_ms = 7321;
    a.companies[2].watched = true;
    CHECK(storage_checkpoint(path, &a, error, sizeof(error)));
    CHECK(file_digest(path) == 0); /* q를 호출하기 전에는 본 저장이 없어야 함 */
    CHECK(storage_load(path, &b, error, sizeof(error)) == LOAD_BACKUP);
    CHECK(!memcmp(&a, &b, sizeof(a)));
    CHECK(storage_finish(path, &a, error, sizeof(error)));
    CHECK(storage_load(path, &b, error, sizeof(error)) == LOAD_OK);
    CHECK(!memcmp(&a, &b, sizeof(a)));
    before = a;
    primary_hash = file_digest(path);
    CHECK(game_tick(&a, &report) && game_tick(&b, &report));
    CHECK(!memcmp(&a, &b, sizeof(a)));
    for (i = 0; i < 200; ++i)
        CHECK(storage_checkpoint(path, &a, error, sizeof(error)));
    CHECK(file_digest(path) == primary_hash); /* 실행 중 체크포인트는 본 저장을 바꾸지 않음 */
    CHECK(storage_load(path, &b, error, sizeof(error)) == LOAD_BACKUP);
    CHECK(!memcmp(&a, &b, sizeof(a)));
    (void)snprintf(pattern, sizeof(pattern), "%s*", path);
    scan = FindFirstFileA(pattern, &found);
    CHECK(scan != INVALID_HANDLE_VALUE);
    do
    {
        if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            ++count;
    } while (FindNextFileA(scan, &found));
    FindClose(scan);
    CHECK(count == 3); /* 본 저장 1개 + 복구 파일 2개에서 더 늘어나지 않음 */
    damage_file(names[(a.save_generation & 1) ? 2 : 1]);
    CHECK(storage_load(path, &b, error, sizeof(error)) == LOAD_BACKUP);
    CHECK(b.save_generation + 1 == a.save_generation);
    damage_file(names[(a.save_generation & 1) ? 1 : 2]);
    CHECK(storage_load(path, &b, error, sizeof(error)) == LOAD_OK);
    CHECK(!memcmp(&before, &b, sizeof(b)));
    damage_file(path);
    CHECK(storage_load(path, &b, error, sizeof(error)) == LOAD_ERROR);
    CHECK(!memcmp(&before, &b, sizeof(b)));
    a = before;
    a.orders[0].reserved_cash = MAX_MONEY;
    CHECK(!game_validate(&a, error, sizeof(error)));
    a = before;
    a.companies[1].id = a.companies[0].id;
    CHECK(!storage_checkpoint(path, &a, error, sizeof(error)));
    for (i = 0; i < 3; ++i)
        (void)remove(names[i]);
}

/** [학습 설명]
 * 함수: dump_save
 * 이 모듈 안에서만 쓰는 보조 함수입니다. 아래 인수와 반환값을 이용해 상위 공개 함수의 처리를 나눕니다.
 * 매개변수:
 * - path: 내부 저장 기준 경로. 일반 사용자가 선택하는 슬롯 인수가 아닙니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
static int dump_save(const char *path)
{
    char error[256];
    LoadResult result = storage_load(path, &a, error, sizeof(error));
    if (result != LOAD_OK && result != LOAD_BACKUP)
        return 1;
    printf("{\"tick\":%" PRIu64 ",\"cash\":%" PRId64 ",\"remaining\":%u,\"rng\":\"%" PRIu64
           "\",\"orders\":%u,\"trades\":%u,\"shares0\":%" PRId64 ",\"price0\":%" PRId64
           ",\"valid\":true}\n",
           a.tick_count, a.cash, a.remaining_ms, a.rng_state, a.order_count, a.trade_count,
           a.shares[0], a.companies[0].price);
    return 0;
}
/** [학습 설명]
 * 함수: test_console
 * 이름에 해당하는 기능을 정상·실패 사례로 검사합니다. CHECK 조건을 따라 기대값과 실제값을 비교해 보세요.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
static int test_console(void)
{
    ScreenState state = {0};
    CONSOLE_SCREEN_BUFFER_INFO before_info, after_info;
    HANDLE active;
    unsigned int i;
    wchar_t text[4096];
    DWORD read;
    COORD origin = {0, 0};
    bool allocated = AllocConsole() != 0;
    HANDLE saved_output = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE console_handle;
    if (allocated)
        ShowWindow(GetConsoleWindow(), SW_HIDE);
    console_handle = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    CHECK(console_handle != INVALID_HANDLE_VALUE);
    CHECK(SetStdHandle(STD_OUTPUT_HANDLE, console_handle));
    game_init(&a, 19);
    screen_init();
    active = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    CHECK(active != INVALID_HANDLE_VALUE);
    CHECK(GetConsoleScreenBufferInfo(active, &before_info));
    for (i = 0; i < 100; ++i)
        screen_render(&a, &state, "Fixed screen test", "b 1 ", false, "test.sav");
    CHECK(GetConsoleScreenBufferInfo(active, &after_info));
    CHECK(before_info.dwSize.Y == after_info.dwSize.Y);
    CHECK(before_info.srWindow.Top == after_info.srWindow.Top);
    CHECK(after_info.dwCursorPosition.Y < after_info.srWindow.Bottom + 1);
    CHECK(ReadConsoleOutputCharacterW(active, text, 4095, origin, &read));
    text[read] = 0;
    CHECK(wcsstr(text, L"V2") != NULL && wcsstr(text, L"b 1 ") != NULL);
    for (i = VIEW_MARKET; i <= VIEW_HELP; ++i)
    {
        state.view = (ViewMode)i;
        state.selected_id = a.companies[0].id;
        screen_render(&a, &state, "View test", "b 1 ", false, "test.sav");
        CHECK(GetConsoleScreenBufferInfo(active, &after_info));
        CHECK(after_info.srWindow.Top == before_info.srWindow.Top);
    }
    {
        SMALL_RECT smaller = {0, 0, 79, 17};
        if (SetConsoleWindowInfo(active, TRUE, &smaller))
        {
            screen_render(&a, &state, "Small window test", "b 1 ", false, "test.sav");
            CHECK(ReadConsoleOutputCharacterW(active, text, 4095, origin, &read));
            text[read] = 0;
            CHECK(wcsstr(text, L"b 1 ") != NULL);
        }
    }
    CloseHandle(active);
    screen_shutdown();
    (void)SetStdHandle(STD_OUTPUT_HANDLE, saved_output);
    CloseHandle(console_handle);
    if (allocated)
        FreeConsole();
    puts("PASS: 100 native-console redraws without scroll; typed input preserved.");
    return 0;
}
/** [학습 설명]
 * 함수: main
 * 테스트 실행 진입점입니다. 핵심 규칙·파일 정책·고정 화면을 검사하고 실패 즉시 위치를 알립니다.
 * 매개변수:
 * - argc: 실행 인수 개수. 일반 실행은 추가 인수가 없습니다.
 * - argv: 실행 인수 문자열 배열. --help 외의 선택 인수는 거절합니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */

/* 시세 모델 검사: 양 극단의 방향과 반대 방향 종목을 통계로 확인합니다. */
static void test_market_v4(void)
{
    TickReport report;
    int fear, sample, tick;
    unsigned int duration_counts[14] = {0};
    Game initial;
    MarketPhase previous = PHASE_NORMAL;
    int consecutive = 0;
    for (fear = 5; fear <= 95; fear += 90)
    {
        unsigned int up = 0, down = 0;
        long long sum = 0;
        game_init(&a, 77);
        a.fear = a.fear_target = fear;
        a.phase = market_phase_from_fear(fear);
        for (sample = 0; sample < 1000; ++sample)
        {
            size_t i;
            memset(&report, 0, sizeof(report));
            for (i = 0; i < COMPANY_COUNT; ++i)
                price(&a, i, 100000);
            stock_prices_update(&a, &report);
            up += report.rising;
            down += report.falling;
            for (i = 0; i < COMPANY_COUNT; ++i)
                sum += a.companies[i].last_change_bp;
        }
        CHECK(up > 0 && down > 0);
        CHECK(fear < 50 ? sum > 0 && up > down : sum < 0 && down > up);
    }
    game_init(&a, 19);
    initial = a;
    for (tick = 0; tick < 3000; ++tick)
    {
        if (!a.phase_ticks_left)
        {
            market_begin(&a, &report);
            CHECK(a.phase_ticks_left >= 1 && a.phase_ticks_left <= MARKET_DURATION_MAX);
            ++duration_counts[a.phase_ticks_left];
        }
        else
            market_begin(&a, &report);
        CHECK(a.phase == market_phase_from_fear(a.fear));
        if (a.phase == previous)
            ++consecutive;
        else
        {
            previous = a.phase;
            consecutive = 1;
        }
        CHECK(consecutive <= 13);
        market_end(&a, &report);
        CHECK(a.phase_ticks_left < MARKET_DURATION_MAX);
    }
    for (tick = 1; tick <= 13; ++tick)
        CHECK(duration_counts[tick] > 0);
    game_init(&a, 19);
    for (tick = 0; tick < FINANCE_INTERVAL; ++tick)
        CHECK(game_tick(&a, &report));
    CHECK(a.companies[0].profit_bp != initial.companies[0].profit_bp ||
          a.companies[0].debt_percent != initial.companies[0].debt_percent ||
          a.companies[0].revenue_million != initial.companies[0].revenue_million);
    /* 일반/시연은 대기 간격만 다르고 같은 난수의 시장 결과는 같습니다. */
    game_init(&a, 400);
    b = a;
    for (tick = 0; tick < 480; ++tick)
    {
        b.remaining_ms = 3000;
        CHECK(game_tick(&a, &report) && game_tick(&b, &report));
        CHECK(!memcmp(&a, &b, sizeof(a)));
    }
}

/* 이름 200개를 실제 교체하며 검사합니다. 현재 30개 이름은 늘 유일해야 합니다. */
static void test_names_v4(void)
{
    char names[200][NAME_SIZE] = {{0}};
    unsigned int count = 0, iteration;
    game_init(&a, 78);
    for (iteration = 0; iteration < 5000; ++iteration)
    {
        size_t i;
        const char *name;
        CHECK(company_replace(&a, iteration % COMPANY_COUNT, STOCK_ORDINARY));
        name = a.companies[iteration % COMPANY_COUNT].name;
        for (i = 0; i < count && strcmp(names[i], name); ++i)
        {
        }
        if (i == count)
        {
            CHECK(count < 200);
            (void)snprintf(names[count++], NAME_SIZE, "%s", name);
        }
        CHECK(game_validate(&a, NULL, 0));
    }
    CHECK(count == 200);
}

/* 대기 없이 정상 game_tick을 실행합니다. 실제 게임과 같은 가격 규칙입니다.
 * CSV의 index는 매 회차 30개에 동일 비중 재투자한 비용 전 가상 지수입니다.
 * 폐지 종목 수익률은 -100%, 새 회사 초기 가격을 수익으로 넣지 않습니다.
 * buy_hold는 최초 30개 동일 금액 보유, 폐지 자산은 0으로 처리합니다. */
static int simulate_market(void)
{
    unsigned int run, horizon;
    puts("seed,ticks,index_return_pct,buy_hold_pct,delisted,blue_delisted,ordinary_delisted,spec_"
         "delisted,mean_lifetime,extreme_greed,greed,neutral,fear,extreme_fear");
    for (horizon = 480; horizon <= 4800; horizon *= 10)
    {
        for (run = 1; run <= 200; ++run)
        {
            double index = 1.0, hold = 0, lifetime = 0;
            int64_t initial_prices[COMPANY_COUNT];
            uint64_t initial_ids[COMPANY_COUNT], born[COMPANY_COUNT] = {0};
            unsigned int regimes[5] = {0}, delists[3] = {0}, tick, total = 0;
            size_t i;
            TickReport report;
            game_init(&a, run);
            for (i = 0; i < COMPANY_COUNT; ++i)
            {
                initial_prices[i] = a.companies[i].price;
                initial_ids[i] = a.companies[i].id;
            }
            for (tick = 0; tick < horizon; ++tick)
            {
                double factor = 0;
                before = a;
                if (!game_tick(&a, &report))
                    return 1;
                for (i = 0; i < COMPANY_COUNT; ++i)
                {
                    if (a.companies[i].id != before.companies[i].id)
                    {
                        ++delists[before.companies[i].type];
                        ++total;
                        lifetime += (double)(a.tick_count - born[i]);
                        born[i] = a.tick_count;
                    }
                    else
                        factor += (double)a.companies[i].price / (double)before.companies[i].price;
                }
                index *= factor / COMPANY_COUNT;
                ++regimes[a.phase];
                if (a.phase_ticks_left > 13 || a.phase != market_phase_from_fear(a.fear))
                    return 2;
            }
            for (i = 0; i < COMPANY_COUNT; ++i)
                if (initial_ids[i] == a.companies[i].id)
                    hold +=
                        (double)a.companies[i].price / (double)initial_prices[i] / COMPANY_COUNT;
            printf("%u,%u,%.5f,%.5f,%u,%u,%u,%u,%.2f,%u,%u,%u,%u,%u\n", run, horizon,
                   (index - 1) * 100, (hold - 1) * 100, total, delists[0], delists[1], delists[2],
                   total ? lifetime / total : 0, regimes[PHASE_EXTREME_GREED], regimes[PHASE_GREED],
                   regimes[PHASE_NORMAL], regimes[PHASE_FEAR], regimes[PHASE_CRASH]);
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--simulate"))
        return simulate_market();
    if (argc == 3 && !strcmp(argv[1], "--dump"))
        return dump_save(argv[2]);
    if (argc == 2 && !strcmp(argv[1], "--console"))
        return test_console();
    if (argc != 2)
    {
        puts("test_game TEST_DIRECTORY | --dump SAVE | --console");
        return 1;
    }
    test_init();
    test_accounting();
    test_orders();
    test_delist();
    test_market();
    test_market_v4();
    test_names_v4();
    test_fundamentals_and_capacity();
    test_query_and_input();
    test_storage(argv[1]);
    printf("PASS: %lu checks; 10,000 full ticks; probabilistic crash; fees, orders, storage.\n",
           checks);
    return 0;
}
