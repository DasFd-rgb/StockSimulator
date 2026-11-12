/** [모듈 설명]
 * saveLoad.c - 바이너리 저장과 복구
 * 역할: 구조체 전체를 fwrite하지 않고 모든 필드를 명시적으로 기록합니다. 복구한 데이터도 거래 예약 규칙에 맞는지 검사합니다.
 * 관리 대상: 파일; 불러오기 성공 시 Game 전체
 * 읽는 순서: 같은 이름의 .h 선언 → 구조체 → 공개 함수 → 내부 보조 함수.
 */
#include "saveLoad.h"
#include "company.h"
#include "market.h"
#include "portfolio.h"
#include <errno.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#define HEADER_SIZE 24U
#define FILE_LIMIT (256U * 1024U)
#define PATH_LIMIT 2048
typedef struct
{
    unsigned char *data;
    size_t position, limit;
    bool okay;
} Buffer;
/** [학습 설명]
 * 함수: fail
 * 오류 문장을 선택적으로 기록하고 false를 반환합니다.
 * 매개변수:
 * - error: 실패 이유를 기록할 출력 문자열. NULL이면 메시지를 생략합니다.
 * - size: 출력 문자열 공간의 바이트 수. 잘라 쓰기로 범위 초과를 방지합니다.
 * - text: 검사하거나 표시할 문자열. 호출자가 내용과 종료 문자를 준비합니다.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
static bool fail(char *error, size_t size, const char *text)
{
    if (error && size)
        (void)snprintf(error, size, "%s", text);
    return false;
}
/** [학습 설명]
 * 함수: money_valid
 * 0~MAX_MONEY 범위인지 검사합니다.
 * 매개변수:
 * - value: 계산·변환할 값 또는 계산 결과를 받을 출력 주소.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
static bool money_valid(int64_t value)
{
    return value >= 0 && value <= MAX_MONEY;
}
/** [학습 설명]
 * 함수: game_validate
 * 저장 전과 불러온 뒤 전체 상태의 범위와 관계를 검사합니다. 고유 ID·이름 중복, 가격 이력, 수량·원가, 예약금 합, 예약 매도
 * 수량, 기록 순서를 검사합니다. 오류 문장을 선택적으로 반환합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - error: 실패 이유를 기록할 출력 문자열. NULL이면 메시지를 생략합니다.
 * - size: 출력 문자열 공간의 바이트 수. 잘라 쓰기로 범위 초과를 방지합니다.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
bool game_validate(const Game *game, char *error, size_t size)
{
    size_t i, j;
    int64_t reservation = 0, sell_reserved[COMPANY_COUNT] = {0};
    if (!money_valid(game->cash) || game->realized < -MAX_MONEY || game->realized > MAX_MONEY ||
        !money_valid(game->fees_paid) || !money_valid(game->taxes_paid) || !game->rng_state ||
        game->next_company_id < 31 || !game->next_order_id || game->remaining_ms > TICK_MS ||
        game->fear < 0 || game->fear > 100 || game->order_count > ORDER_MAX ||
        game->trade_count > TRADE_HISTORY_MAX || game->asset_count < 1 ||
        game->asset_count > ASSET_HISTORY_MAX)
        return fail(error, size, "게임의 기본 값 또는 기록 개수가 올바르지 않습니다.");
    if (game->phase != market_phase_from_fear(game->fear) || game->fear_target < 0 ||
        game->fear_target > 100 || game->phase_ticks_left > MARKET_DURATION_MAX)
        return fail(error, size, "시장 상태 오류입니다.");
    for (i = 0; i < SECTOR_COUNT; ++i)
        if (game->sector_bp[i] < -120 || game->sector_bp[i] > 120)
            return fail(error, size, "업종 상태 오류입니다.");
    for (i = 0; i < COMPANY_COUNT; ++i)
    {
        const Company *c = &game->companies[i];
        if (!c->id || c->id >= game->next_company_id || !company_name_valid(c->name) ||
            c->type < 0 || c->type > STOCK_SPECULATIVE || c->sector < 0 ||
            c->sector >= SECTOR_COUNT || c->health < 0 || c->health > 100 || c->profit_bp < -3000 ||
            c->profit_bp > 3000 || c->debt_percent < 0 || c->debt_percent > 300 ||
            c->revenue_million < 1 || c->revenue_million > 1000000 || c->price <= DELIST_PRICE ||
            c->price > MAX_PRICE || c->last_change_bp < -3000 || c->last_change_bp > 3000 ||
            c->price_count < 1 || c->price_count > PRICE_HISTORY_MAX || game->shares[i] < 0 ||
            game->shares[i] > MAX_SHARES || !money_valid(game->cost_basis[i]) ||
            (game->shares[i] == 0 && game->cost_basis[i] != 0))
            return fail(error, size, "회사·보유 자산 오류입니다.");
        for (j = 0; j < c->price_count; ++j)
            if (c->prices[j] <= DELIST_PRICE || c->prices[j] > MAX_PRICE)
                return fail(error, size, "가격 이력 오류입니다.");
        if (c->prices[c->price_count - 1] != c->price)
            return fail(error, size, "현재가와 가격 이력이 다릅니다.");
        for (j = 0; j < i; ++j)
            if (c->id == game->companies[j].id || !strcmp(c->name, game->companies[j].name))
                return fail(error, size, "회사 ID 또는 이름이 중복됩니다.");
    }
    for (i = 0; i < game->order_count; ++i)
    {
        const Order *o = &game->orders[i];
        int slot = company_find(game, o->company_id);
        int64_t expected;
        if (!o->id || o->id >= game->next_order_id || slot < 0 ||
            o->created_tick > game->tick_count || (o->side != SIDE_BUY && o->side != SIDE_SELL) ||
            o->quantity < 1 || o->quantity > MAX_SHARES || o->limit_price <= DELIST_PRICE ||
            o->limit_price > MAX_PRICE || !money_valid(o->reserved_cash))
            return fail(error, size, "미체결 주문 오류입니다.");
        if (i && game->orders[i - 1].id >= o->id)
            return fail(error, size, "주문 접수 순서 오류입니다.");
        if (o->side == SIDE_BUY)
        {
            if (!buy_cost(o->limit_price, o->quantity, &expected) || expected != o->reserved_cash ||
                reservation > game->cash - expected)
                return fail(error, size, "현금 예약 오류입니다.");
            reservation += expected;
        }
        else
        {
            if (o->reserved_cash || o->quantity > game->shares[slot] - sell_reserved[slot])
                return fail(error, size, "주식 예약 오류입니다.");
            sell_reserved[slot] += o->quantity;
        }
    }
    for (i = 0; i < game->trade_count; ++i)
    {
        const TradeRecord *t = &game->trades[i];
        if (!t->company_id || t->company_id >= game->next_company_id ||
            !company_name_valid(t->name) || t->tick > game->tick_count ||
            t->order_id >= game->next_order_id || t->side < 0 || t->side > SIDE_DELIST ||
            t->quantity < 1 || t->quantity > MAX_SHARES || t->price < 0 || t->price > MAX_PRICE ||
            !money_valid(t->fee) || !money_valid(t->tax) || t->realized < -MAX_MONEY ||
            t->realized > MAX_MONEY || (t->side != SIDE_DELIST && t->price <= DELIST_PRICE) ||
            (t->side == SIDE_DELIST && (t->price || t->fee || t->tax || t->order_id)))
            return fail(error, size, "거래 기록 오류입니다.");
        if (i && t->tick < game->trades[i - 1].tick)
            return fail(error, size, "거래 기록 순서 오류입니다.");
    }
    for (i = 0; i < game->asset_count; ++i)
        if (game->assets[i].tick > game->tick_count || !money_valid(game->assets[i].cash) ||
            !money_valid(game->assets[i].total) || game->assets[i].total < game->assets[i].cash ||
            (i && game->assets[i].tick < game->assets[i - 1].tick))
            return fail(error, size, "자산 기록 오류입니다.");
    if (error && size)
        error[0] = '\0';
    return true;
}
/** [학습 설명]
 * 함수: write_number
 * 부호 없는 값을 작은 바이트부터 기록합니다.
 * 매개변수:
 * - b: 바이너리 버퍼 주소(현재 위치·끝·성공 여부 포함).
 * - value: 계산·변환할 값 또는 계산 결과를 받을 출력 주소.
 * - bytes: 바이너리 바이트 배열 또는 기록할 정수의 바이트 수.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void write_number(Buffer *b, uint64_t value, unsigned int bytes)
{
    unsigned int i;
    if (!b->okay || b->position + bytes > b->limit)
    {
        b->okay = false;
        return;
    }
    for (i = 0; i < bytes; ++i)
    {
        b->data[b->position++] = (unsigned char)value;
        value >>= 8;
    }
}
/** [학습 설명]
 * 함수: read_number
 * 버퍼 경계를 검사하며 작은 바이트부터 정수로 조합합니다.
 * 매개변수:
 * - b: 바이너리 버퍼 주소(현재 위치·끝·성공 여부 포함).
 * - bytes: 바이너리 바이트 배열 또는 기록할 정수의 바이트 수.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
static uint64_t read_number(Buffer *b, unsigned int bytes)
{
    uint64_t value = 0;
    unsigned int i;
    if (!b->okay || b->position + bytes > b->limit)
    {
        b->okay = false;
        return 0;
    }
    for (i = 0; i < bytes; ++i)
        value |= (uint64_t)b->data[b->position++] << (8 * i);
    return value;
}
/** [학습 설명]
 * 함수: write_signed
 * 음수를 포함하는 값을 ZigZag 방식의 부호 없는 값으로 변환합니다.
 * 매개변수:
 * - b: 바이너리 버퍼 주소(현재 위치·끝·성공 여부 포함).
 * - value: 계산·변환할 값 또는 계산 결과를 받을 출력 주소.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void write_signed(Buffer *b, int64_t value)
{
    uint64_t encoded = value >= 0 ? (uint64_t)value * 2 : (uint64_t)(-(value + 1)) * 2 + 1;
    write_number(b, encoded, 8);
}
/** [학습 설명]
 * 함수: read_signed
 * ZigZag 값을 원래 부호 있는 정수로 복원합니다.
 * 매개변수:
 * - b: 바이너리 버퍼 주소(현재 위치·끝·성공 여부 포함).
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
static int64_t read_signed(Buffer *b)
{
    uint64_t encoded = read_number(b, 8);
    return (encoded & 1) ? -(int64_t)(encoded >> 1) - 1 : (int64_t)(encoded >> 1);
}
/** [학습 설명]
 * 함수: name_transfer
 * 고정 크기 회사 이름을 버퍼와 주고받습니다.
 * 매개변수:
 * - b: 바이너리 버퍼 주소(현재 위치·끝·성공 여부 포함).
 * - name: 검사·출력·변환할 회사 이름 문자열.
 * - writing: true이면 기록 방향, false이면 복원 방향입니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void name_transfer(Buffer *b, char *name, bool writing)
{
    if (b->position + NAME_SIZE > b->limit)
    {
        b->okay = false;
        return;
    }
    if (writing)
        memcpy(b->data + b->position, name, NAME_SIZE);
    else
        memcpy(name, b->data + b->position, NAME_SIZE);
    b->position += NAME_SIZE;
}

/** [학습 설명]
 * 함수: transfer
 * 저장과 불러오기에 같은 필드 순서를 사용합니다. U64/U32/I64/SMALL 매크로가 반복을 줄입니다.
 * 매개변수:
 * - b: 바이너리 버퍼 주소(현재 위치·끝·성공 여부 포함).
 * - g: 바이너리로 변환하거나 복원할 게임 상태의 주소.
 * - writing: true이면 기록 방향, false이면 복원 방향입니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void transfer(Buffer *b, Game *g, bool writing)
{
    size_t i, j;
    uint64_t temp;
#define U64(x)                                                                                     \
    do                                                                                             \
    {                                                                                              \
        if (writing)                                                                               \
            write_number(b, (x), 8);                                                               \
        else                                                                                       \
            (x) = read_number(b, 8);                                                               \
    } while (0)
#define U32(x)                                                                                     \
    do                                                                                             \
    {                                                                                              \
        if (writing)                                                                               \
            write_number(b, (uint32_t)(x), 4);                                                     \
        else                                                                                       \
            (x) = (uint32_t)read_number(b, 4);                                                     \
    } while (0)
#define I64(x)                                                                                     \
    do                                                                                             \
    {                                                                                              \
        if (writing)                                                                               \
            write_signed(b, (x));                                                                  \
        else                                                                                       \
            (x) = read_signed(b);                                                                  \
    } while (0)
#define SMALL(x, lo, hi)                                                                           \
    do                                                                                             \
    {                                                                                              \
        if (writing)                                                                               \
            write_signed(b, (x));                                                                  \
        else                                                                                       \
        {                                                                                          \
            int64_t small_value = read_signed(b);                                                  \
            if (small_value < (lo) || small_value > (hi))                                          \
                b->okay = false;                                                                   \
            else                                                                                   \
                (x) = (int)small_value;                                                            \
        }                                                                                          \
    } while (0)
    I64(g->cash);
    I64(g->realized);
    I64(g->fees_paid);
    I64(g->taxes_paid);
    U64(g->rng_state);
    U64(g->next_company_id);
    U64(g->next_order_id);
    U64(g->tick_count);
    U64(g->save_generation);
    U32(g->remaining_ms);
    SMALL(g->fear, 0, 100);
    SMALL(g->fear_target, 0, 100);
    SMALL(g->phase, 0, 4);
    U32(g->phase_ticks_left);
    for (i = 0; i < SECTOR_COUNT; ++i)
        SMALL(g->sector_bp[i], -120, 120);
    U32(g->order_count);
    U32(g->trade_count);
    U32(g->asset_count);
    if (g->order_count > ORDER_MAX || g->trade_count > TRADE_HISTORY_MAX ||
        g->asset_count > ASSET_HISTORY_MAX)
    {
        b->okay = false;
        return;
    }
    for (i = 0; i < COMPANY_COUNT && b->okay; ++i)
    {
        Company *c = &g->companies[i];
        U64(c->id);
        name_transfer(b, c->name, writing);
        SMALL(c->type, 0, 2);
        SMALL(c->sector, 0, SECTOR_COUNT - 1);
        SMALL(c->health, 0, 100);
        SMALL(c->profit_bp, -3000, 3000);
        SMALL(c->debt_percent, 0, 300);
        I64(c->revenue_million);
        I64(c->price);
        SMALL(c->last_change_bp, -3000, 3000);
        temp = c->watched ? 1 : 0;
        U64(temp);
        if (temp > 1)
            b->okay = false;
        c->watched = temp != 0;
        U32(c->price_count);
        if (c->price_count > PRICE_HISTORY_MAX)
        {
            b->okay = false;
            return;
        }
        for (j = 0; j < c->price_count; ++j)
            I64(c->prices[j]);
        I64(g->shares[i]);
        I64(g->cost_basis[i]);
    }
    for (i = 0; i < g->order_count && b->okay; ++i)
    {
        Order *o = &g->orders[i];
        U64(o->id);
        U64(o->company_id);
        U64(o->created_tick);
        SMALL(o->side, 0, 1);
        I64(o->quantity);
        I64(o->limit_price);
        I64(o->reserved_cash);
    }
    for (i = 0; i < g->trade_count && b->okay; ++i)
    {
        TradeRecord *t = &g->trades[i];
        U64(t->tick);
        U64(t->company_id);
        U64(t->order_id);
        name_transfer(b, t->name, writing);
        SMALL(t->side, 0, 2);
        I64(t->quantity);
        I64(t->price);
        I64(t->fee);
        I64(t->tax);
        I64(t->realized);
    }
    for (i = 0; i < g->asset_count && b->okay; ++i)
    {
        U64(g->assets[i].tick);
        I64(g->assets[i].cash);
        I64(g->assets[i].total);
    }
#undef U64
#undef U32
#undef I64
#undef SMALL
}
/** [학습 설명]
 * 함수: checksum
 * 파일 내용의 FNV-1a 손상 검사값을 만듭니다. 암호가 아닙니다.
 * 매개변수:
 * - data: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * - size: 출력 문자열 공간의 바이트 수. 잘라 쓰기로 범위 초과를 방지합니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
static uint64_t checksum(const unsigned char *data, size_t size)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    size_t i;
    for (i = 0; i < size; ++i)
    {
        hash ^= data[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}
/** [학습 설명]
 * 함수: decode
 * 표식·버전·길이·검사값을 확인하고 임시 Game을 해석·검증합니다.
 * 매개변수:
 * - bytes: 바이너리 바이트 배열 또는 기록할 정수의 바이트 수.
 * - length: 바이트 배열의 유효 길이 또는 길이를 반환할 출력 주소.
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
static bool decode(unsigned char *bytes, size_t length, Game *game)
{
    Buffer buffer = {bytes, 8, length, true};
    uint64_t version, payload, hash;
    Game *next;
    bool okay;
    if (length < HEADER_SIZE || memcmp(bytes, "STOCKV4", 8))
        return false;
    version = read_number(&buffer, 4);
    payload = read_number(&buffer, 4);
    hash = read_number(&buffer, 8);
    if (version != SAVE_VERSION || payload != length - HEADER_SIZE ||
        hash != checksum(bytes + HEADER_SIZE, length - HEADER_SIZE))
        return false;
    next = calloc(1, sizeof(*next));
    if (!next)
        return false;
    transfer(&buffer, next, false);
    okay = buffer.okay && buffer.position == length && game_validate(next, NULL, 0);
    if (okay)
        *game = *next;
    free(next);
    return okay;
}

/** [학습 설명]
 * 함수: read_bytes
 * 최대 256KB까지만 읽고 부재·입출력 오류·형식 오류를 구분합니다.
 * 매개변수:
 * - path: 내부 저장 기준 경로. 일반 사용자가 선택하는 슬롯 인수가 아닙니다.
 * - bytes: 바이너리 바이트 배열 또는 기록할 정수의 바이트 수.
 * - length: 바이트 배열의 유효 길이 또는 길이를 반환할 출력 주소.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
static int read_bytes(const char *path, unsigned char *bytes, size_t *length)
{
    FILE *file;
    int extra, failed;
    errno = 0;
    file = fopen(path, "rb");
    if (!file)
        return errno == ENOENT ? 0 : -1;
    *length = fread(bytes, 1, FILE_LIMIT, file);
    extra = fgetc(file);
    failed = ferror(file);
    if (fclose(file))
        failed = 1;
    if (failed)
        return -1;
    return extra == EOF && *length >= HEADER_SIZE ? 1 : 2;
}
/** [학습 설명]
 * 함수: durable_write
 * fwrite, fflush, _commit, fclose의 성공 여부를 확인합니다.
 * 매개변수:
 * - path: 내부 저장 기준 경로. 일반 사용자가 선택하는 슬롯 인수가 아닙니다.
 * - bytes: 바이너리 바이트 배열 또는 기록할 정수의 바이트 수.
 * - length: 바이트 배열의 유효 길이 또는 길이를 반환할 출력 주소.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
static bool durable_write(const char *path, const unsigned char *bytes, size_t length)
{
    FILE *file = fopen(path, "wb");
    bool okay;
    if (!file)
        return false;
    okay = fwrite(bytes, 1, length, file) == length;
    if (okay)
        okay = fflush(file) == 0;
    if (okay)
        okay = _commit(_fileno(file)) == 0;
    if (fclose(file))
        okay = false;
    return okay;
}
/** [학습 설명]
 * 함수: path_with
 * 원본 경로 뒤에 .tmp와 .bak 등을 길이 검사 후 붙입니다.
 * 매개변수:
 * - destination: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * - path: 내부 저장 기준 경로. 일반 사용자가 선택하는 슬롯 인수가 아닙니다.
 * - suffix: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
static bool path_with(char destination[PATH_LIMIT], const char *path, const char *suffix)
{
    int result = snprintf(destination, PATH_LIMIT, "%s%s", path, suffix);
    return result >= 0 && result < PATH_LIMIT;
}

/* 검증과 직렬화 코드는 V2에서 재사용합니다. 파일 개수 정책만 아래에서 변경합니다. */
/** [학습 설명]
 * 함수: encode_snapshot
 * 기존 필드별 바이너리 변환 코드를 재사용하여 바이트 배열을 만듭니다. 원본을 보호하기 위해 Game 복사본을 사용하며 헤더에 V4
 * 버전·길이·손상 검사값을 씁니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - bytes: 바이너리 바이트 배열 또는 기록할 정수의 바이트 수.
 * - length: 바이트 배열의 유효 길이 또는 길이를 반환할 출력 주소.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
static bool encode_snapshot(const Game *game, unsigned char *bytes, size_t *length)
{
    Game *copy = malloc(sizeof(*copy));
    Buffer buffer = {bytes, HEADER_SIZE, FILE_LIMIT, true};
    if (!copy)
        return false;
    *copy = *game;
    transfer(&buffer, copy, true);
    free(copy);
    if (!buffer.okay)
        return false;
    *length = buffer.position;
    memcpy(bytes, "STOCKV4", 8);
    buffer.position = 8;
    write_number(&buffer, SAVE_VERSION, 4);
    write_number(&buffer, *length - HEADER_SIZE, 4);
    write_number(&buffer, checksum(bytes + HEADER_SIZE, *length - HEADER_SIZE), 8);
    return buffer.okay;
}

/* 복구 기록은 두 고정 파일만 번갈아 덮어씁니다.
 * 쓰기 중 종료되면 다른 파일과 본 저장이 남으므로 다음 실행에서 유효한 최신본을 고릅니다.
 * 본 저장은 여기서 바꾸지 않습니다. 저장 성공 후에만 메모리의 순번을 올립니다. */
/** [학습 설명]
 * 함수: storage_checkpoint
 * 게임 상태를 검증하고 저장 순번을 하나 높인 복사본을 두 고정 복구 파일 중 한 곳에 기록합니다. 본 저장은 변경하지 않습니다. 디스크
 * 반영까지 성공한 경우에만 메모리의 순번을 갱신합니다.
 * 매개변수:
 * - path: 내부 저장 기준 경로. 일반 사용자가 선택하는 슬롯 인수가 아닙니다.
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - error: 실패 이유를 기록할 출력 문자열. NULL이면 메시지를 생략합니다.
 * - size: 출력 문자열 공간의 바이트 수. 잘라 쓰기로 범위 초과를 방지합니다.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
bool storage_checkpoint(const char *path, Game *game, char *error, size_t size)
{
    unsigned char *bytes;
    Game *next;
    char recovery[PATH_LIMIT];
    size_t length = 0;
    bool okay = false;
    if (!game_validate(game, error, size))
        return false;
    if (game->save_generation == UINT64_MAX)
        return fail(error, size, "저장 순번의 계산 범위에 도달했습니다.");
    bytes = calloc(1, FILE_LIMIT);
    next = malloc(sizeof(*next));
    if (!bytes || !next)
    {
        free(bytes);
        free(next);
        return fail(error, size, "복구 기록에 필요한 메모리가 부족합니다.");
    }
    *next = *game;
    ++next->save_generation;
    if (!path_with(recovery, path, (next->save_generation & 1) ? ".tmp1" : ".tmp0"))
        fail(error, size, "저장 경로가 너무 깁니다.");
    else if (!encode_snapshot(next, bytes, &length) || !durable_write(recovery, bytes, length))
        fail(error, size, "복구 기록 실패: 저장 공간과 파일 권한을 확인한 뒤 q로 재시도하세요.");
    else
    {
        game->save_generation = next->save_generation;
        okay = true;
        if (error && size)
            error[0] = '\0';
    }
    free(next);
    free(bytes);
    return okay;
}

/* q 전용입니다. 먼저 복구 파일을 완성한 후 본 저장을 기록합니다.
 * 별도 .bak/.tmp 파일을 추가하지 않으므로 데이터 파일은 최대 3개입니다.
 * 본 저장 쓰기 중 중단되어도 직전에 완성한 복구 파일에서 같은 상태를 되살릴 수 있습니다. */
/** [학습 설명]
 * 함수: storage_finish
 * q 종료에만 호출합니다. 최신 복구 파일을 먼저 완성한 뒤 같은 상태를 본 저장에 기록합니다. 본 저장 쓰기가 중단되어도 복구 상태가
 * 남습니다. 별도 파일을 더 만들지 않습니다.
 * 매개변수:
 * - path: 내부 저장 기준 경로. 일반 사용자가 선택하는 슬롯 인수가 아닙니다.
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - error: 실패 이유를 기록할 출력 문자열. NULL이면 메시지를 생략합니다.
 * - size: 출력 문자열 공간의 바이트 수. 잘라 쓰기로 범위 초과를 방지합니다.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
bool storage_finish(const char *path, Game *game, char *error, size_t size)
{
    unsigned char *bytes;
    size_t length;
    bool okay;
    if (!storage_checkpoint(path, game, error, size))
        return false;
    bytes = calloc(1, FILE_LIMIT);
    if (!bytes)
        return fail(error, size, "본 저장용 메모리가 부족합니다.");
    okay = encode_snapshot(game, bytes, &length) && durable_write(path, bytes, length);
    free(bytes);
    if (!okay)
        return fail(error, size, "본 저장 실패. 최신 복구 상태는 보존했습니다. q로 재시도하세요.");
    if (error && size)
        error[0] = '\0';
    return true;
}

/* 본 저장과 복구 파일 둘을 모두 검사하고 저장 순번이 가장 큰 유효 상태를 자동 선택합니다.
 * 사용자가 과거 저장을 고르는 메뉴는 없습니다. 동일 순번이면 본 저장을 우선합니다. */
/** [학습 설명]
 * 함수: storage_load
 * 본 저장과 두 복구 파일의 길이·버전·검사값·상태를 검증하고 순번이 가장 큰 유효 후보를 자동 복원합니다. 사용자가 과거 상태를 고르는
 * 기능은 없습니다. 셋 모두 손상됐다면 기존 메모리를 바꾸지 않고 오류를 반환합니다.
 * 매개변수:
 * - path: 내부 저장 기준 경로. 일반 사용자가 선택하는 슬롯 인수가 아닙니다.
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - error: 실패 이유를 기록할 출력 문자열. NULL이면 메시지를 생략합니다.
 * - size: 출력 문자열 공간의 바이트 수. 잘라 쓰기로 범위 초과를 방지합니다.
 * 반환: 정상 본 저장/복구/파일 없음/오류를 구분한 LoadResult입니다.
 */
LoadResult storage_load(const char *path, Game *game, char *error, size_t size)
{
    unsigned char *bytes = malloc(FILE_LIMIT);
    Game *candidate = malloc(sizeof(*candidate));
    char names[3][PATH_LIMIT];
    size_t length;
    int i, selected = -1;
    bool any_file = false;
    uint64_t best_generation = 0;
    if (!bytes || !candidate)
    {
        free(bytes);
        free(candidate);
        fail(error, size, "복원할 메모리가 부족합니다.");
        return LOAD_ERROR;
    }
    if (!path_with(names[0], path, "") || !path_with(names[1], path, ".tmp0") ||
        !path_with(names[2], path, ".tmp1"))
    {
        free(bytes);
        free(candidate);
        fail(error, size, "저장 경로가 너무 깁니다.");
        return LOAD_ERROR;
    }
    for (i = 0; i < 3; ++i)
    {
        int status = read_bytes(names[i], bytes, &length);
        if (status)
            any_file = true;
        if (status == 1 && decode(bytes, length, candidate) &&
            (selected < 0 || candidate->save_generation > best_generation))
        {
            best_generation = candidate->save_generation;
            *game = *candidate;
            selected = i;
        }
    }
    free(candidate);
    free(bytes);
    if (selected > 0)
    {
        fail(error, size, "최근의 정상 복구 상태를 자동으로 이어서 시작합니다.");
        return LOAD_BACKUP;
    }
    if (selected == 0)
    {
        if (error && size)
            error[0] = '\0';
        return LOAD_OK;
    }
    if (!any_file)
        return LOAD_NOT_FOUND;
    fail(error, size, "세 저장 후보를 복원할 수 없습니다. 파일을 보존하고 실행을 중단합니다.");
    return LOAD_ERROR;
}
